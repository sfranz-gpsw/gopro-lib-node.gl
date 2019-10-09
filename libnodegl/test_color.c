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

#include <math.h>
#include <sxplayer.h>

#include "utils.h"
#include "math_utils.h"
#include "colorconv.h"

#define M_EPS 1e-6

int main(void)
{
    float identity[] = NGLI_MAT4_IDENTITY;
    float tmp[] = NGLI_MAT4_IDENTITY;

    //
    // Checking YCbCr <-> RGB
    //

    float ycbcr_to_rgb_bt709[] = {
        1.16438356,  1.16438356,  1.16438356, 0.0,
               0.0, -0.21324861,  2.11240179, 0.0,
        1.79274107, -0.53290933,         0.0, 0.0,
       -0.97294508,  0.30148267, -1.13340222, 1.0
    };

    float ycbcr_to_rgb[] = NGLI_MAT4_IDENTITY;
    ngli_colorconv_ycbcr_to_rgb_mat4(ycbcr_to_rgb, 1, SXPLAYER_COL_RNG_LIMITED);
    printf("YCbCr to RGB:\n" NGLI_FMT_MAT4 "\n", NGLI_ARG_MAT4(ycbcr_to_rgb));

    float rgb_to_ycbcr[] = NGLI_MAT4_IDENTITY;
    ngli_colorconv_rgb_to_ycbcr_mat4(rgb_to_ycbcr, 1, SXPLAYER_COL_RNG_LIMITED);
    printf("RGB to YCbCr:\n" NGLI_FMT_MAT4 "\n", NGLI_ARG_MAT4(rgb_to_ycbcr));

    ngli_mat4_mul_c(tmp, ycbcr_to_rgb, rgb_to_ycbcr);
    printf("Round trip:\n" NGLI_FMT_MAT4 "\n", NGLI_ARG_MAT4(tmp));

    for (int i = 0; i < 4*4; ++i) {
        ngli_assert(fabs(ycbcr_to_rgb[i] - ycbcr_to_rgb_bt709[i]) < M_EPS);
        ngli_assert(fabs(tmp[i] - identity[i]) < M_EPS);
    }

    //
    // Checking RGB <-> RGB
    //

    float bt709_to_bt2020[] = {
         0.6274039, 0.32928304, 0.04331307, 0.0,
        0.06909729, 0.9195404,  0.01136232, 0.0,
        0.01639144, 0.08801331, 0.89559525, 0.0,
               0.0,        0.0,        0.0, 1.0
    };

    float rgb709_to_rgb2020[] = NGLI_MAT4_IDENTITY;
    ngli_colorconv_rgb_to_rgb_mat4(
        rgb709_to_rgb2020, SXPLAYER_COL_SPC_BT709, SXPLAYER_COL_SPC_BT2020_NCL);
    printf("BT.709 to BT.2020:\n" NGLI_FMT_MAT4 "\n", NGLI_ARG_MAT4(tmp));

    float rgb2020_to_rgb709[] = NGLI_MAT4_IDENTITY;
    ngli_colorconv_rgb_to_rgb_mat4(
        rgb2020_to_rgb709, SXPLAYER_COL_SPC_BT2020_NCL, SXPLAYER_COL_SPC_BT709);
    printf("BT.2020 to BT.709:\n" NGLI_FMT_MAT4 "\n", NGLI_ARG_MAT4(tmp));

    ngli_mat4_mul_c(tmp, rgb709_to_rgb2020, rgb2020_to_rgb709);
    printf("Round trip:\n" NGLI_FMT_MAT4 "\n", NGLI_ARG_MAT4(tmp));

    for (int i = 0; i < 4*4; ++i) {
        ngli_assert(fabs(rgb709_to_rgb2020[i] - bt709_to_bt2020[i]) < M_EPS);
        ngli_assert(fabs(tmp[i] - identity[i]) < M_EPS);
    }

    return 0;
}
