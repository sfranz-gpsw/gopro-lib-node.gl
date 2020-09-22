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
#include "porting/vulkan/VKRenderPass.h"
#include "porting/vulkan/VKDebugUtil.h"
#include "porting/vulkan/VKTexture.h"
#include "porting/vulkan/VKCommandBuffer.h"
#include <vector>
using namespace ngfx;

void VKRenderPass::create(VkDevice device,
        const std::vector<VkAttachmentDescription> &attachments,
        const std::vector<VkSubpassDescription> &subpasses,
        const std::vector<VkSubpassDependency>& dependencies) {
    VkResult vkResult;
    this->device = device;
    createInfo = {
            VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            nullptr, 0,
            uint32_t(attachments.size()), attachments.data(),
            uint32_t(subpasses.size()), subpasses.data(),
            uint32_t(dependencies.size()), dependencies.data()
    };
    V(vkCreateRenderPass(device, &createInfo, nullptr, &v));
}

VKRenderPass::~VKRenderPass() {
    if (v) VK_TRACE(vkDestroyRenderPass(device, v, nullptr));
}
