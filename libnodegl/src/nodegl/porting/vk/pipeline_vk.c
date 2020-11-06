/*
 * Copyright 2016-2018 GoPro Inc.
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

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "nodegl/core/darray.h"
#include "nodegl/core/format.h"
#include "nodegl/porting/vk/gctx_vk.h"
#include "nodegl/core/hmap.h"
#include "nodegl/core/image.h"
#include "nodegl/core/log.h"
#include "nodegl/core/math_utils.h"
#include "nodegl/core/memory.h"
#include "nodegl/core/nodegl.h"
#include "nodegl/core/nodes.h"
#include "nodegl/porting/vk/pipeline_vk.h"
#include "nodegl/core/rendertarget.h"
#include "nodegl/porting/vk/texture_vk.h"
#include "nodegl/core/topology.h"
#include "nodegl/core/type.h"
#include "nodegl/core/utils.h"
#include "nodegl/core/pipeline_util.h"
#include "nodegl/porting/vk/vkcontext.h"
#include "nodegl/porting/vk/format_vk.h"
#include "nodegl/porting/vk/buffer_vk.h"
#include "nodegl/porting/vk/topology_vk.h"
#include "nodegl/porting/vk/program_vk.h"
#include "nodegl/porting/vk/rendertarget_vk.h"
#include "nodegl/porting/vk/vkutils.h"

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

static int init_attributes_data(struct pipeline *s, const struct pipeline_params *params)
{
    struct gctx_vk *gctx_vk = (struct gctx_vk *)s->gctx;
    struct vkcontext *vk = gctx_vk->vkcontext;
    struct pipeline_vk *s_priv = (struct pipeline_vk *)s;

    ngli_darray_init(&s_priv->vertex_attribute_descs, sizeof(VkVertexInputAttributeDescription), 0);
    ngli_darray_init(&s_priv->vertex_binding_descs,   sizeof(VkVertexInputBindingDescription), 0);
    ngli_darray_init(&s_priv->vertex_buffers, sizeof(VkBuffer), 0);
    ngli_darray_init(&s_priv->vertex_offsets, sizeof(VkDeviceSize), 0);

    for (int i = 0; i < params->nb_attributes; i++) {
        const struct pipeline_attribute_desc *desc = &params->attributes_desc[i];

        VkVertexInputBindingDescription binding_desc = {
            .binding   = i,
            .stride    = desc->stride,
            .inputRate = desc->rate ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX,
        };
        if (!ngli_darray_push(&s_priv->vertex_binding_descs, &binding_desc))
            return NGL_ERROR_MEMORY;

        VkFormat format = VK_FORMAT_UNDEFINED;
        int ret = ngli_format_get_vk_format(vk, desc->format, &format);
        if (ret < 0)
            return ret;

        VkVertexInputAttributeDescription attr_desc = {
            .binding  = i,
            .location = desc->location,
            .format   = format,
            .offset   = desc->offset,
        };
        if (!ngli_darray_push(&s_priv->vertex_attribute_descs, &attr_desc))
            return NGL_ERROR_MEMORY;

        VkBuffer buffer = VK_NULL_HANDLE;
        if (!ngli_darray_push(&s_priv->vertex_buffers, &buffer))
            return NGL_ERROR_MEMORY;

        VkDeviceSize offset = 0;
        if (!ngli_darray_push(&s_priv->vertex_offsets, &offset))
            return NGL_ERROR_MEMORY;

    }
    s_priv->nb_unbound_attributes = params->nb_attributes;

    return 0;
}

struct pipeline *ngli_pipeline_vk_create(struct gctx *gctx)
{
    struct pipeline_vk *s = ngli_calloc(1, sizeof(*s));
    if (!s)
        return NULL;
    s->parent.gctx = gctx;
    return (struct pipeline *)s;
}

static int pipeline_graphics_init(struct pipeline *s, const struct pipeline_params *params)
{
    struct gctx_vk *gctx_vk = (struct gctx_vk *)s->gctx;
    struct vkcontext *vk = gctx_vk->vkcontext;
    struct pipeline_vk *s_priv = (struct pipeline_vk *)s;
    struct pipeline_graphics *graphics = &s->graphics;
    struct graphicstate *state = &graphics->state;

    int ret = init_attributes_data(s, params);
    if (ret < 0)
        return ret;

    VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = ngli_darray_count(&s_priv->vertex_binding_descs),
        .pVertexBindingDescriptions = ngli_darray_data(&s_priv->vertex_binding_descs),
        .vertexAttributeDescriptionCount = ngli_darray_count(&s_priv->vertex_attribute_descs),
        .pVertexAttributeDescriptions = ngli_darray_data(&s_priv->vertex_attribute_descs),
    };

    VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = ngli_topology_get_vk_topology(graphics->topology),
    };

    VkViewport viewport = {
        .width = gctx_vk->width,
        .height = gctx_vk->height,
        .minDepth = 0.f,
        .maxDepth = 1.f,
    };

    VkRect2D scissor = {
        .extent = gctx_vk->extent,
    };

    VkPipelineViewportStateCreateInfo viewport_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor,
    };

    VkCullModeFlags cull_mode = get_vk_cull_mode(state->cull_mode);
    VkPipelineRasterizationStateCreateInfo rasterization_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .lineWidth = 1.f,
        .cullMode = cull_mode,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
    };

    const struct rendertarget_desc *desc = &graphics->rt_desc;
    const VkSampleCountFlagBits samples = ngli_vk_get_sample_count(desc->samples);
    VkPipelineMultisampleStateCreateInfo multisampling_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = samples,
    };

    VkPipelineDepthStencilStateCreateInfo depthstencil_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = state->depth_test,
        .depthWriteEnable = state->depth_write_mask,
        .depthCompareOp = get_vk_compare_op(state->depth_func),
        .depthBoundsTestEnable = 0,
        .stencilTestEnable = state->stencil_test,
        .front = {
            .failOp = get_vk_stencil_op(state->stencil_fail),
            .passOp = get_vk_stencil_op(state->stencil_depth_pass),
            .depthFailOp = get_vk_stencil_op(state->stencil_depth_fail),
            .compareOp = get_vk_compare_op(state->stencil_func),
            .compareMask = state->stencil_read_mask,
            .writeMask = state->stencil_write_mask,
            .reference = state->stencil_ref,
        },
        .back = {
            .failOp = get_vk_stencil_op(state->stencil_fail),
            .passOp = get_vk_stencil_op(state->stencil_depth_pass),
            .depthFailOp = get_vk_stencil_op(state->stencil_depth_fail),
            .compareOp = get_vk_compare_op(state->stencil_func),
            .compareMask = state->stencil_read_mask,
            .writeMask = state->stencil_write_mask,
            .reference = state->stencil_ref,
        },
        .minDepthBounds = 0.0f,
        .maxDepthBounds = 0.0f,
    };

    VkPipelineColorBlendAttachmentState colorblend_attachment_states[NGLI_MAX_COLOR_ATTACHMENTS] = {0};
    for (int i = 0; i < graphics->rt_desc.nb_colors; i++) {
        colorblend_attachment_states[i] = (VkPipelineColorBlendAttachmentState) {
            .blendEnable = state->blend,
            .srcColorBlendFactor = get_vk_blend_factor(state->blend_src_factor),
            .dstColorBlendFactor = get_vk_blend_factor(state->blend_dst_factor),
            .colorBlendOp = get_vk_blend_op(state->blend_op),
            .srcAlphaBlendFactor = get_vk_blend_factor(state->blend_src_factor_a),
            .dstAlphaBlendFactor = get_vk_blend_factor(state->blend_dst_factor_a),
            .alphaBlendOp = get_vk_blend_op(state->blend_op_a),
            .colorWriteMask = get_vk_color_write_mask(state->color_write_mask),
        };
    }

    VkPipelineColorBlendStateCreateInfo colorblend_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = graphics->rt_desc.nb_colors,
        .pAttachments = colorblend_attachment_states,
    };

    VkDynamicState dynamic_states[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_LINE_WIDTH,
    };

    VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = NGLI_ARRAY_NB(dynamic_states),
        .pDynamicStates = dynamic_states,
    };

    const struct program_vk *program_vk = (struct program_vk *)s->program;
    VkPipelineShaderStageCreateInfo shader_stage_create_info[2] = {
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = program_vk->shaders[NGLI_PROGRAM_SHADER_VERT].vkmodule,
            .pName = "main",
        },{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = program_vk->shaders[NGLI_PROGRAM_SHADER_FRAG].vkmodule,
            .pName = "main",
        },
    };

    VkRenderPass render_pass;
    ret = ngli_vk_create_compatible_renderpass(s->gctx, &graphics->rt_desc, &render_pass);
    if (ret < 0)
        return ret;

    VkGraphicsPipelineCreateInfo graphics_pipeline_create_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = NGLI_ARRAY_NB(shader_stage_create_info),
        .pStages = shader_stage_create_info,
        .pVertexInputState = &vertex_input_state_create_info,
        .pInputAssemblyState = &input_assembly_state_create_info,
        .pViewportState = &viewport_state_create_info,
        .pRasterizationState = &rasterization_state_create_info,
        .pMultisampleState = &multisampling_state_create_info,
        .pDepthStencilState = &depthstencil_state_create_info,
        .pColorBlendState = &colorblend_state_create_info,
        .pDynamicState = &dynamic_state_create_info,
        .layout = s_priv->pipeline_layout,
        .renderPass = render_pass,
        .subpass = 0,
    };

    VkResult res = vkCreateGraphicsPipelines(vk->device, NULL, 1, &graphics_pipeline_create_info, NULL, &s_priv->pipeline);

    vkDestroyRenderPass(vk->device, render_pass, NULL);

    if (res != VK_SUCCESS)
        return NGL_ERROR_EXTERNAL;

    s_priv->bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;

    return 0;
}

static int pipeline_compute_init(struct pipeline *s)
{
    struct gctx_vk *gctx_vk = (struct gctx_vk *)s->gctx;
    struct vkcontext *vk = gctx_vk->vkcontext;
    struct pipeline_vk *s_priv = (struct pipeline_vk *)s;

    const struct program_vk *program_vk = (struct program_vk *)s->program;

    VkPipelineShaderStageCreateInfo shader_stage_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = program_vk->shaders[NGLI_PROGRAM_SHADER_COMP].vkmodule,
        .pName = "main",
    };

    VkComputePipelineCreateInfo compute_pipeline_create_info = {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage = shader_stage_create_info,
        .layout = s_priv->pipeline_layout,
    };

    VkResult vkret = vkCreateComputePipelines(vk->device, NULL, 1, &compute_pipeline_create_info, NULL, &s_priv->pipeline);
    if (vkret != VK_SUCCESS)
        return NGL_ERROR_EXTERNAL;

    s_priv->bind_point = VK_PIPELINE_BIND_POINT_COMPUTE;

    return 0;
}

static int create_desc_set_layout_bindings(struct pipeline *s, const struct pipeline_params *params);
static int create_pipeline_layout(struct pipeline *s);

static int init_bindings(struct pipeline *s, const struct pipeline_params *params)
{
    struct gctx_vk *gctx = (struct gctx_vk *)s->gctx;
    struct pipeline_vk *s_priv = (struct pipeline_vk *)s;

    ngli_darray_init(&s_priv->attribute_bindings, sizeof(struct attribute_binding), 0);
    ngli_darray_init(&s_priv->buffer_bindings,  sizeof(struct buffer_binding), 0);
    ngli_darray_init(&s_priv->texture_bindings, sizeof(struct texture_binding), 0);

    for (int i = 0; i < params->nb_attributes; i++) {
        const struct pipeline_attribute_desc *desc = &params->attributes_desc[i];

        struct attribute_binding binding = {
            .desc = *desc,
        };
        if (!ngli_darray_push(&s_priv->attribute_bindings, &binding))
            return NGL_ERROR_MEMORY;
    }

    for (int i = 0; i < params->nb_buffers; i++) {
        const struct pipeline_buffer_desc *desc = &params->buffers_desc[i];

        struct buffer_binding binding = {
            .desc = *desc,
        };
        if (!ngli_darray_push(&s_priv->buffer_bindings, &binding))
            return NGL_ERROR_MEMORY;
    }

    for (int i = 0; i < params->nb_textures; i++) {
        const struct pipeline_texture_desc *desc = &params->textures_desc[i];

        struct texture_binding binding = {
            .desc = *desc,
        };
        if (!ngli_darray_push(&s_priv->texture_bindings, &binding))
            return NGL_ERROR_MEMORY;
    }

    return 0;
}

int ngli_pipeline_vk_init(struct pipeline *s, const struct pipeline_params *params)
{
    struct pipeline_vk *s_priv = (struct pipeline_vk *)s;

    s->type     = params->type;
    s->graphics = params->graphics;
    s->program  = params->program;

    init_bindings(s, params);

    int ret;
    if ((ret = create_desc_set_layout_bindings(s, params)) < 0)
        return ret;

    if ((ret = create_pipeline_layout(s)) < 0)
        return ret;

    if (params->type == NGLI_PIPELINE_TYPE_GRAPHICS) {
        ret = pipeline_graphics_init(s, params);
        if (ret < 0)
            return ret;
    } else if (params->type == NGLI_PIPELINE_TYPE_COMPUTE) {
        ret = pipeline_compute_init(s);
        if (ret < 0)
            return ret;
    } else {
        ngli_assert(0);
    }

    return 0;
}

static int create_desc_set_layout_bindings(struct pipeline *s, const struct pipeline_params *params)
{
    struct gctx_vk *gctx_vk = (struct gctx_vk *)s->gctx;
    struct vkcontext *vk = gctx_vk->vkcontext;
    struct pipeline_vk *s_priv = (struct pipeline_vk *)s;

    ngli_darray_init(&s_priv->desc_set_layout_bindings, sizeof(VkDescriptorSetLayoutBinding), 0);

    VkDescriptorPoolSize desc_pool_size_map[NGLI_TYPE_NB] = {
        [NGLI_TYPE_UNIFORM_BUFFER] = {.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER},
        [NGLI_TYPE_STORAGE_BUFFER] = {.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER},
        [NGLI_TYPE_SAMPLER_2D]     = {.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER},
        [NGLI_TYPE_SAMPLER_3D]     = {.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER},
        [NGLI_TYPE_SAMPLER_CUBE]   = {.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER},
        [NGLI_TYPE_IMAGE_2D]       = {.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE},
    };
    /* FIXME: Check access of size_map */

    for (int i = 0; i < params->nb_buffers; i++) {
        const struct pipeline_buffer_desc *desc = &params->buffers_desc[i];

        const VkDescriptorType type = get_descriptor_type(desc->type);
        const VkDescriptorSetLayoutBinding binding = {
            .binding         = desc->binding,
            .descriptorType  = type,
            .descriptorCount = 1,
            .stageFlags      = get_stage_flags(desc->stage),
        };
        if (!ngli_darray_push(&s_priv->desc_set_layout_bindings, &binding))
            return NGL_ERROR_MEMORY;

        desc_pool_size_map[desc->type].descriptorCount += gctx_vk->nb_in_flight_frames;
    }

    for (int i = 0; i < params->nb_textures; i++) {
        const struct pipeline_texture_desc *desc = &params->textures_desc[i];

        const VkDescriptorType type = get_descriptor_type(desc->type);
        const VkDescriptorSetLayoutBinding binding = {
            .binding         = desc->binding,
            .descriptorType  = type,
            .descriptorCount = 1,
            .stageFlags      = get_stage_flags(desc->stage),
        };
        if (!ngli_darray_push(&s_priv->desc_set_layout_bindings, &binding))
            return NGL_ERROR_MEMORY;

        desc_pool_size_map[desc->type].descriptorCount += gctx_vk->nb_in_flight_frames;
    }

    struct VkDescriptorPoolSize desc_pool_sizes[NGLI_ARRAY_NB(desc_pool_size_map)];
    int nb_desc_pool_sizes = 0;
    for (int i = 0; i < NGLI_ARRAY_NB(desc_pool_size_map); i++) {
        if (desc_pool_size_map[i].descriptorCount) {
            desc_pool_sizes[nb_desc_pool_sizes++] = desc_pool_size_map[i];
        }
    }

    if (!nb_desc_pool_sizes)
        return 0;

    VkDescriptorPoolCreateInfo descriptor_pool_create_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .poolSizeCount = nb_desc_pool_sizes,
        .pPoolSizes = desc_pool_sizes,
        .maxSets = gctx_vk->nb_in_flight_frames,
    };

    VkResult vkret = vkCreateDescriptorPool(vk->device, &descriptor_pool_create_info, NULL, &s_priv->desc_pool);
    if (vkret != VK_SUCCESS)
        return NGL_ERROR_EXTERNAL;

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = ngli_darray_count(&s_priv->desc_set_layout_bindings),
        .pBindings = (const VkDescriptorSetLayoutBinding *)ngli_darray_data(&s_priv->desc_set_layout_bindings),
    };

    vkret = vkCreateDescriptorSetLayout(vk->device, &descriptor_set_layout_create_info, NULL, &s_priv->desc_set_layout);
    if (vkret != VK_SUCCESS)
        return NGL_ERROR_EXTERNAL;

    VkDescriptorSetLayout *desc_set_layouts = ngli_calloc(gctx_vk->nb_in_flight_frames, sizeof(*desc_set_layouts));
    if (!desc_set_layouts)
        return NGL_ERROR_MEMORY;

    for (int i = 0; i < gctx_vk->nb_in_flight_frames; i++)
        desc_set_layouts[i] = s_priv->desc_set_layout;

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = s_priv->desc_pool,
        .descriptorSetCount = gctx_vk->nb_in_flight_frames,
        .pSetLayouts = desc_set_layouts
    };

    s_priv->desc_sets = ngli_calloc(gctx_vk->nb_in_flight_frames, sizeof(*s_priv->desc_sets));
    if (!s_priv->desc_sets) {
        ngli_free(desc_set_layouts);
        return NGL_ERROR_MEMORY;
    }

    vkret = vkAllocateDescriptorSets(vk->device, &descriptor_set_allocate_info, s_priv->desc_sets);
    if (vkret != VK_SUCCESS) {
        ngli_free(desc_set_layouts);
        return NGL_ERROR_EXTERNAL;
    }

    ngli_free(desc_set_layouts);
    return 0;
}

