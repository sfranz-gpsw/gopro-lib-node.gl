/*
 * Copyright 2019 GoPro Inc.
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "pipeline_ngfx.h"
#include "log.h"
#include "memory.h"
#include "graphics/Graphics.h"
#include "compute/ComputePipeline.h"
#include "graphics/GraphicsPipeline.h"
#include "graphics/Buffer.h"
#include "buffer_ngfx.h"
#include "gctx_ngfx.h"
#include "program_ngfx.h"
#include "format.h"
#include "type.h"
#include "texture_ngfx.h"
#include "topology_ngfx.h"
#include <map>
using namespace ngfx;

static IndexFormat to_ngfx_index_format(int indices_format)
{
    static const std::map<int, IndexFormat> index_format_map = {
        { NGLI_FORMAT_R16_UNORM, INDEXFORMAT_UINT16 },
        { NGLI_FORMAT_R32_UINT,  INDEXFORMAT_UINT32 }
    };
    return index_format_map.at(indices_format);
}

static const BlendFactor to_ngfx_blend_factor(int blend_factor)
{
    static const std::map<int, BlendFactor> blend_factor_map = {
        { NGLI_BLEND_FACTOR_ZERO                , BLEND_FACTOR_ZERO },
        { NGLI_BLEND_FACTOR_ONE                 , BLEND_FACTOR_ONE },
        { NGLI_BLEND_FACTOR_SRC_COLOR           , BLEND_FACTOR_SRC_COLOR },
        { NGLI_BLEND_FACTOR_ONE_MINUS_SRC_COLOR , BLEND_FACTOR_ONE_MINUS_SRC_COLOR },
        { NGLI_BLEND_FACTOR_DST_COLOR           , BLEND_FACTOR_DST_COLOR },
        { NGLI_BLEND_FACTOR_ONE_MINUS_DST_COLOR , BLEND_FACTOR_ONE_MINUS_DST_COLOR },
        { NGLI_BLEND_FACTOR_SRC_ALPHA           , BLEND_FACTOR_SRC_ALPHA },
        { NGLI_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA , BLEND_FACTOR_ONE_MINUS_SRC_ALPHA },
        { NGLI_BLEND_FACTOR_DST_ALPHA           , BLEND_FACTOR_DST_ALPHA },
        { NGLI_BLEND_FACTOR_ONE_MINUS_DST_ALPHA , BLEND_FACTOR_ONE_MINUS_DST_ALPHA }
    };

    return blend_factor_map.at(blend_factor);
}

static BlendOp to_ngfx_blend_op(int blend_op)
{
    static const std::map<int, BlendOp> blend_op_map = {
        { NGLI_BLEND_OP_ADD              , BLEND_OP_ADD },
        { NGLI_BLEND_OP_SUBTRACT         , BLEND_OP_SUBTRACT },
        { NGLI_BLEND_OP_REVERSE_SUBTRACT , BLEND_OP_REVERSE_SUBTRACT },
        { NGLI_BLEND_OP_MIN              , BLEND_OP_MIN },
        { NGLI_BLEND_OP_MAX              , BLEND_OP_MAX },
    };
    return blend_op_map.at(blend_op);
}

static ColorComponentFlags to_ngfx_color_mask(int color_write_mask)
{
    return (color_write_mask & NGLI_COLOR_COMPONENT_R_BIT ? VK_COLOR_COMPONENT_R_BIT : 0)
         | (color_write_mask & NGLI_COLOR_COMPONENT_G_BIT ? VK_COLOR_COMPONENT_G_BIT : 0)
         | (color_write_mask & NGLI_COLOR_COMPONENT_B_BIT ? VK_COLOR_COMPONENT_B_BIT : 0)
         | (color_write_mask & NGLI_COLOR_COMPONENT_A_BIT ? VK_COLOR_COMPONENT_A_BIT : 0);
}

static CullModeFlags to_ngfx_cull_mode(int cull_mode)
{
    static std::map<int, CullModeFlags> cull_mode_map = {
        { NGLI_CULL_MODE_NONE          , CULL_MODE_NONE },
        { NGLI_CULL_MODE_FRONT_BIT     , CULL_MODE_FRONT_BIT },
        { NGLI_CULL_MODE_BACK_BIT      , CULL_MODE_BACK_BIT },
    };
    return cull_mode_map.at(cull_mode);
}

static int build_attribute_descs(pipeline *s, const pipeline_desc_params *params)
{
    for (int i = 0; i < params->nb_attributes; i++) {
        const struct pipeline_attribute_desc *pipeline_attribute_desc = &params->attributes_desc[i];

        if (!ngli_darray_push(&s->attribute_descs, pipeline_attribute_desc))
            return NGL_ERROR_MEMORY;
    }

    return 0;
}

struct pipeline *ngli_pipeline_ngfx_create(struct gctx *gctx)
{
    pipeline_ngfx *s = (pipeline_ngfx*)ngli_calloc(1, sizeof(*s));
    if (!s)
        return NULL;
    s->parent.gctx = gctx;
    return (struct pipeline *)s;
}
int ngli_pipeline_ngfx_init(struct pipeline *s, const struct pipeline_desc_params *params)
{
    s->type     = params->type;
    s->graphics = params->graphics;
    s->program  = params->program;

    ngli_assert(ngli_darray_count(&s->uniform_descs) == 0);
    LOG(WARNING, "%p %p %p", s->texture_descs.data, s->buffer_descs.data, s->attribute_descs.data);
    ngli_darray_init(&s->texture_descs, sizeof(struct pipeline_texture_desc), 0);
    ngli_darray_init(&s->buffer_descs,  sizeof(struct pipeline_buffer_desc), 0);
    ngli_darray_init(&s->attribute_descs, sizeof(struct pipeline_attribute_desc), 0);

    ngli_assert(ngli_darray_count(&s->uniforms) == 0);
    ngli_darray_init(&s->textures, sizeof(struct texture*), 0);
    ngli_darray_init(&s->buffers,  sizeof(struct buffer*), 0);
    ngli_darray_init(&s->attributes, sizeof(struct buffer*), 0);

    program_ngfx* program = (program_ngfx*)params->program;
    pipeline_ngfx* pipeline = (pipeline_ngfx*)s;
    pipeline_graphics *graphics = &s->graphics;
    graphicstate *gs = &graphics->state;
    gctx_ngfx* gctx = (gctx_ngfx*)pipeline->parent.gctx;

    if (params->type == NGLI_PIPELINE_TYPE_GRAPHICS) {
        int ret = build_attribute_descs(s, params);
        if (ret < 0)
            return ret;
        GraphicsPipeline::State state;

        auto& rt_desc = params->graphics.rt_desc;
        GraphicsContext::RenderPassConfig renderPassConfig;
        renderPassConfig.enableDepthStencil = rt_desc.depth_stencil.format != NGLI_FORMAT_UNDEFINED;
        renderPassConfig.numColorAttachments = rt_desc.nb_colors;
        renderPassConfig.numSamples = std::max(rt_desc.colors[0].samples, 1);
        TODO("remove renderPassConfig.offscreen param");
        renderPassConfig.offscreen = true;
        state.renderPass = gctx->graphics_context->getRenderPass(renderPassConfig);

        state.primitiveTopology = to_ngfx_topology(s->graphics.topology);

        state.blendEnable = gs->blend;
        state.colorBlendOp = to_ngfx_blend_op(gs->blend_op);
        state.srcColorBlendFactor = to_ngfx_blend_factor(gs->blend_src_factor);
        state.dstColorBlendFactor = to_ngfx_blend_factor(gs->blend_dst_factor);
        state.alphaBlendOp = to_ngfx_blend_op(gs->blend_op_a);
        state.srcAlphaBlendFactor = to_ngfx_blend_factor(gs->blend_src_factor_a);
        state.dstAlphaBlendFactor = to_ngfx_blend_factor(gs->blend_dst_factor_a);

        state.colorWriteMask = to_ngfx_color_mask(gs->color_write_mask);

        state.cullModeFlags = to_ngfx_cull_mode(gs->cull_mode);

        pipeline->gp = GraphicsPipeline::create(gctx->graphics_context, state, program->vs, program->fs, PIXELFORMAT_UNDEFINED, PIXELFORMAT_UNDEFINED);
    }
    else if (params->type == NGLI_PIPELINE_TYPE_COMPUTE) {
        pipeline->cp = ComputePipeline::create(gctx->graphics_context, program->cs);
    }
    return 0;
}

static int update_blocks(struct pipeline* s,  const struct pipeline_desc_params *pipeline_desc_params) {
    /* Uniform blocks */
    memcpy(s->ublock, pipeline_desc_params->ublock, sizeof(s->ublock));
    memcpy(s->ubuffer, pipeline_desc_params->ubuffer, sizeof(s->ubuffer));
    for (int i = 0; i < NGLI_PROGRAM_SHADER_NB; i++) {
        const struct buffer *ubuffer = pipeline_desc_params->ubuffer[i];
        if (!ubuffer)
            continue;
        const struct block *ublock = pipeline_desc_params->ublock[i];
        s->udata[i] = (uint8_t*)ngli_calloc(1, ublock->size);
        if (!s->udata[i])
            return NGL_ERROR_MEMORY;
    }
    return 0;
}

