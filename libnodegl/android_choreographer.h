/*
 * Copyright 2017 GoPro Inc.
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

#ifndef ANDROID_CHOREOGRAPHER_H
#define ANDROID_CHOREOGRAPHER_H

#include <stdint.h>

struct android_choreographer;
typedef void (*android_choreographer_cb)(void *user_data, uint64_t frame_time_ns);

struct android_choreographer *ngli_android_choreographer_new(android_choreographer_cb user_cb, void *user_data);
int ngli_android_choreographer_start(struct android_choreographer *choreographer);
int ngli_android_choreographer_stop(struct android_choreographer *choreographer);
void ngli_android_choreographer_free(struct android_choreographer **choreographer);

#endif /* ANDROID_CHOREOGRAPHER_H */