static int create_pipeline_layout(struct pipeline *s)
{
    struct gctx_vk *gctx_vk = (struct gctx_vk *)s->gctx;
    struct vkcontext *vk = gctx_vk->vkcontext;
    struct pipeline_vk *s_priv = (struct pipeline_vk *)s;

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
        .sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = s_priv->desc_set_layout ? 1 : 0,
        .pSetLayouts    = &s_priv->desc_set_layout,
    };

    VkResult vkret = vkCreatePipelineLayout(vk->device, &pipeline_layout_create_info, NULL, &s_priv->pipeline_layout);
    if (vkret != VK_SUCCESS)
        return NGL_ERROR_EXTERNAL;

    return 0;
}

int ngli_pipeline_vk_set_resources(struct pipeline *s, const struct pipeline_resource_params *params)
{
    struct pipeline_vk *s_priv = (struct pipeline_vk *)s;

    ngli_assert(ngli_darray_count(&s_priv->attribute_bindings) == params->nb_attributes);
    for (int i = 0; i < params->nb_attributes; i++) {
        int ret = ngli_pipeline_vk_update_attribute(s, i, params->attributes[i]);
        if (ret < 0)
            return ret;
    }

    ngli_assert(ngli_darray_count(&s_priv->buffer_bindings) == params->nb_buffers);
    for (int i = 0; i < params->nb_buffers; i++) {
        int ret = ngli_pipeline_vk_update_buffer(s, i, params->buffers[i]);
        if (ret < 0)
            return ret;
    }

    ngli_assert(ngli_darray_count(&s_priv->texture_bindings) == params->nb_textures);
    for (int i = 0; i < params->nb_textures; i++) {
        int ret = ngli_pipeline_vk_update_texture(s, i, params->textures[i]);
        if (ret < 0)
            return ret;
    }

    pipeline_update_blocks(s, params);

    return 0;
}

