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
#include "gctx_ngfx.h"
#include "buffer_ngfx.h"
#include "gtimer_ngfx.h"
#include "pipeline_ngfx.h"
#include "program_ngfx.h"
#include "rendertarget_ngfx.h"
#include "swapchain_ngfx.h"
#include "texture_ngfx.h"
#include "memory.h"
#include "log.h"
#include "math_utils.h"
#include "format.h"
#include <glm/gtc/type_ptr.hpp>
#ifdef ENABLE_RENDERDOC_CAPTURE
#include "renderdoc_utils.h"
static bool DEBUG_CAPTURE = (getenv("DEBUG_CAPTURE") != nullptr);
#endif
using namespace std;
using namespace ngfx;

static struct gctx *ngfx_create(const struct ngl_config *config)
{
    gctx_ngfx* ctx = new gctx_ngfx;
    return (struct gctx *)ctx;
}

static int ngfx_init(struct gctx *s)
{
    const ngl_config *config = &s->config;
    gctx_ngfx *ctx = (gctx_ngfx *)s;
#ifdef ENABLE_RENDERDOC_CAPTURE
    if (DEBUG_CAPTURE) init_renderdoc();
#endif
    /* FIXME */
    s->features = -1;
    ctx->graphics_context = GraphicsContext::create("NGLApplication", true);
#ifdef ENABLE_RENDERDOC_CAPTURE
    if (DEBUG_CAPTURE) begin_renderdoc_capture(); //&((ngfx::VKGraphicsContext*)ctx->graphics_context)->vkDevice);
#endif
    if (config->offscreen) {
        Surface surface(config->width, config->height, true);
        ctx->graphics_context->setSurface(&surface);
    }
    else {
        TODO("create window surface or use existing handle");
    }
    ctx->graphics = Graphics::create(ctx->graphics_context);
    bool enable_depth_stencil = true;

    if (!config->offscreen) {
        TODO("create window surface");
    }
    else {
        uint32_t size = config->width * config->height * 4;
        ctx->output_texture = ngfx::Texture::create(ctx->graphics_context, ctx->graphics, nullptr, PIXELFORMAT_RGBA8_UNORM, size, config->width, config->height, 1, 1,
                ImageUsageFlags(IMAGE_USAGE_SAMPLED_BIT | IMAGE_USAGE_TRANSFER_SRC_BIT | IMAGE_USAGE_TRANSFER_DST_BIT | IMAGE_USAGE_COLOR_ATTACHMENT_BIT));
        std::vector<ngfx::Framebuffer::Attachment> attachments = { { ctx->output_texture } };
        if (enable_depth_stencil) {
            ctx->depth_texture = ngfx::Texture::create(ctx->graphics_context, ctx->graphics, nullptr, ctx->graphics_context->depthFormat, size, config->width, config->height, 1, 1,
                IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
            attachments.push_back({ ctx->depth_texture });
        }
        ctx->output_framebuffer = Framebuffer::create(ctx->graphics_context->device, ctx->graphics_context->defaultOffscreenRenderPass, attachments, config->width, config->height);
    }

    const int *viewport = config->viewport;
    if (viewport[2] > 0 && viewport[3] > 0) {
        ngli_gctx_set_viewport(s, viewport);
    } else {
        const int default_viewport[] = {0, 0, config->width, config->height};
        ngli_gctx_set_viewport(s, default_viewport);
    }

    const int scissor[] = {0, 0, config->width, config->height};
    ngli_gctx_set_scissor(s, scissor);

    ngli_gctx_set_clear_color(s, config->clear_color);

    if (config->offscreen) {
        ctx->default_rendertarget_desc.nb_colors = 1;
        ctx->default_rendertarget_desc.colors[0].format = NGLI_FORMAT_R8G8B8A8_UNORM;
        ctx->default_rendertarget_desc.colors[0].samples = config->samples;
        ctx->default_rendertarget_desc.colors[0].resolve = config->samples > 0 ? 1 : 0;
        ctx->default_rendertarget_desc.depth_stencil.format = ctx->graphics_context->depthFormat;
        ctx->default_rendertarget_desc.depth_stencil.samples = config->samples;
        ctx->default_rendertarget_desc.depth_stencil.resolve = 0;
    } else {
        ctx->default_rendertarget_desc.nb_colors = 1;
        ctx->default_rendertarget_desc.colors[0].format = NGLI_FORMAT_B8G8R8A8_UNORM;
        ctx->default_rendertarget_desc.colors[0].samples = config->samples;
        ctx->default_rendertarget_desc.colors[0].resolve = config->samples > 0 ? 1 : 0;
        ctx->default_rendertarget_desc.depth_stencil.format = ctx->graphics_context->depthFormat;
        ctx->default_rendertarget_desc.depth_stencil.samples = config->samples;
    }
    return 0;
}

static int ngfx_resize(struct gctx *s, int width, int height, const int *viewport)
{ TODO("w: %d h: %d", width, height);
    return 0;
}

static int ngfx_pre_draw(struct gctx *s, double t)
{
    gctx_ngfx *s_priv = (gctx_ngfx *)s;
    s_priv->cur_command_buffer = s_priv->graphics_context->drawCommandBuffer();
    s_priv->cur_command_buffer->begin();
    s_priv->output_texture->changeLayout(s_priv->cur_command_buffer, IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    return 0;
}

static int ngfx_post_draw(struct gctx *s, double t)
{
    gctx_ngfx *s_priv = (gctx_ngfx *)s;

    ngli_gctx_ngfx_end_render_pass(s);

    s_priv->cur_command_buffer->end();
    s_priv->graphics_context->queue->submit(s_priv->cur_command_buffer);
    if (s->config.offscreen && s->config.capture_buffer) {
        uint32_t size = s->config.width * s->config.height * 4;
        s_priv->output_texture->download(s->config.capture_buffer, size);
    }
    return 0;
}

static void ngfx_wait_idle(struct gctx *s)
{
    gctx_ngfx *s_priv = (gctx_ngfx *)s;
    s_priv->graphics->waitIdle(s_priv->cur_command_buffer);
}

static void ngfx_destroy(struct gctx *s)
{
    gctx_ngfx* ctx = new gctx_ngfx;
    if (ctx->output_framebuffer) delete ctx->output_framebuffer;
    if (ctx->depth_texture) delete ctx->depth_texture;
    if (ctx->output_texture) delete ctx->output_texture;
    delete ctx->graphics;
    delete ctx->graphics_context;
    delete ctx;
#ifdef ENABLE_RENDERDOC_CAPTURE
    if (DEBUG_CAPTURE) end_renderdoc_capture();
#endif
}

static int ngfx_transform_cull_mode(struct gctx *s, int cull_mode)
{ TODO();
    return 0;
}

static void ngfx_transform_projection_matrix(struct gctx *s, float *dst)
{
#ifdef GRAPHICS_BACKEND_VULKAN
    static const NGLI_ALIGNED_MAT(matrix) = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f,-1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.5f, 0.0f,
        0.0f, 0.0f, 0.5f, 1.0f,
    };
    ngli_mat4_mul(dst, matrix, dst);
#endif
}

static void ngfx_get_rendertarget_uvcoord_matrix(struct gctx *s, float *dst)
{ TODO();

}

static void ngfx_set_rendertarget(struct gctx *s, struct rendertarget *rt)
{ TODO();

}

static struct rendertarget *ngfx_get_rendertarget(struct gctx *s)
{ TODO();
    return NULL;
}

static const struct rendertarget_desc *ngfx_get_default_rendertarget_desc(struct gctx *s)
{
    struct gctx_ngfx *s_priv = (struct gctx_ngfx *)s;
    return &s_priv->default_rendertarget_desc;
}

static void ngfx_set_viewport(struct gctx *s, const int *viewport)
{
    struct gctx_ngfx *s_priv = (struct gctx_ngfx *)s;
    memcpy(s_priv->viewport, viewport, sizeof(s_priv->viewport));
}

static void ngfx_get_viewport(struct gctx *s, int *viewport)
{
    struct gctx_ngfx *s_priv = (struct gctx_ngfx *)s;
    memcpy(viewport, &s_priv->viewport, sizeof(s_priv->viewport));
}

static void ngfx_set_scissor(struct gctx *s, const int *scissor)
{
    struct gctx_ngfx *s_priv = (struct gctx_ngfx *)s;
    memcpy(&s_priv->scissor, scissor, sizeof(s_priv->scissor));
}

static void ngfx_get_scissor(struct gctx *s, int *scissor)
{
    struct gctx_ngfx *s_priv = (struct gctx_ngfx *)s;
    memcpy(scissor, &s_priv->scissor, sizeof(s_priv->scissor));
}

static void ngfx_set_clear_color(struct gctx *s, const float *color)
{
    struct gctx_ngfx *s_priv = (struct gctx_ngfx *)s;
    memcpy(s_priv->clear_color, color, sizeof(s_priv->clear_color));
}

static void ngfx_get_clear_color(struct gctx *s, float *color)
{
    struct gctx_ngfx *s_priv = (struct gctx_ngfx *)s;
    memcpy(color, &s_priv->clear_color, sizeof(s_priv->clear_color));
}

static void ngfx_clear_color(struct gctx *s)
{ TODO();

}

static void ngfx_clear_depth_stencil(struct gctx *s)
{ TODO();

}

static void ngfx_invalidate_depth_stencil(struct gctx *s)
{ TODO();

}

static void ngfx_flush(struct gctx *s)
{ TODO();

}

static int ngfx_get_preferred_depth_format(struct gctx *s)
{ TODO();
    return 0;
}

static int ngfx_get_preferred_depth_stencil_format(struct gctx *s)
{ TODO();
    return 0;
}

void ngli_gctx_ngfx_begin_render_pass(struct gctx *s)
{
    gctx_ngfx *s_priv = (struct gctx_ngfx *)s;

    if (s_priv->render_pass_state == 1)
        return;

    Graphics *graphics = s_priv->graphics;
    CommandBuffer *cmd_buf = s_priv->cur_command_buffer;
    TODO("set correct render pass and framebuffer");
    RenderPass *render_pass = s_priv->graphics_context->defaultOffscreenRenderPass;
    Framebuffer *framebuffer = s_priv->output_framebuffer;
    graphics->beginRenderPass(cmd_buf, render_pass, framebuffer, glm::make_vec4(s_priv->clear_color));
    int* vp = s_priv->viewport;
    graphics->setViewport(cmd_buf, { vp[0], vp[1], uint32_t(vp[2]), uint32_t(vp[3]) });
    int *sr = s_priv->scissor;
    graphics->setScissor(cmd_buf, { sr[0], sr[1], uint32_t(sr[2]), uint32_t(sr[3]) });

    s_priv->render_pass_state = 1;
}

void ngli_gctx_ngfx_end_render_pass(struct gctx *s)
{
    gctx_ngfx *s_priv = (gctx_ngfx *)s;
    Graphics *graphics = s_priv->graphics;
    CommandBuffer *cmd_buf = s_priv->cur_command_buffer;

    if (s_priv->render_pass_state != 1)
        return;

    graphics->endRenderPass(cmd_buf);

    s_priv->render_pass_state = 0;
}

extern "C" const struct gctx_class ngli_gctx_ngfx = {
    .name         = "NGFX",
    .create       = ngfx_create,
    .init         = ngfx_init,
    .resize       = ngfx_resize,
    .pre_draw     = ngfx_pre_draw,
    .post_draw    = ngfx_post_draw,
    .wait_idle    = ngfx_wait_idle,
    .destroy      = ngfx_destroy,

    .transform_cull_mode = ngfx_transform_cull_mode,
    .transform_projection_matrix = ngfx_transform_projection_matrix,
    .get_rendertarget_uvcoord_matrix = ngfx_get_rendertarget_uvcoord_matrix,

    .set_rendertarget         = ngfx_set_rendertarget,
    .get_rendertarget         = ngfx_get_rendertarget,
    .get_default_rendertarget_desc = ngfx_get_default_rendertarget_desc,
    .set_viewport             = ngfx_set_viewport,
    .get_viewport             = ngfx_get_viewport,
    .set_scissor              = ngfx_set_scissor,
    .get_scissor              = ngfx_get_scissor,
    .set_clear_color          = ngfx_set_clear_color,
    .get_clear_color          = ngfx_get_clear_color,
    .clear_color              = ngfx_clear_color,
    .clear_depth_stencil      = ngfx_clear_depth_stencil,
    .invalidate_depth_stencil = ngfx_invalidate_depth_stencil,
    .get_preferred_depth_format= ngfx_get_preferred_depth_format,
    .get_preferred_depth_stencil_format=ngfx_get_preferred_depth_stencil_format,
    .flush                    = ngfx_flush,

    .buffer_create = ngli_buffer_ngfx_create,
    .buffer_init   = ngli_buffer_ngfx_init,
    .buffer_upload = ngli_buffer_ngfx_upload,
    .buffer_download = ngli_buffer_ngfx_download,
    .buffer_map      = ngli_buffer_ngfx_map,
    .buffer_unmap    = ngli_buffer_ngfx_unmap,
    .buffer_freep  = ngli_buffer_ngfx_freep,

    .gtimer_create = ngli_gtimer_ngfx_create,
    .gtimer_init   = ngli_gtimer_ngfx_init,
    .gtimer_start  = ngli_gtimer_ngfx_start,
    .gtimer_stop   = ngli_gtimer_ngfx_stop,
    .gtimer_read   = ngli_gtimer_ngfx_read,
    .gtimer_freep  = ngli_gtimer_ngfx_freep,

    .pipeline_create         = ngli_pipeline_ngfx_create,
    .pipeline_init           = ngli_pipeline_ngfx_init,
    .pipeline_bind_resources = ngli_pipeline_ngfx_bind_resources,
    .pipeline_update_attribute = ngli_pipeline_ngfx_update_attribute,
    .pipeline_update_uniform = ngli_pipeline_ngfx_update_uniform,
    .pipeline_update_texture = ngli_pipeline_ngfx_update_texture,
    .pipeline_draw           = ngli_pipeline_ngfx_draw,
    .pipeline_draw_indexed   = ngli_pipeline_ngfx_draw_indexed,
    .pipeline_dispatch       = ngli_pipeline_ngfx_dispatch,
    .pipeline_freep          = ngli_pipeline_ngfx_freep,

    .program_create = ngli_program_ngfx_create,
    .program_init   = ngli_program_ngfx_init,
    .program_freep  = ngli_program_ngfx_freep,

    .rendertarget_create      = ngli_rendertarget_ngfx_create,
    .rendertarget_init        = ngli_rendertarget_ngfx_init,
    .rendertarget_resolve     = ngli_rendertarget_ngfx_resolve,
    .rendertarget_read_pixels = ngli_rendertarget_ngfx_read_pixels,
    .rendertarget_freep       = ngli_rendertarget_ngfx_freep,

    .swapchain_create         = ngli_swapchain_ngfx_create,
    .swapchain_destroy        = ngli_swapchain_ngfx_destroy,
    .swapchain_acquire_image  = ngli_swapchain_ngfx_acquire_image,

    .texture_create           = ngli_texture_ngfx_create,
    .texture_init             = ngli_texture_ngfx_init,
    .texture_has_mipmap       = ngli_texture_ngfx_has_mipmap,
    .texture_upload           = ngli_texture_ngfx_upload,
    .texture_generate_mipmap  = ngli_texture_ngfx_generate_mipmap,
    .texture_freep            = ngli_texture_ngfx_freep,
};
