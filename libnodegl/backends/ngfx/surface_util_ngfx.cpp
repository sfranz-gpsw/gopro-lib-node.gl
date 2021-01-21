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

#include "surface_util_ngfx.h"
#include "gctx_ngfx.h"
#include "debugutil_ngfx.h"
#ifdef NGFX_GRAPHICS_BACKEND_VULKAN
#include "ngfx/porting/vulkan/VKGraphicsContext.h"
#if defined(TARGET_LINUX)
#define VK_USE_PLATFORM_XLIB_KHR
#include <X11/Xlib.h>
#include <vulkan/vulkan_xlib.h>
#endif
#endif

ngfx::Surface* surface_util_ngfx::create_offscreen_surface(int w, int h) {
    return new ngfx::Surface(w, h, true);
}

ngfx::Surface* surface_util_ngfx::create_surface_from_window_handle(gctx_ngfx *ctx, const ngl_config *config) {
    ngfx::Surface *surface = nullptr;
#if defined(NGFX_GRAPHICS_BACKEND_VULKAN) and defined(VK_USE_PLATFORM_XLIB_KHR)
    if (config->platform == NGL_PLATFORM_XLIB) {
        ngfx::VKGraphicsContext *vk_ctx = (ngfx::VKGraphicsContext *)ctx->graphics_context;
        ngfx::VKSurface *vk_surface = new ngfx::VKSurface();
        vk_surface->instance = vk_ctx->vkInstance.v;
        VkXlibSurfaceCreateInfoKHR surface_create_info = {
            VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR, NULL, 0,
            (Display *)config->display, config->window,
        };
        vkCreateXlibSurfaceKHR(vk_ctx->vkInstance.v, &surface_create_info, NULL, &vk_surface->v);
        vk_surface->w = config->width;
        vk_surface->h = config->height;
        vk_surface->offscreen = false;
        surface = vk_surface;
    }
#else
    TODO("create logical window surface from window handle");
#endif
    return surface;
}