int ngli_pipeline_vk_update_attribute(struct pipeline *s, int index, struct buffer *buffer)
{
    struct pipeline_vk *s_priv = (struct pipeline_vk *)s;

    if (index == -1)
        return NGL_ERROR_NOT_FOUND;

    ngli_assert(s->type == NGLI_PIPELINE_TYPE_GRAPHICS);

    struct attribute_binding *attribute_binding = ngli_darray_get(&s_priv->attribute_bindings, index);
    ngli_assert(attribute_binding);

    const struct buffer *current_buffer = attribute_binding->buffer;
    if (!current_buffer && buffer)
        s_priv->nb_unbound_attributes--;
    else if (current_buffer && !buffer)
        s_priv->nb_unbound_attributes++;

    attribute_binding->buffer = buffer;

    VkBuffer *vertex_buffers = ngli_darray_data(&s_priv->vertex_buffers);
    if (buffer) {
        struct buffer_vk *buffer_vk = (struct buffer_vk *)buffer;
        vertex_buffers[index] = buffer_vk->buffer;
    } else {
        vertex_buffers[index] = VK_NULL_HANDLE;
    }

    return 0;
}

int ngli_pipeline_vk_update_uniform(struct pipeline *s, int index, const void *value)
{
    return pipeline_update_uniform(s, index, value);
}

