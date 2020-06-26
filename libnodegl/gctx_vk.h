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

#ifndef GCTX_VK_H
#define GCTX_VK_H

#include "gctx.h"
#include "vkcontext.h"

struct gctx_vk {
    struct gctx parent;
    struct vkcontext *vkcontext;

    shaderc_compiler_t spirv_compiler;
    shaderc_compile_options_t spirv_compiler_opts;

    struct vkcontext *xxx;

    VkExtent2D extent;

    uint32_t img_index;

    int nb_in_flight_frames;
    int current_frame;

    VkCommandPool transient_command_buffer_pool;
    VkFence transient_command_buffer_fence;

    VkCommandPool command_buffer_pool;
    VkCommandBuffer *command_buffers;
    VkCommandBuffer cur_command_buffer;
    int cur_command_buffer_state;

    struct texture **wrapped_textures;
    struct texture **textures;
    struct texture **depth_textures;
    struct rendertarget **rts;
    uint32_t nb_wrapped_textures;

    VkImage *images;
    uint32_t nb_images;

    VkSemaphore *sem_img_avail;
    VkSemaphore *sem_render_finished;
    VkFence *fences;
    VkStructureType surface_create_type;

    struct texture *rt_color;
    struct texture *rt_depth_stencil;
    struct texture *rt_resolve_color;
    struct rendertarget *rt;

    VkFramebuffer cur_framebuffer;
    VkRenderPass cur_render_pass;
    VkExtent2D cur_render_area;
    int cur_render_pass_state;
    struct darray wait_semaphores;
    struct darray wait_stages;
    struct darray signal_semaphores;

    struct rendertarget *rendertarget;
    struct rendertarget_desc default_rendertarget_desc;
    int viewport[4];
    int scissor[4];
    float clear_color[4];
};

void ngli_gctx_vk_commit_render_pass(struct gctx *s);
void ngli_gctx_vk_end_render_pass(struct gctx *s);
int ngli_gctx_vk_begin_transient_command(struct gctx *s, VkCommandBuffer *command_buffer);
int ngli_gctx_vk_execute_transient_command(struct gctx *s, VkCommandBuffer command_buffer);

#endif
