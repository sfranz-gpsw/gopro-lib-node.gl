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
#include "memory.h"
#include "debugutil_ngfx.h"
#include "ngfx/graphics/Graphics.h"
#include "ngfx/compute/ComputePipeline.h"
#include "ngfx/graphics/GraphicsPipeline.h"
#include "ngfx/graphics/Buffer.h"
#include "format.h"
#include "type.h"
#include "pipeline_util.h"
#include "buffer_ngfx.h"
#include "gctx_ngfx.h"
#include "program_ngfx.h"
#include "texture_ngfx.h"
#include "util_ngfx.h"
#include <map>
using namespace ngfx;

struct attribute_binding {
    struct pipeline_attribute_desc desc;
    struct buffer *buffer;
};

struct buffer_binding {
    struct pipeline_buffer_desc desc;
    struct buffer *buffer;
};

struct texture_binding {
    struct pipeline_texture_desc desc;
    struct texture *texture;
};

static int init_attributes_data(pipeline *s, const pipeline_params *params)
{
    gctx_ngfx *gtctx = (gctx_ngfx *)s->gctx;
    pipeline_ngfx *s_priv = (pipeline_ngfx *)s;

    ngli_darray_init(&s_priv->vertex_buffers, sizeof(ngfx::Buffer *), 0);

    for (int i = 0; i < params->nb_attributes; i++) {
        ngfx::Buffer *buffer = NULL;
        if (!ngli_darray_push(&s_priv->vertex_buffers, &buffer))
            return NGL_ERROR_MEMORY;
    }
    return 0;
}

pipeline *ngli_pipeline_ngfx_create(struct gctx *gctx)
{
    pipeline_ngfx *s = (pipeline_ngfx*)ngli_calloc(1, sizeof(*s));
    if (!s)
        return NULL;
    s->parent.gctx = gctx;
    return (pipeline *)s;
}

static std::set<std::string> get_instance_attributes(const pipeline_attribute_desc *attrs, int num_attrs) {
    std::set<std::string> instance_attrs;
    for (int j = 0; j<num_attrs; j++) {
        const auto& attr = attrs[j];
        if (attr.rate) instance_attrs.insert(attr.name);
    }
    return instance_attrs;
}

static int init_bindings(pipeline *s, const pipeline_params *params)
{
    gctx_ngfx *gctx = (gctx_ngfx *)s->gctx;
    pipeline_ngfx *s_priv = (pipeline_ngfx *)s;

    ngli_darray_init(&s_priv->attribute_bindings, sizeof(attribute_binding), 0);
    ngli_darray_init(&s_priv->buffer_bindings,  sizeof(buffer_binding), 0);
    ngli_darray_init(&s_priv->texture_bindings, sizeof(texture_binding), 0);

    for (int i = 0; i < params->nb_attributes; i++) {
        const pipeline_attribute_desc *desc = &params->attributes_desc[i];

        attribute_binding binding = {
            .desc = *desc,
        };
        if (!ngli_darray_push(&s_priv->attribute_bindings, &binding))
            return NGL_ERROR_MEMORY;
    }

    for (int i = 0; i < params->nb_buffers; i++) {
        const pipeline_buffer_desc *desc = &params->buffers_desc[i];

        buffer_binding binding = {
            .desc = *desc,
        };
        if (!ngli_darray_push(&s_priv->buffer_bindings, &binding))
            return NGL_ERROR_MEMORY;
    }

    for (int i = 0; i < params->nb_textures; i++) {
        const pipeline_texture_desc *desc = &params->textures_desc[i];

        texture_binding binding = {
            .desc = *desc,
        };
        if (!ngli_darray_push(&s_priv->texture_bindings, &binding))
            return NGL_ERROR_MEMORY;
    }

    return 0;
}

