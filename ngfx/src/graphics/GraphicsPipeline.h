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
#include "graphics/Pipeline.h"
#include "graphics/CommandBuffer.h"
#include "graphics/GraphicsCore.h"
#include "graphics/RenderPass.h"
#include "graphics/ShaderModule.h"
#include "graphics/Config.h"
#include <vector>
#include <set>

namespace ngfx {
    class GraphicsContext;
    class GraphicsPipeline : public Pipeline {
    public:
        struct State {
            PrimitiveTopology primitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            PolygonMode polygonMode = POLYGON_MODE_FILL;
            bool blendEnable = false;
            BlendFactor blendSrcFactor = BLEND_FACTOR_SRC_ALPHA;
            BlendFactor blendDstFactor = BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            BlendOp blendOp = BLEND_OP_ADD;
            ColorComponentFlags colorWriteMask =
                COLOR_COMPONENT_R_BIT |
                COLOR_COMPONENT_G_BIT |
                COLOR_COMPONENT_B_BIT |
                COLOR_COMPONENT_A_BIT;
            CullModeFlags cullModeFlags = CULL_MODE_BACK_BIT;
            FrontFace frontFace = FRONT_FACE_COUNTER_CLOCKWISE;
            float lineWidth = 1.0f;
            bool depthTestEnable = false, depthWriteEnable = false;
            RenderPass* renderPass = nullptr;
            uint32_t numSamples = 1, numColorAttachments = 1;
        };
        struct Descriptor {
            DescriptorType type;
            ShaderStageFlags stageFlags = SHADER_STAGE_ALL;
        };
        static GraphicsPipeline* create(GraphicsContext* graphicsContext, const State &state,
            VertexShaderModule* vs, FragmentShaderModule* fs, PixelFormat colorFormat, PixelFormat depthFormat,
            std::set<std::string> instanceAttributes = {});
        virtual ~GraphicsPipeline() {}
        void getBindings(std::vector<uint32_t*> pDescriptorBindings, std::vector<uint32_t*> pVertexAttribBindings);
        std::vector<uint32_t> descriptorBindings, vertexAttributeBindings;
    };
};
