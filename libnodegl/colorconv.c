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

#include "colorconv.h"
#include "utils.h"
#include "math_utils.h"

struct chroma_xy {
    float x;
    float y;
};

static const struct color_space {
    const char *name;
    const char *desc;
    int space_id;
    int primaries_id;
    int transfer_id;
    struct chroma_xy red;
    struct chroma_xy green;
    struct chroma_xy blue;
    struct chroma_xy white;
} color_spaces[] = {
    {
        .name = "SMPTE 170M",
        .desc = "ITU-R BT.601 525, NTSC",
        .space_id = SXPLAYER_COL_SPC_SMPTE170M,
        .primaries_id = SXPLAYER_COL_PRI_SMPTE170M,
        .transfer_id = SXPLAYER_COL_TRC_SMPTE170M,
        .red =   { 0.630 , 0.340  },
        .green = { 0.310 , 0.595  },
        .blue =  { 0.155 , 0.070  },
        .white = { 0.3127, 0.3290 },
    },
    {
        .name = "BT.709",
        .desc = "ITU-R BT.709, HDTV",
        .space_id = SXPLAYER_COL_SPC_BT709,
        .primaries_id = SXPLAYER_COL_PRI_BT709,
        .transfer_id = SXPLAYER_COL_TRC_BT709,
        .red =   { 0.640 , 0.330  },
        .green = { 0.300 , 0.600  },
        .blue =  { 0.150 , 0.060  },
        .white = { 0.3127, 0.3290 },
    },
    {
        .name = "BT.2020",
        .desc = "ITU-R BT.2020, UHDTV",
        .space_id = SXPLAYER_COL_SPC_BT2020_NCL,
        .primaries_id = SXPLAYER_COL_PRI_BT2020,
        .transfer_id = SXPLAYER_COL_TRC_SMPTE2084,
        .red =   { 0.708 , 0.292  },
        .green = { 0.170 , 0.797  },
        .blue =  { 0.131 , 0.046  },
        .white = { 0.3127, 0.3290 },
    }
};

static const struct range {
    const char *name;
    int range_id;
    float offsets[3];
    float scales[3];
} ranges[] = {
    {
        .name = "Legal",
        .range_id = SXPLAYER_COL_RNG_LIMITED,
        .offsets = { 16.0 / 255.0, 128.0 / 255.0, 128.0 / 255.0 },
        .scales = { 219.0 / 255.0, 224.0 / 255.0, 224.0 / 255.0 },
    },
    {
        .name = "Full",
        .range_id = SXPLAYER_COL_RNG_FULL,
        .offsets = { 0.0, 0.0, 0.0 },
        .scales = { 1.0, 1.0, 1.0 },
    }
};

static int find_color_space(struct color_space *color_space, int space_id)
{
    for (int i = 0; i < sizeof(color_spaces) / sizeof(color_space); ++i) {
        if (color_spaces[i].space_id == space_id) {
            *color_space = color_spaces[i];
            return 0;
        }
    }

    return -1;
}

static int find_range(struct range *range, int range_id)
{
    for (int i = 0; i < sizeof(ranges) / sizeof(range); ++i) {
        if (ranges[i].range_id == range_id) {
            *range = ranges[i];
            return 0;
        }
    }

    return -1;
}

static void normalized_primary_matrix(float *dst, const struct color_space *csp)
{
    // SMPTE RP 177-1993, Section 3.3
    float p_mat[3*3] = {
        csp->red.x, csp->green.x, csp->blue.x,
        csp->red.y, csp->green.y, csp->blue.y,
        1.0 - csp->red.x - csp->red.y, 1.0 - csp->green.x - csp->green.y, 1.0 - csp->blue.x - csp->blue.y};

    float white_vec[3] = {
        csp->white.x / csp->white.y,
        1.0,
        (1.0 - csp->white.x - csp->white.y) / csp->white.y
    };

    float p_mat_inv[] = NGLI_MAT3_IDENTITY;
    float c_vec[3] = { 1.0, 1.0, 1.0 };
    ngli_mat3_inverse(p_mat_inv, p_mat);
    ngli_mat3_transpose(p_mat_inv, p_mat_inv);
    ngli_mat3_mul_vec3_c(c_vec, p_mat_inv, white_vec);

    float c_mat[] = NGLI_MAT3_IDENTITY;
    ngli_mat3_transpose(p_mat, p_mat);
    ngli_mat3_scale(c_mat, c_vec[0], c_vec[1], c_vec[2]);
    ngli_mat3_mul_c(dst, p_mat, c_mat);
    ngli_mat3_transpose(dst, dst);
}