static int pipeline_graphics_init(pipeline *s, const pipeline_params *params)
{
    gctx_ngfx* gctx = (gctx_ngfx*)s->gctx;
    pipeline_ngfx *s_priv = (struct pipeline_ngfx *)s;
    program_ngfx* program = (program_ngfx*)params->program;
    pipeline_graphics *graphics = &s->graphics;
    graphicstate *gs = &graphics->state;

    const rendertarget_desc *rt_desc = &graphics->rt_desc;
    const attachment_desc *color_attachment_desc = &rt_desc->colors[0];
    const attachment_desc* depth_attachment_desc = &rt_desc->depth_stencil;

    int ret = init_attributes_data(s, params);
    if (ret < 0)
        return ret;

    GraphicsPipeline::State state;

    state.renderPass = get_render_pass(gctx->graphics_context, params->graphics.rt_desc);
    state.numColorAttachments = params->graphics.rt_desc.nb_colors;

    state.primitiveTopology = to_ngfx_topology(s->graphics.topology);

    state.blendEnable = gs->blend;
    state.colorBlendOp = to_ngfx_blend_op(gs->blend_op);
    state.srcColorBlendFactor = to_ngfx_blend_factor(gs->blend_src_factor);
    state.dstColorBlendFactor = to_ngfx_blend_factor(gs->blend_dst_factor);
    state.alphaBlendOp = to_ngfx_blend_op(gs->blend_op_a);
    state.srcAlphaBlendFactor = to_ngfx_blend_factor(gs->blend_src_factor_a);
    state.dstAlphaBlendFactor = to_ngfx_blend_factor(gs->blend_dst_factor_a);

    state.depthTestEnable = gs->depth_test;
    state.depthWriteEnable = gs->depth_write_mask;

    state.colorWriteMask = to_ngfx_color_mask(gs->color_write_mask);

    state.cullModeFlags = to_ngfx_cull_mode(gs->cull_mode);

    state.numSamples = glm::max(rt_desc->samples, 1);

    //Handle attribute stride mismatch
    for (int j = 0; j<params->nb_attributes; j++) {
        auto src_attr_desc = &params->attributes_desc[j];
        auto dst_attr_desc = program->vs->findAttribute(src_attr_desc->name);
        if (!dst_attr_desc) continue; //unused variable
        uint32_t src_attr_stride = src_attr_desc->stride;
        uint32_t dst_attr_stride = dst_attr_desc->elementSize * dst_attr_desc->count;
        if (src_attr_stride != dst_attr_stride) {
            dst_attr_desc->elementSize = src_attr_desc->stride / dst_attr_desc->count;
        }
    }
    s_priv->gp = GraphicsPipeline::create(
        gctx->graphics_context,
        state,
        program->vs, program->fs,
        to_ngfx_format(color_attachment_desc->format),
        depth_attachment_desc ? to_ngfx_format(depth_attachment_desc->format) : PIXELFORMAT_UNDEFINED,
        get_instance_attributes(params->attributes_desc, params->nb_attributes)
    );

    return 0;
}

static int pipeline_compute_init(pipeline *s, const pipeline_params *params)
{
    gctx_ngfx* gctx = (gctx_ngfx*)s->gctx;
    pipeline_ngfx *s_priv = (struct pipeline_ngfx *)s;
    program_ngfx* program = (program_ngfx*)params->program;
    s_priv->cp = ComputePipeline::create(gctx->graphics_context, program->cs);
    return 0;
}


int ngli_pipeline_ngfx_init(pipeline *s, const pipeline_params *params)
{
    pipeline_ngfx *s_priv = (pipeline_ngfx *)s;

    s->type     = params->type;
    s->graphics = params->graphics;
    s->program  = params->program;

    init_bindings(s, params);

    int ret;
    if (params->type == NGLI_PIPELINE_TYPE_GRAPHICS) {
        ret = pipeline_graphics_init(s, params);
        if (ret < 0)
            return ret;
    } else if (params->type == NGLI_PIPELINE_TYPE_COMPUTE) {
        ret = pipeline_compute_init(s, params);
        if (ret < 0)
            return ret;
    } else {
        ngli_assert(0);
    }

    return 0;
}

