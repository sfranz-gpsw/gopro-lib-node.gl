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

#include <string.h>

#include "gctx.h"
#include "log.h"
#include "memory.h"
#include "nodes.h"
#include "rendertarget.h"

extern const struct gctx_clazz ngli_gctx_gl;
extern const struct gctx_clazz ngli_gctx_gles;
extern const struct gctx_clazz ngli_gctx_vk;

static const struct {
    const char *string_id;
    const struct gctx_clazz *cls;
} backend_map[] = {
    [NGL_BACKEND_OPENGL] = {
        .string_id = "opengl",
#ifdef BACKEND_GL
        .cls = &ngli_gctx_gl,
#endif
    },
    [NGL_BACKEND_OPENGLES] = {
        .string_id = "opengles",
#ifdef BACKEND_GL
        .cls = &ngli_gctx_gles,
#endif
    },
    [NGL_BACKEND_VULKAN] = {
        .string_id = "vulkan",
#ifdef BACKEND_VK
        .cls = &ngli_gctx_vk,
#endif
    },
};

struct gctx *ngli_gctx_create(const struct ngl_config *config)
{
    if (config->backend < 0 ||
        config->backend >= NGLI_ARRAY_NB(backend_map)) {
        LOG(ERROR, "unknown backend %d", config->backend);
        return NULL;
    }
    if (!backend_map[config->backend].cls) {
        LOG(ERROR, "backend \"%s\" not available with this build",
            backend_map[config->backend].string_id);
        return NULL;
    }
    const struct gctx_class *clazz = backend_map[config->backend].cls;
    struct gctx *s = clazz->create(config);
    if (!s)
        return NULL;
    s->config = *config;
    s->backend_str = backend_map[config->backend].string_id;
    s->clazz = clazz;
    return s;
}

int ngli_gctx_init(struct gctx *s)
{
    return s->clazz->init(s);
}

int ngli_gctx_resize(struct gctx *s, int width, int height, const int *viewport)
{
    const struct gctx_class *clazz = s->clazz;
    return clazz->resize(s, width, height, viewport);
}

int ngli_gctx_set_capture_buffer(struct gctx *s, void *capture_buffer)
{
    const struct gctx_class *clazz = s->clazz;
    return clazz->set_capture_buffer(s, capture_buffer);
}

int ngli_gctx_update(struct gctx *s, struct ngl_node *scene, double t)
{
    const struct gctx_class *clazz = s->clazz;

    int ret = clazz->begin_update(s, t);
    if (ret < 0)
        return ret;

    ret = ngli_node_update(scene, t);
    if (ret < 0)
        return ret;

    return clazz->end_update(s, t);
}

int ngli_gctx_begin_draw(struct gctx *s, double t)
{
    return s->clazz->begin_draw(s, t);
}

int ngli_gctx_end_draw(struct gctx *s, double t)
{
    return s->clazz->end_draw(s, t);
}

int ngli_gctx_query_draw_time(struct gctx *s, int64_t *time)
{
    return s->clazz->query_draw_time(s, time);
}

void ngli_gctx_wait_idle(struct gctx *s)
{
    s->clazz->wait_idle(s);
}

void ngli_gctx_freep(struct gctx **sp)
{
    if (!*sp)
        return;

    struct gctx *s = *sp;
    const struct gctx_class *clazz = s->clazz;
    if (clazz)
        clazz->destroy(s);

    ngli_freep(sp);
}

int ngli_gctx_transform_cull_mode(struct gctx *s, int cull_mode)
{
    return s->clazz->transform_cull_mode(s, cull_mode);
}

void ngli_gctx_transform_projection_matrix(struct gctx *s, float *dst)
{
    s->clazz->transform_projection_matrix(s, dst);
}

void ngli_gctx_begin_render_pass(struct gctx *s, struct rendertarget *rt)
{
    s->clazz->begin_render_pass(s, rt);
}

void ngli_gctx_end_render_pass(struct gctx *s)
{
    s->clazz->end_render_pass(s);
}

void ngli_gctx_get_rendertarget_uvcoord_matrix(struct gctx *s, float *dst)
{
    s->clazz->get_rendertarget_uvcoord_matrix(s, dst);
}

struct rendertarget *ngli_gctx_get_default_rendertarget(struct gctx *s)
{
    return s->clazz->get_default_rendertarget(s);
}

const struct rendertarget_desc *ngli_gctx_get_default_rendertarget_desc(struct gctx *s)
{
    return s->clazz->get_default_rendertarget_desc(s);
}

void ngli_gctx_set_viewport(struct gctx *s, const int *viewport)
{
    s->clazz->set_viewport(s, viewport);
}

void ngli_gctx_get_viewport(struct gctx *s, int *viewport)
{
    s->clazz->get_viewport(s, viewport);
}

void ngli_gctx_set_scissor(struct gctx *s, const int *scissor)
{
    s->clazz->set_scissor(s, scissor);
}

void ngli_gctx_get_scissor(struct gctx *s, int *scissor)
{
    s->clazz->get_scissor(s, scissor);
}

int ngli_gctx_get_preferred_depth_format(struct gctx *s)
{
    return s->clazz->get_preferred_depth_format(s);
}

int ngli_gctx_get_preferred_depth_stencil_format(struct gctx *s)
{
    return s->clazz->get_preferred_depth_stencil_format(s);
}

void ngli_gctx_flush(struct gctx *s)
{
    s->clazz->flush(s);
}
