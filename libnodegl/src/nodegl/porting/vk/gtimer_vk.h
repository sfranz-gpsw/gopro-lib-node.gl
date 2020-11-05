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

#ifndef GTIMER_VK_H
#define GTIMER_VK_H

#include <vulkan/vulkan.h>
#include "nodegl/core/gtimer.h"

struct gctx;

struct gtimer_vk {
    struct gtimer parent;
    //VkCommandPool command_pool;
    VkQueryPool query_pool;
    uint64_t results[2];
};

struct gtimer *ngli_gtimer_vk_create(struct gctx *gctx);
int ngli_gtimer_vk_init(struct gtimer *s);
int ngli_gtimer_vk_start(struct gtimer *s);
int ngli_gtimer_vk_stop(struct gtimer *s);
int64_t ngli_gtimer_vk_read(struct gtimer *s);
void ngli_gtimer_vk_freep(struct gtimer **sp);

#endif