int ngli_pipeline_vk_update_texture(struct pipeline *s, int index, struct texture *texture)
{
    struct gctx_vk *gctx_vk = (struct gctx_vk *)s->gctx;
    struct vkcontext *vk = gctx_vk->vkcontext;
    struct pipeline_vk *s_priv = (struct pipeline_vk *)s;

    struct texture_binding *texture_binding = ngli_darray_get(&s_priv->texture_bindings, index);
    ngli_assert(texture_binding);

    texture_binding->texture = texture;

    if (texture) {
        const struct pipeline_texture_desc *desc = &texture_binding->desc;
        const struct texture_vk *texture_vk = (struct texture_vk *)texture;
        VkDescriptorImageInfo image_info = {
            .imageLayout = get_image_layout(desc->type),
            .imageView   = texture_vk->image_view,
            .sampler     = texture_vk->image_sampler,
        };
        VkWriteDescriptorSet write_descriptor_set = {
            .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet           = s_priv->desc_sets[0],
            .dstBinding       = desc->binding,
            .dstArrayElement  = 0,
            .descriptorType   = get_descriptor_type(desc->type),
            .descriptorCount  = 1,
            .pImageInfo       = &image_info,
        };
        vkUpdateDescriptorSets(vk->device, 1, &write_descriptor_set, 0, NULL);
    }

    return 0;
}

