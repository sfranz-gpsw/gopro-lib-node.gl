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

#include <math.h>

#include "math_utils.h"
#include "noise.h"
#include "utils.h"

static float curve_linear(float t)
{
    return t;
}

static float curve_cubic(float t)
{
    return (3.f - 2.f*t)*t*t;
}

static float curve_quintic(float t)
{
    return ((6.f*t - 15.f)*t + 10.f)*t*t*t;
}

static const interp_func_type interp_func_map[NGLI_NOISE_NB] = {
    [NGLI_NOISE_LINEAR]  = curve_linear,
    [NGLI_NOISE_CUBIC]   = curve_cubic,
    [NGLI_NOISE_QUINTIC] = curve_quintic,
};

/*
 * xorshift64s PRNG, could be replaced with something else
 */
static uint64_t hash(uint64_t x)
{
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    return x * UINT64_C(0x2545F4914F6CDD1D);
}

/*
 * Return a random double-precision float between [0;1)
 * Taken from http://prng.di.unimi.it/
 */
static double rand_u64_to_f64(uint64_t x)
{
    const union { uint64_t i; double d; } u = {.i = UINT64_C(0x3FF) << 52 | x >> 12};
    return u.d - 1.0;
}

/* Gradient noise, returns a value in [-.5;.5) */
static float noise(const struct noise *s, float t)
{
    const float i = floorf(t);  // integer part (lattice point)
    const float f = t - i;      // fractional part: where we are between 2 lattice points
    const uint64_t x = (uint64_t)i + s->params.seed; // seed is an offseting on the lattice

    /*
     * The random values correspond to the random slopes found at the 2 lattice
     * points surrounding our current point; they are between [0;1) so we
     * rescale them to [-1;1), which is equivalent to a tilt between [-π/4;π/4)
     */
    const float s0 = rand_u64_to_f64(hash(x))     * 2.f - 1.f;
    const float s1 = rand_u64_to_f64(hash(x + 1)) * 2.f - 1.f;

    /* Get the y-coordinate of the slope of each lattice */
    const float y0 = s0 * f;
    const float y1 = s1 * (f - 1.f);

    /* Interpolate between the 2 slope y-coordinates */
    const float a = s->interp_func(f);
    const float r = NGLI_MIX(y0, y1, a);
    return r;
}

int ngli_noise_init(struct noise *s, const struct noise_params *params)
{
    ngli_assert(params->function >= 0 && params->function < NGLI_ARRAY_NB(interp_func_map));
    s->interp_func = interp_func_map[params->function];
    s->params = *params;
    return 0;
}

float ngli_noise_get(const struct noise *s, float t)
{
    /* Fractional Brownian Motion */
    const struct noise_params *p = &s->params;
    float sum = 0.f;
    float amp = p->amplitude;
    float x = t * p->frequency;
    for (int i = 0; i < p->octaves; i++) {
        sum += noise(s, x) * amp;
        x *= p->lacunarity; // equivalent to noise(t * freq) with freq *= lacunarity at each octave
        amp *= p->gain;
    }
    return sum;
}