static int upload_uniforms(struct pipeline *s)
{
    for (int i = 0; i < NGLI_PROGRAM_SHADER_NB; i++) {
        const uint8_t *udata = s->udata[i];
        if (!udata)
            continue;
        struct buffer *ubuffer = s->ubuffer[i];
        const struct block *ublock = s->ublock[i];
        int ret = ngli_buffer_upload(ubuffer, udata, ublock->size, 0);
        if (ret < 0)
            return ret;
    }

    return 0;
}

static int bind_pipeline(struct pipeline *s) {
    pipeline_ngfx* pipeline = (pipeline_ngfx*)s;
    struct gctx_ngfx *gctx_ngfx = (struct gctx_ngfx *)s->gctx;
    CommandBuffer* cmd_buf = gctx_ngfx->cur_command_buffer;
    if (pipeline->gp) gctx_ngfx->graphics->bindGraphicsPipeline(cmd_buf, pipeline->gp);
    else if (pipeline->cp) gctx_ngfx->graphics->bindComputePipeline(cmd_buf, pipeline->cp);
    return 0;
}

int ngli_pipeline_ngfx_bind_resources(struct pipeline *s, const struct pipeline_desc_params *desc_params,
                                      const struct pipeline_resource_params *data_params)
{
    int ret;
    if ((ret = update_blocks(s, desc_params)) < 0)
        return ret;
    ngli_darray_clear(&s->attributes);
    ngli_darray_clear(&s->buffers);
    ngli_darray_clear(&s->textures);
    for (int i = 0; i<data_params->nb_attributes; i++) {
        const struct buffer **attribute = &data_params->attributes[i];
        if (!ngli_darray_push(&s->attributes, attribute))
            return NGL_ERROR_MEMORY;
        const struct pipeline_attribute_desc *pipeline_attribute_desc = &desc_params->attributes_desc[i];
        if (!ngli_darray_push(&s->attribute_descs, pipeline_attribute_desc))
            return NGL_ERROR_MEMORY;
    }
    for (int i = 0; i<data_params->nb_buffers; i++) {
        const struct buffer **buffer = &data_params->buffers[i];
        if (!ngli_darray_push(&s->buffers, buffer))
            return NGL_ERROR_MEMORY;
        const struct pipeline_buffer_desc *pipeline_buffer_desc = &desc_params->buffers_desc[i];
        if (!ngli_darray_push(&s->buffer_descs, pipeline_buffer_desc))
            return NGL_ERROR_MEMORY;
    }
    for (int i = 0; i<data_params->nb_textures; i++) {
        const struct texture **texture= &data_params->textures[i];
        if (!ngli_darray_push(&s->textures, texture))
            return NGL_ERROR_MEMORY;
        const struct pipeline_texture_desc *pipeline_texture_desc = &desc_params->textures_desc[i];
        if (!ngli_darray_push(&s->texture_descs, pipeline_texture_desc))
            return NGL_ERROR_MEMORY;
    }

    return 0;
}
int ngli_pipeline_ngfx_update_attribute(struct pipeline *s, int index, struct buffer *buffer) {
    TODO();
    return 0;
}
int ngli_pipeline_ngfx_update_uniform(struct pipeline *s, int index, const void *value) {
    if (index == -1)
        return NGL_ERROR_NOT_FOUND;

    const int stage = index >> 16;
    const int field_index = index & 0xffff;
    const struct block *block = s->ublock[stage];
    const struct block_field *field_info = (const struct block_field *)ngli_darray_data(&block->fields);
    const struct block_field *fi = &field_info[field_index];
    uint8_t *dst = s->udata[stage] + fi->offset;
    ngli_block_datacopy(fi, dst, (const uint8_t *)value);
    return 0;
}
int ngli_pipeline_ngfx_update_texture(struct pipeline *s, int index, struct texture *texture) {
    ngli_darray_set(&s->textures, index, &texture);
    return 0;
}

