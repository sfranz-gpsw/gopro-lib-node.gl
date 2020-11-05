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

#ifndef VKUTILS_H
#define VKUTILS_H

#include <vulkan/vulkan.h>

const char *vk_res2str(VkResult res);

uint32_t clip_u32(uint32_t x, uint32_t min, uint32_t max);

const VkBlendFactor get_vk_blend_factor(int blend_factor);
VkBlendOp get_vk_blend_op(int blend_op);
VkCompareOp get_vk_compare_op(int compare_op);
VkStencilOp get_vk_stencil_op(int stencil_op);
VkCullModeFlags get_vk_cull_mode(int cull_mode);
VkColorComponentFlags get_vk_color_write_mask(int color_write_mask);
VkDescriptorType get_descriptor_type(int type);
VkShaderStageFlags get_stage_flags(int flags);
VkImageLayout get_image_layout(int type);

#endif