int ngli_pipeline_vk_update_buffer(struct pipeline *s, int index, struct buffer *buffer)
{
    struct gctx_vk *gctx_vk = (struct gctx_vk *)s->gctx;
    struct vkcontext *vk = gctx_vk->vkcontext;
    struct pipeline_vk *s_priv = (struct pipeline_vk *)s;

    if (index == -1)
        return NGL_ERROR_NOT_FOUND;

    struct buffer_binding *buffer_binding = ngli_darray_get(&s_priv->buffer_bindings, index);
    ngli_assert(buffer_binding);

    buffer_binding->buffer = buffer;

    if (buffer) {
        const struct buffer_vk *buffer_vk = (struct buffer_vk *)buffer;
        const VkDescriptorBufferInfo descriptor_buffer_info = {
            .buffer = buffer_vk->buffer,
            .offset = 0,
            .range  = buffer->size,
        };
        const struct pipeline_buffer_desc *desc = &buffer_binding->desc;
        const VkWriteDescriptorSet write_descriptor_set = {
            .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet           = s_priv->desc_sets[0],
            .dstBinding       = desc->binding,
            .dstArrayElement  = 0,
            .descriptorType   = get_descriptor_type(desc->type),
            .descriptorCount  = 1,
            .pBufferInfo      = &descriptor_buffer_info,
            .pImageInfo       = NULL,
            .pTexelBufferView = NULL,
        };
        vkUpdateDescriptorSets(vk->device, 1, &write_descriptor_set, 0, NULL);
    }

    return 0;
}