static void bind_buffers(CommandBuffer *cmd_buf, pipeline *s) {
    struct gctx_ngfx *gctx_ngfx = (struct gctx_ngfx *)s->gctx;
    int nb_buffers = ngli_darray_count(&s->buffers);
    for (int j = 0; j<nb_buffers; j++) {
        const buffer_ngfx *buffer = *(const buffer_ngfx **)ngli_darray_get(&s->buffers, j);
        const pipeline_buffer_desc &buffer_desc = *(const pipeline_buffer_desc *)ngli_darray_get(&s->buffer_descs, j);
        if (buffer_desc.type == NGLI_TYPE_UNIFORM_BUFFER) {
            gctx_ngfx->graphics->bindUniformBuffer(cmd_buf, buffer->v, buffer_desc.binding, buffer_desc.stage);
        }
        else {
            gctx_ngfx->graphics->bindStorageBuffer(cmd_buf, buffer->v, buffer_desc.binding, buffer_desc.stage);
        }
    }
}

static void bind_textures(CommandBuffer *cmd_buf, pipeline *s) {
    struct gctx_ngfx *gctx_ngfx = (struct gctx_ngfx *)s->gctx;
    int nb_textures = ngli_darray_count(&s->textures);
    for (int j = 0; j<nb_textures; j++) {
        const pipeline_texture_desc &texture_desc = *(const pipeline_texture_desc *)ngli_darray_get(&s->texture_descs, j);
        const texture_ngfx *texture = *(const texture_ngfx **)ngli_darray_get(&s->textures, j);
        gctx_ngfx->graphics->bindTexture(cmd_buf, texture->v, texture_desc.binding);
    }
}