static int bind_pipeline(pipeline *s) {
    pipeline_ngfx* pipeline = (pipeline_ngfx*)s;
    gctx_ngfx *gctx = (gctx_ngfx *)s->gctx;
    CommandBuffer* cmd_buf = gctx->cur_command_buffer;
    if (pipeline->gp) gctx->graphics->bindGraphicsPipeline(cmd_buf, pipeline->gp);
    else if (pipeline->cp) gctx->graphics->bindComputePipeline(cmd_buf, pipeline->cp);
    return 0;
}

int ngli_pipeline_ngfx_set_resources(pipeline *s, const pipeline_resource_params *params)
{
    pipeline_ngfx *s_priv = (pipeline_ngfx *)s;

    ngli_assert(ngli_darray_count(&s_priv->attribute_bindings) == params->nb_attributes);
    for (int i = 0; i < params->nb_attributes; i++) {
        int ret = ngli_pipeline_ngfx_update_attribute(s, i, params->attributes[i]);
        if (ret < 0)
            return ret;
    }

    ngli_assert(ngli_darray_count(&s_priv->buffer_bindings) == params->nb_buffers);
    for (int i = 0; i < params->nb_buffers; i++) {
        int ret = ngli_pipeline_ngfx_update_buffer(s, i, params->buffers[i]);
        if (ret < 0)
            return ret;
    }

    ngli_assert(ngli_darray_count(&s_priv->texture_bindings) == params->nb_textures);
    for (int i = 0; i < params->nb_textures; i++) {
        int ret = ngli_pipeline_ngfx_update_texture(s, i, params->textures[i]);
        if (ret < 0)
            return ret;
    }

    int ret;
    if ((ret = pipeline_update_blocks(s, params)) < 0)
        return ret;

    return 0;
}

int ngli_pipeline_ngfx_update_attribute(pipeline *s, int index, buffer *p_buffer) {
    pipeline_ngfx *s_priv = (pipeline_ngfx *)s;

    if (index == -1)
        return NGL_ERROR_NOT_FOUND;

    ngli_assert(s->type == NGLI_PIPELINE_TYPE_GRAPHICS);

    attribute_binding *attr_binding = (attribute_binding *)ngli_darray_get(&s_priv->attribute_bindings, index);
    ngli_assert(attr_binding);

    const buffer *current_buffer = attr_binding->buffer;
    if (!current_buffer && p_buffer)
        s_priv->nb_unbound_attributes--;
    else if (current_buffer && !p_buffer)
        s_priv->nb_unbound_attributes++;

    attr_binding->buffer = p_buffer;

    ngfx::Buffer **vertex_buffers = (ngfx::Buffer **)ngli_darray_data(&s_priv->vertex_buffers);
    if (p_buffer) {
        buffer_ngfx *buffer = (struct buffer_ngfx *)p_buffer;
        vertex_buffers[index] = buffer->v;
    } else {
        vertex_buffers[index] = NULL;
    }

    return 0;
}

int ngli_pipeline_ngfx_update_uniform(pipeline *s, int index, const void *value) {
    return pipeline_update_uniform(s, index, value);
}

int ngli_pipeline_ngfx_update_texture(pipeline *s, int index, texture *p_texture) {
    gctx_ngfx *gctx = (gctx_ngfx *)s->gctx;
    pipeline_ngfx *s_priv = (pipeline_ngfx *)s;

    texture_binding *binding = (texture_binding *)ngli_darray_get(&s_priv->texture_bindings, index);
    ngli_assert(binding);
    LOG(INFO, "binding: [%d][%s]: %p", index, binding->desc.name, p_texture);
    binding->texture = p_texture;

    return 0;
}

