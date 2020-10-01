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
using namespace ngfx;

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
    LOG(INFO, "%p %p %p", s->texture_descs.data, s->buffer_descs.data, s->attribute_descs.data);
    ngli_darray_init(&s->texture_descs, sizeof(struct pipeline_texture_desc), 0);
    ngli_darray_init(&s->buffer_descs,  sizeof(struct pipeline_buffer_desc), 0);
    ngli_darray_init(&s->attribute_descs, sizeof(struct pipeline_attribute_desc), 0);

    ngli_assert(ngli_darray_count(&s->uniforms) == 0);
    ngli_darray_init(&s->textures, sizeof(struct texture*), 0);
    ngli_darray_init(&s->buffers,  sizeof(struct buffer*), 0);
    ngli_darray_init(&s->attributes, sizeof(struct buffer*), 0);

    program_ngfx* program = (program_ngfx*)params->program;
    pipeline_ngfx* pipeline = (pipeline_ngfx*)s;
    gctx_ngfx* gctx = (gctx_ngfx*)pipeline->parent.gctx;

    if (params->type == NGLI_PIPELINE_TYPE_GRAPHICS) {
        GraphicsPipeline::State state;
        auto& rt_desc = params->graphics.rt_desc;
        GraphicsContext::RenderPassConfig renderPassConfig;
        renderPassConfig.enableDepthStencil = rt_desc.depth_stencil.format != NGLI_FORMAT_UNDEFINED;
        renderPassConfig.numColorAttachments = rt_desc.nb_colors;
        renderPassConfig.numSamples = std::max(rt_desc.colors[0].samples, 1);
        TODO("set renderPassConfig.offscreen");
        renderPassConfig.offscreen = true;
        state.renderPass = gctx->graphics_context->getRenderPass(renderPassConfig);
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

    upload_uniforms(s);

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

    ngli_gctx_ngfx_begin_render_pass(s->gctx);

    bind_pipeline(s);
    bind_vertex_buffers(cmd_buf, s);
    bind_buffers(cmd_buf, s);
    bind_textures(cmd_buf, s);
    TODO("Graphics::draw");

    ngli_gctx_ngfx_end_render_pass(s->gctx);
}
void ngli_pipeline_ngfx_draw_indexed(struct pipeline *s, struct buffer *indices, int indices_format, int nb_indices, int nb_instances) {
    struct gctx_ngfx *gctx_ngfx = (struct gctx_ngfx *)s->gctx;
    CommandBuffer *cmd_buf = gctx_ngfx->cur_command_buffer;

    ngli_gctx_ngfx_begin_render_pass(s->gctx);

    bind_pipeline(s);
    bind_vertex_buffers(cmd_buf, s);
    bind_buffers(cmd_buf, s);
    bind_textures(cmd_buf, s);

    TODO("Graphics::bindIndexBuffer");
    TODO("Graphics::drawIndexed");

    ngli_gctx_ngfx_end_render_pass(s->gctx);
}
void ngli_pipeline_ngfx_dispatch(struct pipeline *s, int nb_group_x, int nb_group_y, int nb_group_z) {
    struct gctx_ngfx *gctx_ngfx = (struct gctx_ngfx *)s->gctx;
    CommandBuffer *cmd_buf = gctx_ngfx->cur_command_buffer;

    ngli_gctx_ngfx_begin_render_pass(s->gctx);

    bind_pipeline(s);
    bind_vertex_buffers(cmd_buf, s);
    bind_buffers(cmd_buf, s);
    bind_textures(cmd_buf, s);

    TODO("Graphics::dispatch");

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