static const VkIndexType vk_indices_type_map[NGLI_FORMAT_NB] = {
    [NGLI_FORMAT_R16_UNORM] = VK_INDEX_TYPE_UINT16,
    [NGLI_FORMAT_R32_UINT]  = VK_INDEX_TYPE_UINT32,
};

static VkIndexType get_vk_indices_type(int indices_format)
{
    return vk_indices_type_map[indices_format];
}

void ngli_pipeline_vk_draw(struct pipeline *s, int nb_vertices, int nb_instances)
{
    struct gctx_vk *gctx_vk = (struct gctx_vk *)s->gctx;
    struct pipeline_vk *s_priv = (struct pipeline_vk *)s;

    if (s_priv->nb_unbound_attributes) {
        LOG(ERROR, "pipeline has unbound vertex attributes");
        return;
    }

    pipeline_set_uniforms(s);

    VkCommandBuffer cmd_buf = gctx_vk->cur_command_buffer;

    vkCmdBindPipeline(cmd_buf, s_priv->bind_point, s_priv->pipeline);

    struct pipeline_graphics *graphics = &s->graphics;

    VkViewport viewport = {
        .x = gctx_vk->viewport[0],
        .y = gctx_vk->viewport[1],
        .width = gctx_vk->viewport[2],
        .height = gctx_vk->viewport[3],
        .minDepth = 0.f,
        .maxDepth = 1.f,
    };
    vkCmdSetViewport(cmd_buf, 0, 1, &viewport);

    vkCmdSetLineWidth(cmd_buf, 1.0f);

    struct rendertarget *rt = gctx_vk->rendertarget;
    VkRect2D scissor;
    if (graphics->state.scissor_test) {
        scissor.offset.x = gctx_vk->scissor[0];
        scissor.offset.y = NGLI_MAX(rt->height - gctx_vk->scissor[1] - gctx_vk->scissor[3], 0);
        scissor.extent.width = gctx_vk->scissor[2];
        scissor.extent.height = gctx_vk->scissor[3];
    } else {
        scissor.offset.x = 0;
        scissor.offset.y = 0;
        scissor.extent.width = rt->width;
        scissor.extent.height = rt->height;
    }

    vkCmdSetScissor(cmd_buf, 0, 1, &scissor);

    const int nb_vertex_buffers = ngli_darray_count(&s_priv->vertex_buffers);
    VkBuffer *vertex_buffers = ngli_darray_data(&s_priv->vertex_buffers);
    VkDeviceSize *vertex_offsets = ngli_darray_data(&s_priv->vertex_offsets);
    vkCmdBindVertexBuffers(cmd_buf, 0, nb_vertex_buffers, vertex_buffers, vertex_offsets);

    if (s_priv->desc_sets)
        vkCmdBindDescriptorSets(cmd_buf, s_priv->bind_point, s_priv->pipeline_layout,
                                0, 1, &s_priv->desc_sets[0], 0, NULL);

    vkCmdDraw(cmd_buf, nb_vertices, nb_instances, 0, 0);
}