int ngli_pipeline_ngfx_update_buffer(pipeline *s, int index, buffer *p_buffer)
{
    gctx_ngfx *gctx = (struct gctx_ngfx *)s->gctx;
    pipeline_ngfx *s_priv = (pipeline_ngfx *)s;

    if (index == -1)
        return NGL_ERROR_NOT_FOUND;

    buffer_binding *binding = (buffer_binding *)ngli_darray_get(&s_priv->buffer_bindings, index);
    ngli_assert(binding);

    binding->buffer = p_buffer;

    return 0;
}

static ShaderModule *get_shader_module(program_ngfx *program, int stage) {
    switch (stage) {
    case 0: return program->vs; break;
    case 1: return program->fs; break;
    case 2: return program->cs; break;
    }
    return nullptr;
}

static void bind_buffers(CommandBuffer *cmd_buf, pipeline *s) {
    gctx_ngfx *gctx = (gctx_ngfx *)s->gctx;
    pipeline_ngfx *s_priv = (pipeline_ngfx *)s;
    int nb_buffers = ngli_darray_count(&s_priv->buffer_bindings);
    program_ngfx *program = (program_ngfx *)s->program;
    for (int j = 0; j<nb_buffers; j++) {
        const buffer_binding *binding = (buffer_binding *)ngli_darray_get(&s_priv->buffer_bindings, j);
        const buffer_ngfx *buffer = (buffer_ngfx *)binding->buffer;
        const pipeline_buffer_desc &buffer_desc = binding->desc;
        ShaderModule *shader_module = get_shader_module(program, buffer_desc.stage);
        if (buffer_desc.type == NGLI_TYPE_UNIFORM_BUFFER) {
            auto buffer_info = shader_module->findUniformBufferInfo(buffer_desc.name);
            if (!buffer_info) continue; //unused variable
            gctx->graphics->bindUniformBuffer(cmd_buf, buffer->v, buffer_info->set, buffer_info->shaderStages);
        }
        else {
            auto buffer_info = shader_module->findStorageBufferInfo(buffer_desc.name);
            if (!buffer_info) continue; //unused variable
            gctx->graphics->bindStorageBuffer(cmd_buf, buffer->v, buffer_info->set, buffer_info->shaderStages);
        }
    }
}

static void bind_textures(CommandBuffer *cmd_buf, pipeline *s) {
    gctx_ngfx *gctx = (gctx_ngfx *)s->gctx;
    pipeline_ngfx *s_priv = (pipeline_ngfx *)s;
    int nb_textures = ngli_darray_count(&s_priv->texture_bindings);
    program_ngfx *program = (program_ngfx *)s->program;
    for (int j = 0; j<nb_textures; j++) {
        const texture_binding *binding = (texture_binding *)ngli_darray_get(&s_priv->texture_bindings, j);
        const pipeline_texture_desc &texture_desc = binding->desc;
        ShaderModule *shader_module = get_shader_module(program, texture_desc.stage);
        auto texture_info = shader_module->findDescriptorInfo(texture_desc.name);
        if (!texture_info) continue;
        const texture_ngfx *texture = (texture_ngfx *)binding->texture;
        if (!texture) texture = (texture_ngfx *)gctx->dummy_texture;
        LOG(INFO, "bind texture: [%d][%s]: %p set: %d", j, binding->desc.name, texture, texture_info->set);
        if (texture) gctx->graphics->bindTexture(cmd_buf, texture->v, texture_info->set);
    }
}

static void bind_vertex_buffers(CommandBuffer *cmd_buf, pipeline *s) {
    gctx_ngfx *gctx = (gctx_ngfx *)s->gctx;
    pipeline_ngfx *s_priv = (pipeline_ngfx *)s;
    int nb_attributes = ngli_darray_count(&s_priv->attribute_bindings);
    program_ngfx* program = (program_ngfx*)s->program;
    for (int j = 0; j<nb_attributes; j++) {
        const attribute_binding *attr_binding = (const attribute_binding *)ngli_darray_get(&s_priv->attribute_bindings, j);
        const pipeline_attribute_desc *attr_desc = &attr_binding->desc;
        auto dst_attr_desc = program->vs->findAttribute(attr_desc->name);
        if (!dst_attr_desc) continue; //unused variable
        const buffer_ngfx *buffer = (const buffer_ngfx *)attr_binding->buffer;
        uint32_t dst_attr_stride = dst_attr_desc->elementSize * dst_attr_desc->count;
        gctx->graphics->bindVertexBuffer(cmd_buf, buffer->v, dst_attr_desc->location, dst_attr_stride);
    }
}

