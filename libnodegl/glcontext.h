/*
 * Copyright 2016 GoPro Inc.
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

#ifndef GLCONTEXT_H
#define GLCONTEXT_H

#include "glfunctions.h"
#include "glwrappers.h"
#include "nodegl.h"

#define NGLI_FEATURE_VERTEX_ARRAY_OBJECT          (1 << 0)
#define NGLI_FEATURE_TEXTURE_3D                   (1 << 1)
#define NGLI_FEATURE_TEXTURE_STORAGE              (1 << 2)
#define NGLI_FEATURE_COMPUTE_SHADER               (1 << 3)
#define NGLI_FEATURE_PROGRAM_INTERFACE_QUERY      (1 << 4)
#define NGLI_FEATURE_SHADER_IMAGE_LOAD_STORE      (1 << 5)
#define NGLI_FEATURE_SHADER_STORAGE_BUFFER_OBJECT (1 << 6)
#define NGLI_FEATURE_FRAMEBUFFER_OBJECT           (1 << 7)
#define NGLI_FEATURE_INTERNALFORMAT_QUERY         (1 << 8)
#define NGLI_FEATURE_PACKED_DEPTH_STENCIL         (1 << 9)

#define NGLI_FEATURE_COMPUTE_SHADER_ALL (NGLI_FEATURE_COMPUTE_SHADER           | \
                                         NGLI_FEATURE_PROGRAM_INTERFACE_QUERY  | \
                                         NGLI_FEATURE_SHADER_IMAGE_LOAD_STORE  | \
                                         NGLI_FEATURE_SHADER_STORAGE_BUFFER_OBJECT)

struct glcontext_class;

struct glcontext {
    /* GL context */
    const struct glcontext_class *class;
    void *priv_data;

    /* User options */
    int platform;
    int api;
    int wrapped;
    int offscreen;
    int width;
    int height;
    int samples;
    int set_surface_pts;

    /* GL api */
    int major_version;
    int minor_version;

    /* GL features */
    int features;
    int max_texture_image_units;
    int max_compute_work_group_counts[3];

    GLenum gl_1comp;
    GLenum gl_2comp;

    /* GL functions */
    struct glfunctions funcs;
};

struct glcontext_class {
    int (*init)(struct glcontext *glcontext, uintptr_t display, uintptr_t window, uintptr_t handle);
    int (*create)(struct glcontext *glcontext, uintptr_t other);
    int (*make_current)(struct glcontext *glcontext, int current);
    void (*swap_buffers)(struct glcontext *glcontext);
    void (*set_surface_pts)(struct glcontext *glcontext, double t);
    void* (*get_texture_cache)(struct glcontext *glcontext);
    void* (*get_proc_address)(struct glcontext *glcontext, const char *name);
    void (*uninit)(struct glcontext *glcontext);
    size_t priv_size;
};

struct glcontext *ngli_glcontext_new(struct ngl_config *config);
int ngli_glcontext_load_extensions(struct glcontext *glcontext);
int ngli_glcontext_make_current(struct glcontext *glcontext, int current);
void ngli_glcontext_swap_buffers(struct glcontext *glcontext);
void ngli_glcontext_set_surface_pts(struct glcontext *glcontext, double t);
void *ngli_glcontext_get_proc_address(struct glcontext *glcontext, const char *name);
void *ngli_glcontext_get_texture_cache(struct glcontext *glcontext);
void ngli_glcontext_freep(struct glcontext **glcontext);
int ngli_glcontext_check_extension(const char *extension, const char *extensions);
int ngli_glcontext_check_gl_error(struct glcontext *glcontext);

#endif /* GLCONTEXT_H */
