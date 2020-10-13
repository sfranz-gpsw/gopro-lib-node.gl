/*
 * Copyright 2018 GoPro Inc.
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

#include "rendertarget_ngfx.h"
#include <glm/gtc/type_ptr.hpp>
#include "memory.h"
#include "gctx_ngfx.h"
#include "texture_ngfx.h"
#include "util_ngfx.h"
#include <vector>
using namespace ngfx;

struct rendertarget *ngli_rendertarget_ngfx_create(struct gctx *gctx) {
    rendertarget_ngfx *s = new rendertarget_ngfx;
    if (!s)
        return NULL;
    s->parent.gctx = gctx;
    return (struct rendertarget *)s;
}
int ngli_rendertarget_ngfx_init(struct rendertarget *s, const struct rendertarget_params *params) {
    rendertarget_ngfx *s_priv = (rendertarget_ngfx *)s;
    gctx_ngfx *gctx = (gctx_ngfx *)s->gctx;
    auto &ctx = gctx->graphics_context;

    rendertarget_desc rt_desc = {};
    std::vector<Framebuffer::Attachment> attachments;
    uint32_t w = 0, h = 0;
    for (int i = 0; i < params->nb_colors; i++) {
        const attachment *color_attachment = &params->colors[i];
        const texture_ngfx *color_texture = (const texture_ngfx *)color_attachment->attachment;
        const texture_params *color_texture_params = &color_texture->parent.params;
        rt_desc.colors[rt_desc.nb_colors].format = color_texture_params->format;
        rt_desc.colors[rt_desc.nb_colors].samples = color_texture_params->samples;
        rt_desc.colors[rt_desc.nb_colors].resolve = color_attachment->resolve_target != NULL;
        rt_desc.nb_colors++;
        if (i == 0) { w = color_texture->v->w; h = color_texture->v->h; }
        attachments.push_back({ color_texture->v, 0, uint32_t(i) });
    }

    const attachment *depth_attachment = &params->depth_stencil;
    const texture_ngfx *depth_texture = (const texture_ngfx *)depth_attachment->attachment;
    if (depth_texture) {
        const texture_params *depth_texture_params = &depth_texture->parent.params;
        rt_desc.depth_stencil.format = depth_texture_params->format;
        rt_desc.depth_stencil.samples = depth_texture_params->samples;
        rt_desc.depth_stencil.resolve = depth_attachment->resolve_target != NULL;
        attachments.push_back({ depth_texture->v });
    }

    s_priv->render_pass = get_render_pass(ctx, rt_desc);

    s_priv->output_framebuffer = Framebuffer::create(ctx->device, s_priv->render_pass, attachments, w, h);

    s_priv->parent.width = w;
    s_priv->parent.height = h;

    return 0;
}
void ngli_rendertarget_ngfx_resolve(struct rendertarget *s) {

}
void ngli_rendertarget_ngfx_read_pixels(struct rendertarget *s, uint8_t *data) {

}

static void begin_render_pass(rendertarget_ngfx *thiz, struct gctx *s)
{
    gctx_ngfx *s_priv = (struct gctx_ngfx *)s;

    Graphics *graphics = s_priv->graphics;
    CommandBuffer *cmd_buf = s_priv->cur_command_buffer;
    RenderPass *render_pass = thiz->render_pass;

    Framebuffer *framebuffer = thiz->output_framebuffer;
    graphics->beginRenderPass(cmd_buf, render_pass, framebuffer, glm::make_vec4(s_priv->clear_color));
    int vp[4];
    ngli_gctx_get_viewport(s, vp);
    graphics->setViewport(cmd_buf, { vp[0], vp[1], uint32_t(vp[2]), uint32_t(vp[3]) });
    int sr[4];
    ngli_gctx_get_scissor(s, sr);
    graphics->setScissor(cmd_buf, { sr[0], sr[1], uint32_t(sr[2]), uint32_t(sr[3]) });
}

static void end_render_pass(rendertarget_ngfx *thiz, struct gctx *s)
{
    gctx_ngfx *s_priv = (gctx_ngfx *)s;
    Graphics *graphics = s_priv->graphics;
    CommandBuffer *cmd_buf = s_priv->cur_command_buffer;

    graphics->endRenderPass(cmd_buf);
}

void ngli_rendertarget_ngfx_begin_pass(struct rendertarget *s) {
    rendertarget_ngfx *s_priv = (rendertarget_ngfx *)s;
    gctx_ngfx *ctx = (gctx_ngfx *)s_priv->parent.gctx;
    auto output_texture = s_priv->output_framebuffer->attachments[0].texture;
    output_texture->changeLayout(ctx->cur_command_buffer, IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    begin_render_pass((rendertarget_ngfx *)s, s->gctx);
}
void ngli_rendertarget_ngfx_end_pass(struct rendertarget *s) {
    end_render_pass((rendertarget_ngfx *)s, s->gctx);
    rendertarget_ngfx *s_priv = (rendertarget_ngfx *)s;
    gctx_ngfx *ctx = (gctx_ngfx *)s_priv->parent.gctx;
    auto output_texture = s_priv->output_framebuffer->attachments[0].texture;
    if (output_texture->imageUsageFlags & IMAGE_USAGE_SAMPLED_BIT) {
        output_texture->changeLayout(ctx->cur_command_buffer, IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
}


void ngli_rendertarget_ngfx_freep(struct rendertarget **sp) {
    if (!*sp)
        return;
    rendertarget_ngfx *s_priv = (rendertarget_ngfx *)*sp;
    if (s_priv->output_framebuffer) delete s_priv->output_framebuffer;
    delete s_priv;
}
