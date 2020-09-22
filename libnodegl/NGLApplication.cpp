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

#include "NGLApplication.h"
#include "graphics/ShaderModule.h"
#include <memory>
#include <glm/gtx/string_cast.hpp>
#include "nodegl.h"
using namespace NGL;

NGLApplication::NGLApplication(int w, int h, bool enableDepthStencil, bool offscreen)
    : Application("application_ngfx", w, h, enableDepthStencil, offscreen)  {
    this->persistentCommandBuffers = false;
}

void NGLApplication::init() {
    if (cfg) {
        w = cfg->width; h = cfg->height;
        offscreen = cfg->offscreen;
    }
    Application::init();
}

void NGLApplication::createWindow() {
    if (cfg && cfg->windowHandle) {
        window.reset(new NGLWindowWrapper(
            graphicsContext.get(), cfg->width, cfg->height,
            cfg->displayHandle, cfg->windowHandle));
        graphicsContext->setSurface(window->surface);
    } else {
        Application::createWindow();
    }
}

void NGLApplication::onInit() {
    auto &ctx = graphicsContext;
    if (cfg) ctx->clearColor = cfg->clearColor;
    GraphicsState state = {};
    state.colorFormat = ctx->surfaceFormat;
    state.depthFormat = ctx->depthFormat;
    scene->init(ctx.get(), graphics.get(), state);
}

void NGLApplication::onRecordCommandBuffer(CommandBuffer* commandBuffer) {
    auto& ctx = graphicsContext;
    GraphicsState state;

    graphics->beginComputePass(commandBuffer);
    scene->compute(commandBuffer, graphics.get(), state);
    graphics->endComputePass(commandBuffer);

    state.rttPass = true;
    scene->draw(commandBuffer, graphics.get(), state);
    if (!offscreen) ctx->beginRenderPass(commandBuffer, graphics.get());
    else {
        outputTexture->changeLayout(commandBuffer, IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        ctx->beginOffscreenRenderPass(commandBuffer, graphics.get(), outputFramebuffer.get());
    }

    state.rttPass = false;
    scene->draw(commandBuffer, graphics.get(), state);
    if (!offscreen) ctx->endRenderPass(commandBuffer, graphics.get());
    else {
        ctx->endOffscreenRenderPass(commandBuffer, graphics.get());
        outputTexture->changeLayout(commandBuffer, IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
}

void NGLApplication::run() {
    useInternalTimer = true;
    Application::run();
}
void NGLApplication::onUpdate() {
    if (useInternalTimer) {
        auto nglCtx = scene->thiz->ctx;
        timer.update();
        nglCtx->time += timer.elapsed;
    }
    scene->update();
}

void NGLApplication::onPaint() {
    paint();
    if (offscreen && cfg && cfg->captureBuffer) {
        uint32_t size = w * h * 4;
        outputTexture->download(cfg->captureBuffer, size);
    }
}