void ngli_pipeline_vk_draw_indexed(struct pipeline *s, struct buffer *indices, int indices_format, int nb_indices, int nb_instances)
{
    struct gctx_vk *gctx_vk = (struct gctx_vk *)s->gctx;
    struct pipeline_vk *s_priv = (struct pipeline_vk *)s;

    if (s_priv->nb_unbound_attributes) {
        LOG(ERROR, "pipeline has unbound vertex attributes");
        return;
    }

    pipeline_set_uniforms(s);

    VkCommandBuffer cmd_buf = gctx_vk->cur_command_buffer;

    vkCmdBindPipeline(cmd_buf, s_priv->bind_point, s_priv->pipeline);

    struct pipeline_graphics *graphics = &s->graphics;

    VkViewport viewport = {
        .x = gctx_vk->viewport[0],
        .y = gctx_vk->viewport[1],
        .width = gctx_vk->viewport[2],
        .height = gctx_vk->viewport[3],
        .minDepth = 0.f,
        .maxDepth = 1.f,
    };
    vkCmdSetViewport(cmd_buf, 0, 1, &viewport);

    vkCmdSetLineWidth(cmd_buf, 1.0f);

    struct rendertarget *rt = gctx_vk->rendertarget;
    VkRect2D scissor;
    if (1 || graphics->state.scissor_test) {
        scissor.offset.x = gctx_vk->scissor[0];
        scissor.offset.y = NGLI_MAX(rt->height - gctx_vk->scissor[1] - gctx_vk->scissor[3], 0);
        scissor.extent.width = gctx_vk->scissor[2];
        scissor.extent.height = gctx_vk->scissor[3];
    } else {
        scissor.offset.x = 0;
        scissor.offset.y = 0;
        scissor.extent.width = rt->width;
        scissor.extent.height = rt->height;
    }

    vkCmdSetScissor(cmd_buf, 0, 1, &scissor);

    const int nb_vertex_buffers = ngli_darray_count(&s_priv->vertex_buffers);
    VkBuffer *vertex_buffers = ngli_darray_data(&s_priv->vertex_buffers);
    VkDeviceSize *vertex_offsets = ngli_darray_data(&s_priv->vertex_offsets);
    vkCmdBindVertexBuffers(cmd_buf, 0, nb_vertex_buffers, vertex_buffers, vertex_offsets);

    if (s_priv->desc_sets)
        vkCmdBindDescriptorSets(cmd_buf, s_priv->bind_point, s_priv->pipeline_layout,
                                0, 1, &s_priv->desc_sets[0], 0, NULL);

    struct buffer_vk *indices_vk = (struct buffer_vk *)indices;
    VkIndexType indices_type = get_vk_indices_type(indices_format);
    vkCmdBindIndexBuffer(cmd_buf, indices_vk->buffer, 0, indices_type);

    vkCmdDrawIndexed(cmd_buf, nb_indices, nb_instances, 0, 0, 0);
}

