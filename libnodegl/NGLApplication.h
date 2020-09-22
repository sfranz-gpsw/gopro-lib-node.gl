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

#pragma once
#include "Application.h"
#include "Timer.h"
#include <memory>
using namespace ngfx;

struct ngl_config;

namespace NGL {
    class NGLApplication : public Application {
    public:
        NGLApplication(int w = Window::DISPLAY_WIDTH, int h = Window::DISPLAY_HEIGHT,
            bool enableDepthStencil = true, bool offscreen = false);
        virtual ~NGLApplication() {}
        void init() override;
        void createWindow() override;
        void onInit() override;
        void onRecordCommandBuffer(CommandBuffer* commandBuffer) override;
        void onUpdate() override;
        void onPaint() override;
        void run() override;
        void* scene = nullptr;
        struct ngl_config* cfg = nullptr;
    protected:
        bool useInternalTimer = false;
        Timer timer;
    };
}
