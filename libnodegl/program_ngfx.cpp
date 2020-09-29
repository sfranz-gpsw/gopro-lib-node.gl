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

#include "memory.h"
#include "program_ngfx.h"
#include <stdlib.h>
#include "log.h"
#include "graphics/ShaderModule.h"
#include "graphics/ShaderTools.h"
#include "FileUtil.h"
#include "gctx_ngfx.h"
#include <filesystem>
using namespace ngfx;
using namespace std;
namespace fs = std::filesystem;
static ShaderTools shaderTools(true);

struct program *ngli_program_ngfx_create(struct gctx *gctx) {
    program_ngfx *s = (program_ngfx*)ngli_calloc(1, sizeof(*s));
    if (!s)
        return NULL;
    s->parent.gctx = gctx;
    return (struct program *)s;
}

static string compileShader(string src, const string& ext) {
    //patch source bindings
    string tmpFile = string(fs::temp_directory_path()) + "/" + "tmp" + ext;
    FileUtil::writeFile(tmpFile, src);
    string outDir = fs::temp_directory_path();
    auto glslFiles = { tmpFile };
    auto spvFiles = shaderTools.compileShaders(glslFiles, outDir, "glsl", "", ShaderTools::PATCH_SHADER_LAYOUTS_GLSL);
    auto spvMapFiles = shaderTools.generateShaderMaps(glslFiles, outDir, "glsl");
    return FileUtil::splitExt(spvFiles[0])[0];
}

int ngli_program_ngfx_init(struct program *s, const char *vertex, const char *fragment, const char *compute) {
    gctx_ngfx *gctx = (gctx_ngfx  *)s->gctx;
    program_ngfx* program = (program_ngfx*)s;
    if (vertex) program->vs = VertexShaderModule::create(gctx->graphicsContext->device, compileShader(vertex, ".vert")).release();
    if (fragment) program->fs = FragmentShaderModule::create(gctx->graphicsContext->device, compileShader(fragment, ".frag")).release();
    if (compute) program->cs = ComputeShaderModule::create(gctx->graphicsContext->device, compileShader(compute, ".comp")).release();
    return 0;
}
void ngli_program_ngfx_freep(struct program **sp) {
    program_ngfx* program = (program_ngfx*)*sp;
    if (program->vs) { delete program->vs; }
    if (program->fs) { delete program->fs; }
    if (program->cs) { delete program->cs; }
    ngli_freep(sp);
}