static void bind_vertex_buffers(CommandBuffer *cmd_buf, pipeline *s) {
    struct gctx_ngfx *gctx_ngfx = (struct gctx_ngfx *)s->gctx;
    int nb_attributes = ngli_darray_count(&s->attributes);
    for (int j = 0; j<nb_attributes; j++) {
        const pipeline_attribute_desc &attr_desc = *(const pipeline_attribute_desc *)ngli_darray_get(&s->attribute_descs, j);
        const buffer_ngfx *buffer = *(const buffer_ngfx **)ngli_darray_get(&s->attributes, j);
        gctx_ngfx->graphics->bindVertexBuffer(cmd_buf, buffer->v, attr_desc.location);
    }
}

void ngli_pipeline_ngfx_draw(struct pipeline *s, int nb_vertices, int nb_instances) {
    struct gctx_ngfx *gctx_ngfx = (struct gctx_ngfx *)s->gctx;
    CommandBuffer *cmd_buf = gctx_ngfx->cur_command_buffer;

    upload_uniforms(s);

    //TODO: move to higher-level
    ngli_gctx_ngfx_begin_render_pass(s->gctx);

    bind_pipeline(s);
    bind_vertex_buffers(cmd_buf, s);
    bind_buffers(cmd_buf, s);
    bind_textures(cmd_buf, s);
    gctx_ngfx->graphics->draw(cmd_buf, nb_vertices, nb_instances);

}
void ngli_pipeline_ngfx_draw_indexed(struct pipeline *s, struct buffer *indices, int indices_format, int nb_indices, int nb_instances) {
    struct gctx_ngfx *gctx_ngfx = (struct gctx_ngfx *)s->gctx;
    CommandBuffer *cmd_buf = gctx_ngfx->cur_command_buffer;

    upload_uniforms(s);

    //TODO: move to higher-level
    ngli_gctx_ngfx_begin_render_pass(s->gctx);

    bind_pipeline(s);
    bind_vertex_buffers(cmd_buf, s);
    bind_buffers(cmd_buf, s);
    bind_textures(cmd_buf, s);

    gctx_ngfx->graphics->bindIndexBuffer(cmd_buf, ((buffer_ngfx *)indices)->v, to_ngfx_index_format(indices_format));
    gctx_ngfx->graphics->drawIndexed(cmd_buf, nb_indices, nb_instances);

}
void ngli_pipeline_ngfx_dispatch(struct pipeline *s, int nb_group_x, int nb_group_y, int nb_group_z) {
    struct gctx_ngfx *gctx_ngfx = (struct gctx_ngfx *)s->gctx;
    CommandBuffer *cmd_buf = gctx_ngfx->cur_command_buffer;

    upload_uniforms(s);

    //TODO: move to higher-level
    ngli_gctx_ngfx_begin_render_pass(s->gctx);

    bind_pipeline(s);
    bind_vertex_buffers(cmd_buf, s);
    bind_buffers(cmd_buf, s);
    bind_textures(cmd_buf, s);

    TODO("pass threads_per_group as params");
    int threads_per_group_x = 1, threads_per_group_y = 1, threads_per_group_z = 1;
    gctx_ngfx->graphics->dispatch(cmd_buf, nb_group_x, nb_group_y, nb_group_z, threads_per_group_x, threads_per_group_y, threads_per_group_z);

    //TODO: move to higher-level
    ngli_gctx_ngfx_end_render_pass(s->gctx);
}
void ngli_pipeline_ngfx_freep(struct pipeline **sp) {
    if (!*sp)
        return;

    pipeline *s = *sp;
    ngli_darray_reset(&s->uniform_descs);
    ngli_darray_reset(&s->texture_descs);
    ngli_darray_reset(&s->buffer_descs);
    ngli_darray_reset(&s->attribute_descs);

    ngli_darray_reset(&s->uniforms);
    ngli_darray_reset(&s->textures);
    ngli_darray_reset(&s->buffers);
    ngli_darray_reset(&s->attributes);
}