static void luma_coefficients(float *dst, const struct color_space *csp)
{
    float npm[] = NGLI_MAT3_IDENTITY;
    normalized_primary_matrix(npm, csp);

    // SMPTE RP 177-1993, Section 3.3.8
    dst[0] = npm[3];
    dst[1] = npm[4];
    dst[2] = npm[5];

    // Round to 4 decimals places to match standards
    int16_t rnd = 1e4;
    dst[0] = roundf(dst[0] * rnd) / rnd;
    dst[1] = roundf(dst[1] * rnd) / rnd;
    dst[2] = roundf(dst[2] * rnd) / rnd;
}

void ngli_colorconv_ycbcr_to_rgb_mat4(float *dst, int colorspace_id, int range_id)
{
    struct range range;
    if (find_range(&range, range_id) < 0)
        return;

    // YCbCr to RGB matrix
    float ycbcr_to_rgb[] = NGLI_MAT3_IDENTITY;
    ngli_colorconv_rgb_to_ycbcr_mat4(dst, colorspace_id, range_id);
    ngli_mat3_from_mat4(ycbcr_to_rgb, dst);
    ngli_mat3_inverse(ycbcr_to_rgb, ycbcr_to_rgb);

    // Offset components
    float offsets[3] = { -1.0, -1.0, -1.0 };
    ngli_vec3_mul(offsets, range.offsets, offsets);
    ngli_mat3_mul_vec3_c(offsets, ycbcr_to_rgb, offsets);

    ngli_mat4_identity(dst);
    dst[ 0] = ycbcr_to_rgb[0];
    dst[ 1] = ycbcr_to_rgb[1];
    dst[ 2] = ycbcr_to_rgb[2];
    dst[ 4] = ycbcr_to_rgb[3];
    dst[ 5] = ycbcr_to_rgb[4];
    dst[ 6] = ycbcr_to_rgb[5];
    dst[ 8] = ycbcr_to_rgb[6];
    dst[ 9] = ycbcr_to_rgb[7];
    dst[10] = ycbcr_to_rgb[8];
    dst[12] = offsets[0];
    dst[13] = offsets[1];
    dst[14] = offsets[2];
}

void ngli_colorconv_rgb_to_ycbcr_mat4(float *dst, int colorspace_id, int range_id)
{
    struct color_space color_space;
    struct range range;
    if (find_color_space(&color_space, colorspace_id) < 0)
        return;
    if (find_range(&range, range_id) < 0)
        return;

    float luma_coeff[3] = { 1.0, 1.0, 1.0 };
    luma_coefficients(luma_coeff, &color_space);

    // Luma / Chroma difference encoding
    ngli_mat4_identity(dst);
    dst[ 0] = luma_coeff[0];
    dst[ 1] = luma_coeff[1];
    dst[ 2] = luma_coeff[2];
    dst[ 4] = -luma_coeff[0];
    dst[ 5] = -luma_coeff[1];
    dst[ 6] = 1.0 - luma_coeff[2];
    dst[ 8] = 1.0 - luma_coeff[0];
    dst[ 9] = -luma_coeff[1];
    dst[10] = -luma_coeff[2];

    // Scale components
    float chroma_scale[3] = {
        1.0,
        0.5 / (1.0 - luma_coeff[2]),
        0.5 / (1.0 - luma_coeff[0])};

    dst[ 0] *= range.scales[0];
    dst[ 1] *= range.scales[0];
    dst[ 2] *= range.scales[0];
    dst[ 4] *= chroma_scale[1] * range.scales[1];
    dst[ 5] *= chroma_scale[1] * range.scales[1];
    dst[ 6] *= chroma_scale[1] * range.scales[1];
    dst[ 8] *= chroma_scale[2] * range.scales[2];
    dst[ 9] *= chroma_scale[2] * range.scales[2];
    dst[10] *= chroma_scale[2] * range.scales[2];
    dst[ 3] = range.offsets[0];
    dst[ 7] = range.offsets[1];
    dst[11] = range.offsets[2];

    // OpenGL convention
    ngli_mat4_transpose(dst, dst);
}

void ngli_colorconv_rgb_to_rgb_mat4(
    float *dst, int colorspace_src_id, int colorspace_dst_id)
{
    struct color_space color_space_src, color_space_dst;
    if (find_color_space(&color_space_src, colorspace_src_id) < 0)
        return;
    if (find_color_space(&color_space_dst, colorspace_dst_id) < 0)
        return;

    float src_npm[] = NGLI_MAT3_IDENTITY;
    float dst_npm[] = NGLI_MAT3_IDENTITY;
    normalized_primary_matrix(src_npm, &color_space_src);
    normalized_primary_matrix(dst_npm, &color_space_dst);

    ngli_mat3_inverse(dst_npm, dst_npm);
    ngli_mat3_mul_c(src_npm, src_npm, dst_npm);
    ngli_mat4_from_mat3(dst, src_npm);
}