static void set_viewport(CommandBuffer *cmd_buf, gctx_ngfx *gctx) {
    int *vp = gctx->viewport;
    gctx->graphics->setViewport(gctx->cur_command_buffer, { vp[0], vp[1], uint32_t(vp[2]), uint32_t(vp[3]) });
}

static void set_scissor(CommandBuffer *cmd_buf, gctx_ngfx *gctx) {
    int *sr = gctx->scissor;
    gctx->graphics->setScissor(gctx->cur_command_buffer, { sr[0], sr[1], uint32_t(sr[2]), uint32_t(sr[3]) });
}

void ngli_pipeline_ngfx_draw(pipeline *s, int nb_vertices, int nb_instances) {
    gctx_ngfx *gctx = (gctx_ngfx *)s->gctx;
    CommandBuffer *cmd_buf = gctx->cur_command_buffer;

    pipeline_set_uniforms(s);

    bind_pipeline(s);
    set_viewport(cmd_buf, gctx);
    set_scissor(cmd_buf, gctx);

    bind_vertex_buffers(cmd_buf, s);
    bind_buffers(cmd_buf, s);
    bind_textures(cmd_buf, s);

    gctx->graphics->draw(cmd_buf, nb_vertices, nb_instances);

}
void ngli_pipeline_ngfx_draw_indexed(pipeline *s, buffer *indices, int indices_format, int nb_indices, int nb_instances) {
    gctx_ngfx *gctx = (gctx_ngfx *)s->gctx;
    CommandBuffer *cmd_buf = gctx->cur_command_buffer;

    pipeline_set_uniforms(s);

    bind_pipeline(s);
    set_viewport(cmd_buf, gctx);
    set_scissor(cmd_buf, gctx);

    bind_vertex_buffers(cmd_buf, s);
    bind_buffers(cmd_buf, s);
    bind_textures(cmd_buf, s);

    gctx->graphics->bindIndexBuffer(cmd_buf, ((buffer_ngfx *)indices)->v, to_ngfx_index_format(indices_format));

    gctx->graphics->drawIndexed(cmd_buf, nb_indices, nb_instances);

}
void ngli_pipeline_ngfx_dispatch(pipeline *s, int nb_group_x, int nb_group_y, int nb_group_z) {
    gctx_ngfx *gctx = (gctx_ngfx *)s->gctx;
    CommandBuffer *cmd_buf = gctx->cur_command_buffer;

    pipeline_set_uniforms(s);

    bind_pipeline(s);
    bind_vertex_buffers(cmd_buf, s);
    bind_buffers(cmd_buf, s);
    bind_textures(cmd_buf, s);

    TODO("pass threads_per_group as params");
    int threads_per_group_x = 1, threads_per_group_y = 1, threads_per_group_z = 1;
    gctx->graphics->dispatch(cmd_buf, nb_group_x, nb_group_y, nb_group_z, threads_per_group_x, threads_per_group_y, threads_per_group_z);
}

void ngli_pipeline_ngfx_freep(pipeline **sp) {
    if (!*sp)
        return;

    pipeline *s = *sp;
    pipeline_ngfx *s_priv = (pipeline_ngfx *)s;

    if (s_priv->gp) delete s_priv->gp;
    if (s_priv->cp) delete s_priv->cp;

    ngli_darray_reset(&s_priv->texture_bindings);
    ngli_darray_reset(&s_priv->buffer_bindings);
    ngli_darray_reset(&s_priv->attribute_bindings);

    ngli_darray_reset(&s_priv->vertex_buffers);
}
