/*
 * Copyright 2016 GoPro Inc.
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
#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "nodegl.h"
#include "nodes.h"
#include "pipeline.h"
#include "topology.h"
#include "utils.h"

static const struct {
    const char *name;
    int format;
} formats_map[NGLI_NODE_GEOMETRY_INDEX_NB] = {
    [NGLI_NODE_GEOMETRY_INDEX_VERTICES] = {"ngl_position", NGLI_FORMAT_R32G32B32_SFLOAT},
    [NGLI_NODE_GEOMETRY_INDEX_UVCOORDS] = {"ngl_uvcoord",  NGLI_FORMAT_R32G32_SFLOAT},
    [NGLI_NODE_GEOMETRY_INDEX_NORMALS]  = {"ngl_normals",  NGLI_FORMAT_R32G32B32_SFLOAT},
};

int ngli_node_geometry_init_attribute(struct ngl_node *node, int index, const void *data, int data_size)
{
    struct ngl_ctx *ctx = node->ctx;
    struct geometry_priv *s = node->priv_data;

    ngli_assert(index >= 0 && index < NGLI_NODE_GEOMETRY_INDEX_NB);

    struct buffer *buffer = &s->buffers[index];

    int ret = ngli_buffer_init(buffer, ctx, data_size, NGLI_BUFFER_USAGE_STATIC);
    if (ret < 0)
        return ret;

    ret = ngli_buffer_upload(buffer, data, data_size);
    if (ret < 0)
        return ret;

    struct pipeline_attribute *attribute = &s->attributes[index];
    int format = formats_map[index].format;
    const char *name = formats_map[index].name;

    *attribute = (struct pipeline_attribute) {
        .format = format,
        .count  = 1,
        .stride = ngli_format_get_bytes_per_pixel(format),
        .rate   = 0,
        .buffer = buffer,
    };
    snprintf(attribute->name, sizeof(attribute->name), "%s", name);

    return 0;
}

static const struct param_choices topology_choices = {
    .name = "topology",
    .consts = {
        {"point_list",     NGLI_PRIMITIVE_TOPOLOGY_POINT_LIST,     .desc=NGLI_DOCSTRING("point list")},
        {"line_strip",     NGLI_PRIMITIVE_TOPOLOGY_LINE_STRIP,     .desc=NGLI_DOCSTRING("line strip")},
        {"line_list",      NGLI_PRIMITIVE_TOPOLOGY_LINE_LIST,      .desc=NGLI_DOCSTRING("line list")},
        {"triangle_strip", NGLI_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, .desc=NGLI_DOCSTRING("triangle strip")},
        {"triangle_fan",   NGLI_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN,   .desc=NGLI_DOCSTRING("triangle fan")},
        {"triangle_list",  NGLI_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,  .desc=NGLI_DOCSTRING("triangle list")},
        {NULL}
    }
};

#define TEXCOORDS_TYPES_LIST (const int[]){NGL_NODE_BUFFERFLOAT,            \
                                           NGL_NODE_BUFFERVEC2,             \
                                           NGL_NODE_BUFFERVEC3,             \
                                           NGL_NODE_ANIMATEDBUFFERFLOAT,    \
                                           NGL_NODE_ANIMATEDBUFFERVEC2,     \
                                           NGL_NODE_ANIMATEDBUFFERVEC3,     \
                                           -1}

#define OFFSET(x) offsetof(struct geometry_priv, x)
static const struct node_param geometry_params[] = {
    {"vertices",  PARAM_TYPE_NODE, OFFSET(vertices_buffer),
                  .node_types=(const int[]){NGL_NODE_BUFFERVEC3, NGL_NODE_ANIMATEDBUFFERVEC3, -1},
                  .flags=PARAM_FLAG_CONSTRUCTOR | PARAM_FLAG_DOT_DISPLAY_FIELDNAME,
                  .desc=NGLI_DOCSTRING("vertice coordinates defining the geometry")},
    {"uvcoords",  PARAM_TYPE_NODE, OFFSET(uvcoords_buffer),
                  .node_types=TEXCOORDS_TYPES_LIST,
                  .flags=PARAM_FLAG_DOT_DISPLAY_FIELDNAME,
                  .desc=NGLI_DOCSTRING("coordinates used for UV mapping of each `vertices`")},
    {"normals",   PARAM_TYPE_NODE, OFFSET(normals_buffer),
                  .node_types=(const int[]){NGL_NODE_BUFFERVEC3, NGL_NODE_ANIMATEDBUFFERVEC3, -1},
                  .flags=PARAM_FLAG_DOT_DISPLAY_FIELDNAME,
                  .desc=NGLI_DOCSTRING("normal vectors of each `vertices`")},
    {"indices",   PARAM_TYPE_NODE, OFFSET(indices_buffer),
                  .node_types=(const int[]){NGL_NODE_BUFFERUSHORT, NGL_NODE_BUFFERUINT, -1},
                  .flags=PARAM_FLAG_DOT_DISPLAY_FIELDNAME,
                  .desc=NGLI_DOCSTRING("indices defining the drawing order of the `vertices`, auto-generated if not set")},
    {"topology",  PARAM_TYPE_SELECT, OFFSET(topology), {.i64=NGLI_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST},
                  .choices=&topology_choices,
                  .desc=NGLI_DOCSTRING("primitive topology")},
    {NULL}
};


static int init_attribute(struct ngl_node *node, int index, struct ngl_node *attribute)
{
    struct geometry_priv *s = node->priv_data;
    struct buffer_priv *attribute_priv = attribute->priv_data;

    ngli_assert(index >= 0 && index < NGLI_NODE_GEOMETRY_INDEX_NB);

    int stride = attribute_priv->data_stride;
    int offset = 0;
    struct buffer *buffer = &attribute_priv->buffer;

    if (attribute_priv->block) {
        struct block_priv *block = attribute_priv->block->priv_data;
        const struct block_field_info *fi = &block->field_info[attribute_priv->block_field];
        stride = fi->stride;
        offset = fi->offset;
        buffer = &block->buffer;
    }

    const char *name = formats_map[index].name;

    if (attribute_priv->count != s->graphics.nb_vertices) {
        LOG(ERROR, "%s count (%d) does not match vertices count (%d)",
            name, attribute_priv->count, s->graphics.nb_vertices);
        return NGL_ERROR_UNSUPPORTED;
    }

    s->attributes[index] = (struct pipeline_attribute){
        .format = attribute_priv->data_format,
        .stride = stride,
        .offset = offset,
        .count = 1,
        .rate = 0,
        .buffer = buffer,
    };
    snprintf(s->attributes[index].name, sizeof(s->attributes[index].name), "%s", name);

    int ret = ngli_node_buffer_ref(attribute);
    if (ret < 0)
        return ret;

    return 0;
}

static int geometry_init(struct ngl_node *node)
{
    struct geometry_priv *s = node->priv_data;
    struct buffer_priv *vertices = s->vertices_buffer->priv_data;

    struct pipeline_graphics *graphics = &s->graphics;
    graphics->topology = s->topology;
    graphics->nb_instances = 1;
    graphics->nb_vertices = vertices->count;

    if (s->indices_buffer) {
        struct ngl_node *indices = s->indices_buffer;
        int ret = ngli_node_buffer_ref(indices);
        if (ret < 0)
            return ret;

        struct buffer_priv *indices_priv = indices->priv_data;
        if (indices_priv->block) {
            LOG(ERROR, "geometry indices buffers referencing a block are not supported");
            return NGL_ERROR_UNSUPPORTED;
        }

        graphics->nb_indices = indices_priv->count;
        graphics->indices_format = indices_priv->data_format;
        graphics->indices = &indices_priv->buffer;
    }

    int ret = init_attribute(node, NGLI_NODE_GEOMETRY_INDEX_VERTICES, s->vertices_buffer);
    if (ret < 0)
        return ret;

    ret = init_attribute(node, NGLI_NODE_GEOMETRY_INDEX_UVCOORDS, s->uvcoords_buffer);
    if (ret < 0)
        return ret;

    ret = init_attribute(node, NGLI_NODE_GEOMETRY_INDEX_NORMALS, s->normals_buffer);
    if (ret < 0)
        return ret;

    return 0;
}

static int update_buffer(struct ngl_node *node, double t)
{
    if (!node)
        return 0;

    int ret = ngli_node_update(node, t);
    if (ret < 0)
        return ret;

    ret = ngli_node_buffer_upload(node);
    if (ret < 0)
        return ret;

    return 0;
}

static int geometry_update(struct ngl_node *node, double t)
{
    struct geometry_priv *s = node->priv_data;

    int ret = update_buffer(s->vertices_buffer, t);
    if (ret < 0)
        return ret;

    ret = update_buffer(s->uvcoords_buffer, t);
    if (ret < 0)
        return ret;

    ret = update_buffer(s->normals_buffer, t);
    if (ret < 0)
        return ret;

    return 0;
}

static void geometry_uninit(struct ngl_node *node)
{
    struct geometry_priv *s = node->priv_data;
}

const struct node_class ngli_geometry_class = {
    .id        = NGL_NODE_GEOMETRY,
    .name      = "Geometry",
    .init      = geometry_init,
    .update    = geometry_update,
    .uninit    = geometry_uninit,
    .priv_size = sizeof(struct geometry_priv),
    .params    = geometry_params,
    .file      = __FILE__,
};
