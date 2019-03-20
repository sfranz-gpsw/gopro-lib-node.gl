/*
 * Copyright 2019 GoPro Inc.
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

#ifndef RENDERPASS_H
#define RENDERPASS_H

#include "glcontext.h"

struct renderpass_params {
};

struct renderpass {
    struct glcontext *gl;
    struct renderpass_params params;
};

int ngl_renderpass_init(struct renderpass *s, struct glcontext *gl, const struct renderpass_params *params);
int ngl_renderpass_begin(struct renderpass *s);
int ngl_renderpass_uninit(struct renderpass *s);


#endif
