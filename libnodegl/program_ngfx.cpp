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

#include "memory.h"
#include "program_ngfx.h"
#include <stdlib.h>
#include "log.h"
#include "graphics/ShaderModule.h"
#include "gctx_ngfx.h"
using namespace ngfx;

struct program *ngli_program_ngfx_create(struct gctx *gctx) {
    program_ngfx *s = (program_ngfx*)ngli_calloc(1, sizeof(*s));
    if (!s)
        return NULL;
    s->parent.gctx = gctx;
    return (struct program *)s;
}
int ngli_program_ngfx_init(struct program *s, const char *vertex, const char *fragment, const char *compute) {
    gctx_ngfx *p_gctx_ngfx = (struct gctx_ngfx *)s->gctx;
    //if (vertex) VertexShaderModule::create(p_gctx_ngfx->graphicsContext->device, vertex);
    //if (fragment) FragmentShaderModule::create(p_gctx_ngfx->graphicsContext->device, fragment);
    //if (compute) ComputeShaderModule::create(p_gctx_ngfx->graphicsContext->device, compute);
    TODO("VertexShaderModule::create / FragmentShaderModule::create / ComputeShaderModule::create");
    return 0;
}
void ngli_program_ngfx_freep(struct program **sp) {
    TODO("delete pipeline");
}

