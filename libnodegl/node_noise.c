/*
 * Copyright 2021 GoPro Inc.
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

#include <stddef.h>

#include "nodegl.h"
#include "nodes.h"
#include "noise.h"
#include "type.h"

struct noise_priv {
    struct variable_priv var;
    struct noise_params noise_params;
    struct noise noise;
};

const struct param_choices noise_func_choices = {
    .name = "interp_noise",
    .consts = {
        {"linear",  NGLI_NOISE_LINEAR,  .desc=NGLI_DOCSTRING("linear interpolation (not recommended), f(t)=t")},
        {"cubic",   NGLI_NOISE_CUBIC,   .desc=NGLI_DOCSTRING("cubic hermite curve, f(t)=3t²-2t³")},
        {"quintic", NGLI_NOISE_QUINTIC, .desc=NGLI_DOCSTRING("quintic curve, f(t)=6t⁵-15t⁴+10t³")},
        {NULL}
    }
};

#define OFFSET(x) offsetof(struct noise_priv, x)
static const struct node_param noise_params[] = {
    {"frequency",   PARAM_TYPE_DBL, OFFSET(noise_params.frequency), {.dbl=1.},
                    .flags=PARAM_FLAG_ALLOW_LIVE_CHANGE,
                    .desc=NGLI_DOCSTRING("oscillation per second")},
    {"amplitude",   PARAM_TYPE_DBL, OFFSET(noise_params.amplitude), {.dbl=1.},
                    .flags=PARAM_FLAG_ALLOW_LIVE_CHANGE,
                    .desc=NGLI_DOCSTRING("by how much it oscillates")},
    {"octaves",     PARAM_TYPE_INT, OFFSET(noise_params.octaves), {.i64=3},
                    .flags=PARAM_FLAG_ALLOW_LIVE_CHANGE,
                    .desc=NGLI_DOCSTRING("number of accumulated noise layers (controls the level of details)")},
    {"lacunarity",  PARAM_TYPE_DBL, OFFSET(noise_params.lacunarity), {.dbl=2.0},
                    .flags=PARAM_FLAG_ALLOW_LIVE_CHANGE,
                    .desc=NGLI_DOCSTRING("frequency multiplier per octave")},
    {"gain",        PARAM_TYPE_DBL, OFFSET(noise_params.gain), {.dbl=0.5},
                    .flags=PARAM_FLAG_ALLOW_LIVE_CHANGE,
                    .desc=NGLI_DOCSTRING("amplitude multiplier per octave (also known as persistence)")},
    {"seed",        PARAM_TYPE_INT, OFFSET(noise_params.seed), {.i64=0x50726e67 /* "Prng" */},
                    .desc=NGLI_DOCSTRING("random seed")},
    {"interpolant", PARAM_TYPE_SELECT, OFFSET(noise_params.function), {.i64=NGLI_NOISE_QUINTIC},
                    .choices=&noise_func_choices,
                    .desc=NGLI_DOCSTRING("interpolation function to use between noise points")},
    {NULL}
};

NGLI_STATIC_ASSERT(variable_priv_is_first, OFFSET(var) == 0);

static int noise_update(struct ngl_node *node, double t)
{
    struct noise_priv *s = node->priv_data;
    s->var.scalar = ngli_noise_get(&s->noise, t);
    return 0;
}

static int noise_init(struct ngl_node *node)
{
    struct noise_priv *s = node->priv_data;
    s->var.data = &s->var.scalar;
    s->var.data_size = sizeof(float);
    s->var.data_type = NGLI_TYPE_FLOAT;
    return ngli_noise_init(&s->noise, &s->noise_params);
}

const struct node_class ngli_noise_class = {
    .id        = NGL_NODE_NOISE,
    .category  = NGLI_NODE_CATEGORY_UNIFORM,
    .name      = "Noise",
    .init      = noise_init,
    .update    = noise_update,
    .priv_size = sizeof(struct noise_priv),
    .params    = noise_params,
    .file      = __FILE__,
};
