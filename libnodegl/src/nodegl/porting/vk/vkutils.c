/*
 * Copyright 2020 GoPro Inc.
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

#include "vkutils.h"
#include "nodegl/core/graphicstate.h"
#include "nodegl/core/type.h"
#include "nodegl/core/program.h"

const char *vk_res2str(VkResult res)
{
    switch (res) {
        case VK_SUCCESS:                        return "success";
        case VK_NOT_READY:                      return "not ready";
        case VK_TIMEOUT:                        return "timeout";
        case VK_EVENT_SET:                      return "event set";
        case VK_EVENT_RESET:                    return "event reset";
        case VK_INCOMPLETE:                     return "incomplete";
        case VK_ERROR_OUT_OF_HOST_MEMORY:       return "out of host memory";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:     return "out of device memory";
        case VK_ERROR_INITIALIZATION_FAILED:    return "initialization failed";
        case VK_ERROR_DEVICE_LOST:              return "device lost";
        case VK_ERROR_MEMORY_MAP_FAILED:        return "memory map failed";
        case VK_ERROR_LAYER_NOT_PRESENT:        return "layer not present";
        case VK_ERROR_EXTENSION_NOT_PRESENT:    return "extension not present";
        case VK_ERROR_FEATURE_NOT_PRESENT:      return "feature not present";
        case VK_ERROR_INCOMPATIBLE_DRIVER:      return "incompatible driver";
        case VK_ERROR_TOO_MANY_OBJECTS:         return "too many objects";
        case VK_ERROR_FORMAT_NOT_SUPPORTED:     return "format not supported";
        case VK_ERROR_FRAGMENTED_POOL:          return "fragmented pool";
#ifdef VK_ERROR_OUT_OF_POOL_MEMORY
        case VK_ERROR_OUT_OF_POOL_MEMORY:       return "out of pool memory";
#endif
#ifdef VK_ERROR_INVALID_EXTERNAL_HANDLE
        case VK_ERROR_INVALID_EXTERNAL_HANDLE:  return "invalid external handle";
#endif
        case VK_ERROR_SURFACE_LOST_KHR:         return "surface lost (KHR)";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return "native window in use (KHR)";
        case VK_SUBOPTIMAL_KHR:                 return "suboptimal (KHR)";
        case VK_ERROR_OUT_OF_DATE_KHR:          return "out of date (KHR)";
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: return "incompatible display (KHR)";
        case VK_ERROR_VALIDATION_FAILED_EXT:    return "validation failed ext";
        case VK_ERROR_INVALID_SHADER_NV:        return "invalid shader nv";
#ifdef VK_ERROR_FRAGMENTATION_EXT
        case VK_ERROR_FRAGMENTATION_EXT:        return "fragmentation ext";
#endif
#ifdef VK_ERROR_NOT_PERMITTED_EXT
        case VK_ERROR_NOT_PERMITTED_EXT:        return "not permitted ext";
#endif
        default:                                return "unknown";
    }
}

uint32_t clip_u32(uint32_t x, uint32_t min, uint32_t max)
{
    if (x < min)
        return min;
    if (x > max)
        return max;
    return x;
}

static const VkBlendFactor vk_blend_factor_map[NGLI_BLEND_FACTOR_NB] = {

    [NGLI_BLEND_FACTOR_ZERO]                = VK_BLEND_FACTOR_ZERO,
    [NGLI_BLEND_FACTOR_ONE]                 = VK_BLEND_FACTOR_ONE,
    [NGLI_BLEND_FACTOR_SRC_COLOR]           = VK_BLEND_FACTOR_SRC_COLOR,
    [NGLI_BLEND_FACTOR_ONE_MINUS_SRC_COLOR] = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
    [NGLI_BLEND_FACTOR_DST_COLOR]           = VK_BLEND_FACTOR_DST_COLOR,
    [NGLI_BLEND_FACTOR_ONE_MINUS_DST_COLOR] = VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
    [NGLI_BLEND_FACTOR_SRC_ALPHA]           = VK_BLEND_FACTOR_SRC_ALPHA,
    [NGLI_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA] = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    [NGLI_BLEND_FACTOR_DST_ALPHA]           = VK_BLEND_FACTOR_DST_ALPHA,
    [NGLI_BLEND_FACTOR_ONE_MINUS_DST_ALPHA] = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
};

const VkBlendFactor get_vk_blend_factor(int blend_factor)
{
    return vk_blend_factor_map[blend_factor];
}

static const VkBlendOp vk_blend_op_map[NGLI_BLEND_OP_NB] = {
    [NGLI_BLEND_OP_ADD]              = VK_BLEND_OP_ADD,
    [NGLI_BLEND_OP_SUBTRACT]         = VK_BLEND_OP_SUBTRACT,
    [NGLI_BLEND_OP_REVERSE_SUBTRACT] = VK_BLEND_OP_REVERSE_SUBTRACT,
    [NGLI_BLEND_OP_MIN]              = VK_BLEND_OP_MIN,
    [NGLI_BLEND_OP_MAX]              = VK_BLEND_OP_MAX,
};

VkBlendOp get_vk_blend_op(int blend_op)
{
    return vk_blend_op_map[blend_op];
}

static const VkCompareOp vk_compare_op_map[NGLI_COMPARE_OP_NB] = {
    [NGLI_COMPARE_OP_NEVER]            = VK_COMPARE_OP_NEVER,
    [NGLI_COMPARE_OP_LESS]             = VK_COMPARE_OP_LESS,
    [NGLI_COMPARE_OP_EQUAL]            = VK_COMPARE_OP_EQUAL,
    [NGLI_COMPARE_OP_LESS_OR_EQUAL]    = VK_COMPARE_OP_LESS_OR_EQUAL,
    [NGLI_COMPARE_OP_GREATER]          = VK_COMPARE_OP_GREATER,
    [NGLI_COMPARE_OP_NOT_EQUAL]        = VK_COMPARE_OP_NOT_EQUAL,
    [NGLI_COMPARE_OP_GREATER_OR_EQUAL] = VK_COMPARE_OP_GREATER_OR_EQUAL,
    [NGLI_COMPARE_OP_ALWAYS]           = VK_COMPARE_OP_ALWAYS,
};

VkCompareOp get_vk_compare_op(int compare_op)
{
    return vk_compare_op_map[compare_op];
}

static const VkStencilOp vk_stencil_op_map[NGLI_STENCIL_OP_NB] = {
    [NGLI_STENCIL_OP_KEEP]                = VK_STENCIL_OP_KEEP,
    [NGLI_STENCIL_OP_ZERO]                = VK_STENCIL_OP_ZERO,
    [NGLI_STENCIL_OP_REPLACE]             = VK_STENCIL_OP_REPLACE,
    [NGLI_STENCIL_OP_INCREMENT_AND_CLAMP] = VK_STENCIL_OP_INCREMENT_AND_CLAMP,
    [NGLI_STENCIL_OP_DECREMENT_AND_CLAMP] = VK_STENCIL_OP_DECREMENT_AND_CLAMP,
    [NGLI_STENCIL_OP_INVERT]              = VK_STENCIL_OP_INVERT,
    [NGLI_STENCIL_OP_INCREMENT_AND_WRAP]  = VK_STENCIL_OP_INCREMENT_AND_WRAP,
    [NGLI_STENCIL_OP_DECREMENT_AND_WRAP]  = VK_STENCIL_OP_DECREMENT_AND_WRAP,
};

VkStencilOp get_vk_stencil_op(int stencil_op)
{
    return vk_stencil_op_map[stencil_op];
}

static const VkCullModeFlags vk_cull_mode_map[NGLI_CULL_MODE_NB] = {
    [NGLI_CULL_MODE_NONE]           = VK_CULL_MODE_NONE,
    [NGLI_CULL_MODE_FRONT_BIT]      = VK_CULL_MODE_FRONT_BIT,
    [NGLI_CULL_MODE_BACK_BIT]       = VK_CULL_MODE_BACK_BIT,
};

VkCullModeFlags get_vk_cull_mode(int cull_mode)
{
    return vk_cull_mode_map[cull_mode];
}

VkColorComponentFlags get_vk_color_write_mask(int color_write_mask)
{
    return (color_write_mask & NGLI_COLOR_COMPONENT_R_BIT ? VK_COLOR_COMPONENT_R_BIT : 0)
         | (color_write_mask & NGLI_COLOR_COMPONENT_G_BIT ? VK_COLOR_COMPONENT_G_BIT : 0)
         | (color_write_mask & NGLI_COLOR_COMPONENT_B_BIT ? VK_COLOR_COMPONENT_B_BIT : 0)
         | (color_write_mask & NGLI_COLOR_COMPONENT_A_BIT ? VK_COLOR_COMPONENT_A_BIT : 0);
}

static const VkDescriptorType descriptor_type_map[] = {
    [NGLI_TYPE_UNIFORM_BUFFER] = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    [NGLI_TYPE_STORAGE_BUFFER] = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
    [NGLI_TYPE_SAMPLER_2D]     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    [NGLI_TYPE_SAMPLER_3D]     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    [NGLI_TYPE_SAMPLER_CUBE]   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    [NGLI_TYPE_IMAGE_2D]       = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
};

VkDescriptorType get_descriptor_type(int type)
{
    ngli_assert(type >= 0 && type < NGLI_TYPE_NB);
    VkDescriptorType descriptor_type = descriptor_type_map[type];
    ngli_assert(descriptor_type);
    return descriptor_type;
}

VkShaderStageFlags get_stage_flags(int flags)
{
    VkShaderStageFlags vk_flags = 0;
    if (flags & NGLI_PROGRAM_SHADER_VERT)
        vk_flags |= VK_SHADER_STAGE_VERTEX_BIT;
    if (flags & NGLI_PROGRAM_SHADER_FRAG)
        vk_flags |= VK_SHADER_STAGE_FRAGMENT_BIT;
    if (flags & NGLI_PROGRAM_SHADER_COMP)
        vk_flags |= VK_SHADER_STAGE_COMPUTE_BIT;
    return vk_flags;
}

static const VkImageLayout image_layout_map[] ={
    [NGLI_TYPE_SAMPLER_2D]     = VK_IMAGE_LAYOUT_GENERAL,
    [NGLI_TYPE_SAMPLER_3D]     = VK_IMAGE_LAYOUT_GENERAL,
    [NGLI_TYPE_SAMPLER_CUBE]   = VK_IMAGE_LAYOUT_GENERAL,
    [NGLI_TYPE_IMAGE_2D]       = VK_IMAGE_LAYOUT_GENERAL,
};

VkImageLayout get_image_layout(int type)
{
    ngli_assert(type >= 0 && type < NGLI_TYPE_NB);
    VkImageLayout image_layout = image_layout_map[type];
    ngli_assert(image_layout);
    return image_layout;
}