void ngli_pipeline_vk_dispatch(struct pipeline *s, int nb_group_x, int nb_group_y, int nb_group_z)
{
    struct gctx_vk *gctx_vk = (struct gctx_vk *)s->gctx;
    struct pipeline_vk *s_priv = (struct pipeline_vk *)s;

    pipeline_set_uniforms(s);

    VkCommandBuffer cmd_buf = gctx_vk->cur_command_buffer;

    vkCmdBindPipeline(cmd_buf, s_priv->bind_point, s_priv->pipeline);

    if (s_priv->desc_sets)
        vkCmdBindDescriptorSets(cmd_buf, s_priv->bind_point, s_priv->pipeline_layout,
                                0, 1, &s_priv->desc_sets[0], 0, NULL);

    vkCmdDispatch(cmd_buf, nb_group_x, nb_group_y, nb_group_z);

    // FIXME: use more specific barriers for buffers and images
    // FIXME: this is valid for the OpenGL implementation
    VkMemoryBarrier barrier = {
        .sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
    };
    const VkPipelineStageFlags src_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    const VkPipelineStageFlags dst_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    vkCmdPipelineBarrier(cmd_buf, src_stage, dst_stage, 0, 1, &barrier, 0, NULL, 0, NULL);
}

void ngli_pipeline_vk_freep(struct pipeline **sp)
{
    if (!*sp)
        return;

    struct pipeline *s = *sp;
    struct pipeline_vk *s_priv = (struct pipeline_vk *)s;
    struct gctx_vk *gctx_vk = (struct gctx_vk *)s->gctx;
    struct vkcontext *vk = gctx_vk->vkcontext;

    vkDestroyDescriptorPool(vk->device, s_priv->desc_pool, NULL);
    vkDestroyDescriptorSetLayout(vk->device, s_priv->desc_set_layout, NULL);
    ngli_free(s_priv->desc_sets);

    vkDestroyPipeline(vk->device, s_priv->pipeline, NULL);
    vkDestroyPipelineLayout(vk->device, s_priv->pipeline_layout, NULL);

    ngli_darray_reset(&s_priv->texture_bindings);
    ngli_darray_reset(&s_priv->buffer_bindings);
    ngli_darray_reset(&s_priv->attribute_bindings);

    ngli_darray_reset(&s_priv->vertex_attribute_descs);
    ngli_darray_reset(&s_priv->vertex_binding_descs);
    ngli_darray_reset(&s_priv->vertex_buffers);
    ngli_darray_reset(&s_priv->vertex_offsets);
    ngli_darray_reset(&s_priv->desc_set_layout_bindings);

    ngli_freep(sp);
}
