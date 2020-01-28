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

#ifndef GRAPHICCONFIG_H
#define GRAPHICCONFIG_H

#include "utils.h"

enum {
    NGLI_BLEND_FACTOR_ZERO,
    NGLI_BLEND_FACTOR_ONE,
    NGLI_BLEND_FACTOR_SRC_COLOR,
    NGLI_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
    NGLI_BLEND_FACTOR_DST_COLOR,
    NGLI_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
    NGLI_BLEND_FACTOR_SRC_ALPHA,
    NGLI_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    NGLI_BLEND_FACTOR_DST_ALPHA,
    NGLI_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
    NGLI_BLEND_FACTOR_NB
};

#define BLEND_FACTOR_NBITS 4
NGLI_STATIC_ASSERT(blend_factors_4bits, NGLI_BLEND_FACTOR_NB <= (1<<BLEND_FACTOR_NBITS));

enum {
    NGLI_BLEND_OP_ADD,
    NGLI_BLEND_OP_SUBTRACT,
    NGLI_BLEND_OP_REVERSE_SUBTRACT,
    NGLI_BLEND_OP_MIN,
    NGLI_BLEND_OP_MAX,
    NGLI_BLEND_OP_NB
};

#define BLEND_OP_NBITS 3
NGLI_STATIC_ASSERT(blend_op_3bits, NGLI_BLEND_OP_NB <= (1<<BLEND_OP_NBITS));

enum {
    NGLI_COMPARE_OP_NEVER,
    NGLI_COMPARE_OP_LESS,
    NGLI_COMPARE_OP_EQUAL,
    NGLI_COMPARE_OP_LESS_OR_EQUAL,
    NGLI_COMPARE_OP_GREATER,
    NGLI_COMPARE_OP_NOT_EQUAL,
    NGLI_COMPARE_OP_GREATER_OR_EQUAL,
    NGLI_COMPARE_OP_ALWAYS,
    NGLI_COMPARE_OP_NB
};

#define COMPARE_OP_NBITS 3
NGLI_STATIC_ASSERT(compare_op_3bits, NGLI_COMPARE_OP_NB <= (1<<COMPARE_OP_NBITS));

enum {
    NGLI_STENCIL_OP_KEEP,
    NGLI_STENCIL_OP_ZERO,
    NGLI_STENCIL_OP_REPLACE,
    NGLI_STENCIL_OP_INCREMENT_AND_CLAMP,
    NGLI_STENCIL_OP_DECREMENT_AND_CLAMP,
    NGLI_STENCIL_OP_INVERT,
    NGLI_STENCIL_OP_INCREMENT_AND_WRAP,
    NGLI_STENCIL_OP_DECREMENT_AND_WRAP,
    NGLI_STENCIL_OP_NB
};

#define STENCIL_OP_NBITS 3
NGLI_STATIC_ASSERT(stencil_op_3bits, NGLI_STENCIL_OP_NB <= (1<<STENCIL_OP_NBITS));

enum {
    NGLI_CULL_MODE_NONE,
    NGLI_CULL_MODE_FRONT_BIT,
    NGLI_CULL_MODE_BACK_BIT,
    NGLI_CULL_MODE_FRONT_AND_BACK,
    NGLI_CULL_MODE_NB
};

#define CULL_MODE_OP_NBITS 2

NGLI_STATIC_ASSERT(cull_mode, (NGLI_CULL_MODE_FRONT_BIT | NGLI_CULL_MODE_BACK_BIT) == NGLI_CULL_MODE_FRONT_AND_BACK);

enum {
    NGLI_COLOR_COMPONENT_R_BIT = 1 << 0,
    NGLI_COLOR_COMPONENT_G_BIT = 1 << 1,
    NGLI_COLOR_COMPONENT_B_BIT = 1 << 2,
    NGLI_COLOR_COMPONENT_A_BIT = 1 << 3,
};

struct graphicconfig {
    unsigned blend:1;
    unsigned blend_dst_factor:BLEND_FACTOR_NBITS;
    unsigned blend_src_factor:BLEND_FACTOR_NBITS;
    unsigned blend_dst_factor_a:BLEND_FACTOR_NBITS;
    unsigned blend_src_factor_a:BLEND_FACTOR_NBITS;
    unsigned blend_op:BLEND_OP_NBITS;
    unsigned blend_op_a:BLEND_OP_NBITS;

    unsigned color_write_mask:4;

    unsigned depth_test:1;
    unsigned depth_write_mask:1;
    unsigned depth_func:COMPARE_OP_NBITS;

    unsigned stencil_test:1;
    unsigned stencil_write_mask:1;
    unsigned stencil_func:COMPARE_OP_NBITS;
    unsigned stencil_ref:8;
    unsigned stencil_read_mask:8;
    unsigned stencil_fail:STENCIL_OP_NBITS;
    unsigned stencil_depth_fail:STENCIL_OP_NBITS;
    unsigned stencil_depth_pass:STENCIL_OP_NBITS;

    unsigned cull_face:1;
    unsigned cull_face_mode:CULL_MODE_OP_NBITS;

    unsigned scissor_test:1;
};

void ngli_graphicconfig_init(struct graphicconfig *s);

#endif
