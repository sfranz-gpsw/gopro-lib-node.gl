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

#include "gctx.h"
#include "rendertarget.h"

struct rendertarget *ngli_rendertarget_create(struct gctx *gctx)
{
    return gctx->clazz->rendertarget_create(gctx);
}

int ngli_rendertarget_init(struct rendertarget *s, const struct rendertarget_params *params)
{
    return s->gctx->clazz->rendertarget_init(s, params);
}

void ngli_rendertarget_read_pixels(struct rendertarget *s, uint8_t *data)
{
    s->gctx->clazz->rendertarget_read_pixels(s, data);
}

void ngli_rendertarget_freep(struct rendertarget **sp)
{
    if (!*sp)
        return;
    (*sp)->gctx->clazz->rendertarget_freep(sp);
}
