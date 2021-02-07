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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "noise.h"
#include "utils.h"

static const struct noise_params test_params = {
    .frequency  = 10.f,
    .amplitude  = 1.5,
    .octaves    = 12,
    .lacunarity = 1.95,
    .gain       = .55,
    .seed       = 0xc474fe39,
    .function   = NGLI_NOISE_CUBIC,
};

static const float expected_values[] = {
   0.000000,-0.022757,-0.041530,-0.042940,-0.031355,-0.011934, 0.013487, 0.032621, 0.020161,-0.062068,
   0.134803, 0.080771, 0.013989,-0.042985, 0.144758, 0.046720,-0.200473,-0.016233,-0.165513,-0.137913,
   0.031570,-0.009753, 0.055482,-0.112498, 0.055139,-0.019630,-0.004833,-0.047401, 0.143775,-0.165227,
};

static int run_test(void)
{
    struct noise noise;
    if (ngli_noise_init(&noise, &test_params) < 0)
        return EXIT_FAILURE;

    int ret = 0;
    const int nb_values = NGLI_ARRAY_NB(expected_values);
    for (int i = 0; i < nb_values; i++) {
        const float t = i / test_params.frequency;
        const float gv = ngli_noise_get(&noise, t);
        const float ev = expected_values[i];
        if (fabs(gv - ev) > 0.0001) {
            fprintf(stderr, "value #%d: got %g but expected %g [err:%g]\n", i, gv, ev, fabs(gv - ev));
            ret = EXIT_FAILURE;
        }
    }

    return ret;
}

int main(int ac, char **av)
{
    if (ac == 1)
        return run_test();

    const float duration = ac > 1 ? atof(av[1]) : 3.f;
    const struct noise_params np = {
        .frequency  = ac > 2 ? atof(av[2]) : test_params.frequency,
        .amplitude  = ac > 3 ? atof(av[3]) : test_params.amplitude,
        .octaves    = ac > 4 ? atoi(av[4]) : test_params.octaves,
        .lacunarity = ac > 5 ? atof(av[5]) : test_params.lacunarity,
        .gain       = ac > 6 ? atof(av[6]) : test_params.gain,
        .seed       = ac > 8 ? atoi(av[8]) : test_params.seed,
        .function   = ac > 7 ? atoi(av[7]) : test_params.function,
    };

    printf("# duration:%g frq:%g amp:%g oct:%d lac:%g gain:%g seed:0x%08x fn:%d\n",
           duration, np.frequency, np.amplitude, np.octaves, np.lacunarity, np.gain, np.function, np.seed);

    struct noise noise;
    if (ngli_noise_init(&noise, &np) < 0)
        return EXIT_FAILURE;

    const int nb_values = (int)(duration * np.frequency);
    for (int i = 0; i < nb_values; i++) {
        const float t = i / np.frequency;
        printf("%f: %f\n", t, ngli_noise_get(&noise, t));
    }

    return 0;
}
