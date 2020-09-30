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

#include "texture_ngfx.h"
#include "log.h"
#include "memory.h"
#include <map>
#include "graphics/Texture.h"
#include "gctx_ngfx.h"
#include "format_ngfx.h"
using namespace ngfx;
using namespace std;

static FilterMode to_ngfx_filter(int filter) {
    static const map<int, ngfx::FilterMode> filter_map = {
        { NGLI_FILTER_NEAREST, FILTER_NEAREST },
        { NGLI_FILTER_LINEAR, FILTER_LINEAR }
    };
    return filter_map.at(filter);
}

static FilterMode to_ngfx_mip_filter(int filter) {
    static const map<int, ngfx::FilterMode> mip_filter_map = {
        { NGLI_MIPMAP_FILTER_NEAREST, FILTER_NEAREST},
        { NGLI_MIPMAP_FILTER_LINEAR, FILTER_LINEAR }
    };
    return mip_filter_map.at(filter);
}

static inline uint32_t get_bpp(int format) {
    return ngli_format_get_bytes_per_pixel(format);
}

static TextureType to_ngfx_texture_type(int type) {
    static const map<int, TextureType> texture_type_map = {
        { NGLI_TEXTURE_TYPE_2D, TEXTURE_TYPE_2D },
        { NGLI_TEXTURE_TYPE_3D, TEXTURE_TYPE_3D },
        { NGLI_TEXTURE_TYPE_CUBE, TEXTURE_TYPE_CUBE }
    };
    return texture_type_map.at(type);
}

struct texture *ngli_texture_ngfx_create(struct gctx *gctx) {
    texture_ngfx *s = (texture_ngfx*)ngli_calloc(1, sizeof(*s));
    if (!s)
        return NULL;
    s->parent.gctx = gctx;
    return (struct texture *)s;
}

int ngli_texture_ngfx_init(struct texture *s,
                           const struct texture_params *p) {
    struct texture_ngfx *s_priv = (struct texture_ngfx *)s;
    struct gctx_ngfx *ctx = (struct gctx_ngfx *)s->gctx;
    uint32_t size = get_bpp(p->format) * p->width * p->height;
    bool gen_mipmaps = p->mipmap_filter != NGLI_MIPMAP_FILTER_NONE;
    uint32_t image_usage_flags = ImageUsageFlags(IMAGE_USAGE_TRANSFER_SRC_BIT | IMAGE_USAGE_TRANSFER_DST_BIT | IMAGE_USAGE_SAMPLED_BIT);
    uint32_t depth = (p->type == NGLI_TEXTURE_TYPE_3D) ? p->depth : 1;
    uint32_t array_layers = (p->type == NGLI_TEXTURE_TYPE_CUBE) ? 6 : 1;
    s_priv->v = Texture::create(ctx->graphicsContext, ctx->graphics,
        nullptr, to_ngfx_format(p->format), size, p->width, p->height, depth, array_layers,
        image_usage_flags, to_ngfx_texture_type(p->type), gen_mipmaps,
        to_ngfx_filter(p->min_filter), to_ngfx_filter(p->mag_filter),
        gen_mipmaps ? to_ngfx_mip_filter(p->mipmap_filter) : FILTER_NEAREST,
        p->samples == 0 ? 1 : p->samples
    );
    return 0;
}

int ngli_texture_ngfx_has_mipmap(const struct texture *s) { TODO(); return 0; }
int ngli_texture_ngfx_match_dimensions(const struct texture *s, int width, int height, int depth) {
    const texture_params *params = &s->params;
    return params->width == width && params->height == height && params->depth == depth;
}

int ngli_texture_ngfx_upload(struct texture *s, const uint8_t *data, int linesize) {
    TODO("Texture::upload");
    return 0;
}
int ngli_texture_ngfx_generate_mipmap(struct texture *s) {
    TODO("Texture::generateMipmaps");
    return 0;
}

void ngli_texture_ngfx_freep(struct texture **sp) {
    if (!sp) return;
    texture *s = *sp;
    texture_ngfx *s_priv = (struct texture_ngfx *)s;
    delete s_priv->v;
    ngli_freep(sp);
}

