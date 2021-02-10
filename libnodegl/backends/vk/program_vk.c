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

#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "memory.h"
#include "nodes.h"
#include "program_vk.h"
#include "gctx_vk.h"

struct program *ngli_program_vk_create(struct gctx *gctx)
{
    struct program_vk *s = ngli_calloc(1, sizeof(*s));
    if (!s)
        return NULL;
    s->parent.gctx = gctx;
    return (struct program *)s;
}

int ngli_program_vk_init(struct program *s, const char *vertex, const char *fragment, const char *compute)
{
    struct gctx_vk *gctx_vk = (struct gctx_vk *)s->gctx;
    struct vkcontext *vk = gctx_vk->vkcontext;
    struct program_vk *s_priv = (struct program_vk *)s;

    struct {
        shaderc_shader_kind kind;
        const char *src;
    } shaders[] = {
        [NGLI_PROGRAM_SHADER_VERT] = {shaderc_glsl_vertex_shader,   vertex,},
        [NGLI_PROGRAM_SHADER_FRAG] = {shaderc_glsl_fragment_shader, fragment},
        [NGLI_PROGRAM_SHADER_COMP] = {shaderc_glsl_compute_shader,  compute},
    };

    for (int i = 0; i < NGLI_ARRAY_NB(s_priv->shaders); i++) {
        if (!shaders[i].src)
            continue;

        shaderc_compilation_result_t result = shaderc_compile_into_spv(gctx_vk->spirv_compiler,
                                                                       shaders[i].src, strlen(shaders[i].src),
                                                                       shaders[i].kind,
                                                                       "whatever",
                                                                       "main",
                                                                       gctx_vk->spirv_compiler_opts);

        shaderc_compilation_status status = shaderc_result_get_compilation_status(result);
        if (status != shaderc_compilation_status_success) {
            LOG(ERROR, "unable to compile shader: %s", shaderc_result_get_error_message(result));
            char *s_with_numbers = ngli_numbered_lines(shaders[i].src);
            if (s_with_numbers) {
                LOG(ERROR, "%s", s_with_numbers);
                ngli_free(s_with_numbers);
            }
            shaderc_result_release(result);
            return NGL_ERROR_EXTERNAL;
        }

        const uint32_t *code = (const uint32_t *)shaderc_result_get_bytes(result);
        const size_t code_size = shaderc_result_get_length(result);
        VkShaderModuleCreateInfo shader_module_create_info = {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = code_size,
            .pCode = code,
        };
        VkResult res = vkCreateShaderModule(vk->device, &shader_module_create_info, NULL, &s_priv->shaders[i]);

        shaderc_result_release(result);

        if (res != VK_SUCCESS)
            return NGL_ERROR_EXTERNAL;
    }

    return 0;
}

void ngli_program_vk_freep(struct program **sp)
{
    if (!*sp)
        return;
    struct program *s = *sp;
    struct program_vk *s_priv = (struct program_vk *)s;
    struct gctx_vk *gctx_vk = (struct gctx_vk *)s->gctx;
    struct vkcontext *vk = gctx_vk->vkcontext;
    for (int i = 0; i < NGLI_ARRAY_NB(s_priv->shaders); i++)
        vkDestroyShaderModule(vk->device, s_priv->shaders[i], NULL);
    ngli_freep(sp);
}
