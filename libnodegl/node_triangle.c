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

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "log.h"
#include "math_utils.h"
#include "nodegl.h"
#include "nodes.h"
#include "topology.h"
#include "utils.h"

#define OFFSET(x) offsetof(struct geometry_priv, x)
static const struct node_param triangle_params[] = {
    {"edge0", PARAM_TYPE_VEC3, OFFSET(triangle_edges[0]), .flags=PARAM_FLAG_CONSTRUCTOR,
              .desc=NGLI_DOCSTRING("first edge coordinate of the triangle")},
    {"edge1", PARAM_TYPE_VEC3, OFFSET(triangle_edges[3]), .flags=PARAM_FLAG_CONSTRUCTOR,
              .desc=NGLI_DOCSTRING("second edge coordinate of the triangle")},
    {"edge2", PARAM_TYPE_VEC3, OFFSET(triangle_edges[6]), .flags=PARAM_FLAG_CONSTRUCTOR,
             .desc=NGLI_DOCSTRING("third edge coordinate of the triangle")},
    {"uv_edge0", PARAM_TYPE_VEC2, OFFSET(triangle_uvs[0]), {.vec={0.0f, 0.0f}},
                 .desc=NGLI_DOCSTRING("UV coordinate associated with `edge0`")},
    {"uv_edge1", PARAM_TYPE_VEC2, OFFSET(triangle_uvs[2]), {.vec={0.0f, 1.0f}},
                 .desc=NGLI_DOCSTRING("UV coordinate associated with `edge1`")},
    {"uv_edge2", PARAM_TYPE_VEC2, OFFSET(triangle_uvs[4]), {.vec={1.0f, 1.0f}},
                 .desc=NGLI_DOCSTRING("UV coordinate associated with `edge2`")},
    {NULL}
};

#define NB_VERTICES 3

static int triangle_init(struct ngl_node *node)
{
    struct geometry_priv *s = node->priv_data;

    int ret = ngli_node_geometry_init_attribute(node, NGLI_NODE_GEOMETRY_INDEX_VERTICES, s->triangle_edges, sizeof(s->triangle_edges));
    if (ret < 0)
        return ret;

    ret = ngli_node_geometry_init_attribute(node, NGLI_NODE_GEOMETRY_INDEX_UVCOORDS, s->triangle_uvs, sizeof(s->triangle_uvs));
    if (ret < 0)
        return ret;

    float normals[3 * NB_VERTICES];
    ngli_vec3_normalvec(normals,
                        s->triangle_edges,
                        s->triangle_edges + 3,
                        s->triangle_edges + 6);

    for (int i = 1; i < NB_VERTICES; i++)
        memcpy(normals + (i * 3), normals, 3 * sizeof(*normals));

    ret = ngli_node_geometry_init_attribute(node, NGLI_NODE_GEOMETRY_INDEX_NORMALS, normals, sizeof(normals));
    if (ret < 0)
        return ret;

    s->graphics.topology = NGLI_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    s->graphics.nb_vertices = NB_VERTICES;

    return 0;
}

static void triangle_uninit(struct ngl_node *node)
{
    struct geometry_priv *s = node->priv_data;

    for (int i = 0; i < NGLI_ARRAY_NB(s->buffers); i++)
        ngli_buffer_reset(&s->buffers[i]);
}

const struct node_class ngli_triangle_class = {
    .id        = NGL_NODE_TRIANGLE,
    .name      = "Triangle",
    .init      = triangle_init,
    .uninit    = triangle_uninit,
    .priv_size = sizeof(struct geometry_priv),
    .params    = triangle_params,
    .file      = __FILE__,
};
