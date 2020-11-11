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

#include "ShaderTools.h"
#include "ngfx/core/FileUtil.h"
#include "ngfx/core/StringUtil.h"
#include "ngfx/core/DebugUtil.h"
#include <filesystem>
#include <regex>
#include <fstream>
#include <sstream>
#include <set>
#include <cctype>
using namespace std;
using namespace ngfx;
using namespace spirv_cross;
auto readFile = FileUtil::readFile;
auto toLower = StringUtil::toLower;
namespace fs = std::filesystem;
#define V(func) { ret = func; if (ret != 0) return ret; }

#ifdef _WIN32
    #define PATCH string("patch.exe")
    #define SPIRV_CROSS string("spirv-cross.exe")
#else
    #define PATCH string("patch")
    #define SPIRV_CROSS string("spirv-cross")
#endif

static string getEnv(const string &name) {
    char* value = getenv(name.c_str());
    return (value ? value : "");
}
static json* getEntry(const json &data, const string &key) {
    auto it = data.find(key);
    if (it == data.end()) return nullptr;
    return (json*)&it.value();
}
ShaderTools::ShaderTools(bool verbose): verbose(verbose) {
    defaultIncludePaths = { "ngfx/data/shaders", "nodegl/data/shaders" };
}

int ShaderTools::cmd(string str) {
    if (verbose) { LOG(">> %s", str.c_str()); }
    else str += " >> /dev/null 2>&1";
    return system(str.c_str());
}

bool ShaderTools::findIncludeFile(const string &includeFilename, const vector<string> &includePaths,
        string &includeFile) {
    for (const string &includePath : includePaths) {
        fs::path filename = includePath  / fs::path(includeFilename);
        if (fs::exists(filename)) {
            includeFile = filename.string();
            return true;
        }
    }
    return false;
}

string ShaderTools::preprocess(const string &dataPath, const string &inFile) {
    string contents = "";
    vector<string> includePaths = defaultIncludePaths;
    includePaths.push_back(dataPath);
    istringstream sstream(readFile(inFile));
    string line;
    while (std::getline(sstream, line)) {
        smatch matchIncludeGroups;
        bool matchInclude = regex_search(line, matchIncludeGroups, regex("#include \"([^\"]*)"));
        if (matchInclude) {
            string includeFilename = matchIncludeGroups[1];
            string includeFilePath;
            findIncludeFile(includeFilename, includePaths, includeFilePath);
            contents += readFile(includeFilePath);
        }
        else {
            contents += line + "\n";
        }
    }
    return contents;
}

int ShaderTools::compileShaderGLSL(const string &inFile, const MacroDefinitions &defines, const string &outFile, bool verbose,
        shaderc_optimization_level optimizationLevel) {
    shaderc::Compiler compiler;
    shaderc::CompileOptions compileOptions;
    for (const MacroDefinition &define : defines) {
        compileOptions.AddMacroDefinition(define.name, define.value);
    }
    compileOptions.SetOptimizationLevel(optimizationLevel);
    compileOptions.SetGenerateDebugInfo();
    static map<string, shaderc_shader_kind> shadercKindMap = {
        { ".vert", shaderc_glsl_default_vertex_shader },
        { ".frag", shaderc_glsl_default_fragment_shader },
        { ".comp", shaderc_glsl_default_compute_shader }
    };
    string src = FileUtil::readFile(inFile);
    auto splitFilename = FileUtil::splitExt(fs::path(inFile).filename().string());
    string ext = splitFilename[1];

    auto result = compiler.CompileGlslToSpv(src, shadercKindMap.at(ext), inFile.c_str(), compileOptions);
    if (result.GetCompilationStatus() == shaderc_compilation_status_success) {
        if (verbose) LOG("compiled file: %s", inFile.c_str());
    }
    else {
        ERR("cannot compile file: %s", inFile.c_str());
        return 1;
    }
    string spv((const char*)result.cbegin(), sizeof(uint32_t) * (result.cend() - result.cbegin()));
    FileUtil::writeFile(outFile, spv);
    return 0;
}

int ShaderTools::removeUnusedVariablesGLSL(const std::string &inFile, const MacroDefinitions &defines, const std::string &outFile) {
    int ret = 0;
    V(compileShaderGLSL(inFile, defines, outFile + ".spv", false));
    std::string spv = FileUtil::readFile(outFile + ".spv");
    spirv_cross::CompilerGLSL compilerGLSL((const uint32_t *)spv.data(), spv.size() / sizeof(uint32_t));
    auto activeVariables = compilerGLSL.get_active_interface_variables();
    compilerGLSL.get_shader_resources(activeVariables);
    compilerGLSL.set_enabled_interface_variables(move(activeVariables));
    string compiledOutput = compilerGLSL.compile();
    FileUtil::writeFile(outFile, compiledOutput);
    return 0;
}

int ShaderTools::patchShaderLayoutsGLSL(const string &inFile, const string &outFile) {
    string output = "";
    istringstream sstream(FileUtil::readFile(inFile));
    string line;
    while (std::getline(sstream, line)) {
        //Patch GLSL shader layouts
        smatch g;

        bool matchLayout = regex_search(line, g,
            regex("^(.*)" "layout\\s*\\(" "([^)]*)" "binding[\\s]*=[\\s]*" "([\\d]+)" "([^)]*)" "\\)" "(.*)\r*$"));
        if (matchLayout) {
            output += g[1].str() + "layout(" + g[2].str() + "set = " + g[3].str() + ", binding = 0" + g[4].str() + ")" + g[5].str() + "\n";
        }
        else {
            output += line + "\n";
        }
    }
    FileUtil::writeFile(outFile, output);
    return 0;
}

int ShaderTools::compileShaderGLSL(string filename, const MacroDefinitions &defines, const string &outDir, vector<string> &outFiles, int flags) {
    string parentPath = fs::path(filename).parent_path().string();
    filename = fs::path(filename).filename().string();
    string inFileName = fs::path(parentPath + "/" + filename).make_preferred().string();
    string outFileName = fs::path(outDir + "/" + filename + ".spv").make_preferred().string();
    if (!FileUtil::srcFileNewerThanOutFile(inFileName, outFileName)) {
        outFiles.push_back(outFileName);
        return 0;
    }
    
    string preprocessFileName;
    int ret = 0;
    if (flags & REMOVE_UNUSED_VARIABLES) {
        preprocessFileName = fs::path(outDir + "/" + "p0_" + filename).make_preferred().string();
        V(removeUnusedVariablesGLSL(inFileName, defines, preprocessFileName));
        inFileName = preprocessFileName;
    }
    if (flags & PATCH_SHADER_LAYOUTS_GLSL) {
        preprocessFileName = fs::path(outDir + "/" + "p1_" + filename).make_preferred().string();
        V(patchShaderLayoutsGLSL(inFileName, preprocessFileName));
        inFileName = preprocessFileName;
    }
    V(compileShaderGLSL(inFileName, defines, outFileName));
    outFiles.push_back(outFileName);
    return 0;
}

int ShaderTools::compileShaderMTL(const string &file, const MacroDefinitions &defines, string outDir, vector<string> &outFiles) {
    string strippedFilename = FileUtil::splitExt(fs::path(file).filename().string())[0];
    string inFileName = fs::path(outDir +"/" + strippedFilename + ".metal").make_preferred().string();
    string outFileName = fs::path(outDir + "/" + strippedFilename + ".metallib").make_preferred().string();
    if (!FileUtil::srcFileNewerThanOutFile(inFileName, outFileName)) {
        outFiles.push_back(outFileName);
        return 0;
    }

    string debugFlags = "-gline-tables-only -MO";
    int result = cmd(
          "xcrun -sdk macosx metal " + debugFlags + " -c " + inFileName + " -o " + outDir + "/" + strippedFilename + ".air && "
          "xcrun -sdk macosx metallib " + outDir + "/" + strippedFilename + ".air -o " + outFileName
    );
    if (result == 0)
        LOG("compiled file: %s", file.c_str());
    else
        ERR("cannot compile file: %s", file.c_str());
    outFiles.push_back(outFileName);
    return result;
}

int ShaderTools::compileShaderHLSL(const string &file, const MacroDefinitions &defines, string outDir, vector<string> &outFiles) {
    string strippedFilename = FileUtil::splitExt(fs::path(file).filename().string())[0];
    string inFileName = fs::path(outDir+"/"+strippedFilename +".hlsl").make_preferred().string();
    string outFileName = fs::path(outDir +"/" +strippedFilename +".dxc").make_preferred().string();
    if (!FileUtil::srcFileNewerThanOutFile(inFileName, outFileName)) {
        outFiles.push_back(outFileName);
        return 0;
    }

    string shaderModel = "";
    if (strstr(inFileName.c_str(), ".vert")) shaderModel = "vs_5_0";
    else if (strstr(inFileName.c_str(), ".frag")) shaderModel = "ps_5_0";
    else if (strstr(inFileName.c_str(), ".comp")) shaderModel = "cs_5_0";
    int result = cmd("dxc.exe /T "+shaderModel+" /Fo "+outFileName+" "+inFileName);
    if (result == 0)
        LOG("compiled file: %s", file.c_str());
    else
        ERR("cannot compile file: %s", file.c_str());
    outFiles.push_back(outFileName);
    return result;
}

int ShaderTools::convertShader(const string &file, const string &extraArgs, string outDir, Format fmt, vector<string> &outFiles) {
    string strippedFilename = FileUtil::splitExt(fs::path(file).filename().string())[0];
    string inFileName = fs::path(outDir + "/" + strippedFilename + ".spv").make_preferred().string();
    string outFileName = fs::path(outDir + "/" + strippedFilename + (fmt == FORMAT_MSL ? ".metal" : ".hlsl")).make_preferred().string();
    if (!FileUtil::srcFileNewerThanOutFile(inFileName, outFileName)) {
        outFiles.push_back(outFileName);
        return 0;
    }

    string args = (fmt == FORMAT_MSL ? "--msl" : "--hlsl --shader-model 60") + extraArgs;
    int result = cmd(SPIRV_CROSS + " " + args + " " + inFileName + " " + "--output" + " " + outFileName);
    if (result == 0)
        LOG("converted file: %s to %s", inFileName.c_str(), outFileName.c_str());
    else
        ERR("cannot convert file: %s", file.c_str());
    outFiles.push_back(outFileName);
    return result;
}

unique_ptr<CompilerReflection> ShaderTools::genShaderReflectionGLSL(const string &file, string outDir) {
    std::string spv = FileUtil::readFile(file);
    return make_unique<CompilerReflection>((const uint32_t *)spv.data(), spv.size() / sizeof(uint32_t));
}

bool ShaderTools::findMetalReflectData(const vector<RegexUtil::Match> &metalReflectData, const string &name, RegexUtil::Match &match) {
    for (const RegexUtil::Match &data: metalReflectData) {
        if (data.s[2] == name) { match = data; return true; }
        else if (strstr(data.s[1].c_str(), name.c_str())) { match = data; return true; }
    }
    return false;
}

json ShaderTools::patchShaderReflectionDataMSL(const string &metalFile, json &reflectData, const string &ext) {
    string metalContents = readFile(metalFile);
    MetalReflectData metalReflectData;
    if (ext == ".vert") {
       metalReflectData.attributes = RegexUtil::findAll(regex("([^\\s]*)[\\s]*([^\\s]*)[\\s]*\\[\\[attribute\\(([0-9]+)\\)\\]\\]"), metalContents);
    }
    metalReflectData.buffers = RegexUtil::findAll(regex("([^\\s]*)[\\s]*([^\\s]*)[\\s]*\\[\\[buffer\\(([0-9]+)\\)\\]\\]"), metalContents);
    metalReflectData.textures = RegexUtil::findAll(regex("([^\\s]*)[\\s]*([^\\s]*)[\\s]*\\[\\[texture\\(([0-9]+)\\)\\]\\]"), metalContents);
    
    json *textures = getEntry(reflectData, "textures"),
        *ubos = getEntry(reflectData, "ubos"),
        *ssbos = getEntry(reflectData, "ssbos"),
        *images = getEntry(reflectData, "images");
    uint32_t numDescriptors = (textures ? textures->size() : 0) + (images ? images->size() : 0) +
        (ubos ? ubos->size() : 0) + (ssbos ? ssbos->size() : 0);

    //update input bindings
    if (ext == ".vert") {
        json* inputs = getEntry(reflectData, "inputs");
        for (json &input : *inputs) {
            RegexUtil::Match metalInputReflectData;
            bool foundMatch = findMetalReflectData(metalReflectData.attributes, input["name"], metalInputReflectData);
            if (!foundMatch) {
                ERR("cannot patch shader reflection data for file: %s", metalFile.c_str());
                return {};
            }
            input["location"] = stoi(metalInputReflectData.s[3]) + numDescriptors;
        }
    }

    //update descriptor bindings
    if (textures) for (json &descriptor : *textures) {
        RegexUtil::Match metalTextureReflectData;
        bool foundMatch = findMetalReflectData(metalReflectData.textures, descriptor["name"], metalTextureReflectData);
        assert(foundMatch);
        descriptor["set"] = stoi(metalTextureReflectData.s[3]);
    }
    if (ubos) for (json &descriptor: *ubos) {
        RegexUtil::Match metalBufferReflectData;
        bool foundMatch = findMetalReflectData(metalReflectData.buffers, descriptor["name"], metalBufferReflectData);
        assert(foundMatch);
        descriptor["set"] = stoi(metalBufferReflectData.s[3]);
    }
    if (ssbos) for (json &descriptor: *ssbos) {
        RegexUtil::Match metalBufferReflectData;
        bool foundMatch = findMetalReflectData(metalReflectData.buffers, descriptor["name"], metalBufferReflectData);
        assert(foundMatch);
        descriptor["set"] = stoi(metalBufferReflectData.s[3]);
    }
    if (images) for (json &descriptor: *images) {
        RegexUtil::Match metalTextureReflectData;
        bool foundMatch = findMetalReflectData(metalReflectData.textures, descriptor["name"], metalTextureReflectData);
        assert(foundMatch);
        descriptor["set"] = stoi(metalTextureReflectData.s[3]);
    }

    return reflectData;
}

json ShaderTools::patchShaderReflectionDataHLSL(const string &hlslFile, json &reflectData, string ext) {
    string hlslContents = readFile(hlslFile);
    HLSLReflectData hlslReflectData;

    //parse semantics
    if (ext == ".vert") {
        json* inputs = getEntry(reflectData, "inputs");
        for (json &input: *inputs) {
            regex p(input["name"].get<string>() + "\\s*:\\s*([^;]*);");
            vector<RegexUtil::Match> hlslReflectData = RegexUtil::findAll(p, hlslContents);
            input["semantic"] = hlslReflectData[0].s[1];
        }
    }

    //get descriptors
    json *textures = getEntry(reflectData, "textures"),
         *ubos = getEntry(reflectData, "ubos"),
         *ssbos = getEntry(reflectData, "ssbos"),
         *images = getEntry(reflectData, "images");
    map<int, json*> descriptors;
    if (textures) for (auto &desc: *textures) descriptors[desc["set"].get<int>()] = &desc;
    if (ubos) for (auto &desc: *ubos) descriptors[desc["set"].get<int>()] = &desc;
    if (ssbos) for  (auto &desc: *ssbos) descriptors[desc["set"].get<int>()] = &desc;
    if (images) for (auto &desc: *images) descriptors[desc["set"].get<int>()] = &desc;

    //patch descriptor bindings
    set<int> sets;
    set<string> samplerTypes = {"sampler2D", "sampler3D", "samplerCube"};
    for (const auto &kv : descriptors) {
        uint32_t set = kv.first;
        json &desc = *kv.second;
        while (sets.find(set) != sets.end()) set += 1;
        desc["set"] = set;
        sets.insert(set);
        if (samplerTypes.find(desc["type"]) != samplerTypes.end())
            sets.insert(set + 1);
    }
    return reflectData;
}

unique_ptr<CompilerReflection> ShaderTools::genShaderReflectionMSL(const string &file, string outDir) {
#if 0 //TDOO
    auto splitFilename = FileUtil::splitExt(fs::path(file).filename().string());
    string strippedFilename = splitFilename[0];
    string inFileName = fs::path(outDir + "/" + strippedFilename + ".spv.reflect").make_preferred().string();
    string outFileName = fs::path(outDir + "/" + strippedFilename + ".metal.reflect").make_preferred().string();
    if (!FileUtil::srcFileNewerThanOutFile(inFileName, outFileName))
        return 0;

    json reflectData = json::parse(readFile(inFileName));

    string ext = FileUtil::splitExt(splitFilename[0])[1];
    string mslFile = fs::path(outDir + "/" + strippedFilename + ".metal").make_preferred().string();
    reflectData = patchShaderReflectionDataMSL(mslFile, reflectData, ext);
    if (reflectData.empty()) {
        ERR("cannot generate reflection map for file: %s", file.c_str());
        return nullptr;
    }
        
    FileUtil::writeFile(outFileName, reflectData.dump(4));

    LOG("generated reflection map: %s", outFileName.c_str());
    return reflectData;
#endif
    return nullptr;
}

unique_ptr<CompilerReflection> ShaderTools::genShaderReflectionHLSL(const string &file, string outDir) {
#if 0 //TODO
    auto splitFilename = FileUtil::splitExt(fs::path(file).filename().string());
    string strippedFilename = splitFilename[0];
    string inFileName = fs::path(outDir + "/" + strippedFilename + ".spv.reflect").make_preferred().string();
    string outFileName = fs::path(outDir + "/" + strippedFilename + ".hlsl.reflect").make_preferred().string();
    if (!FileUtil::srcFileNewerThanOutFile(inFileName, outFileName))
        return 0;

    json reflectData = json::parse(readFile(inFileName));

    string ext = FileUtil::splitExt(splitFilename[0])[1];
    string hlslFile = fs::path(outDir + "/" + strippedFilename + ".hlsl").make_preferred().string();
    reflectData = patchShaderReflectionDataHLSL(hlslFile, reflectData, ext);
    if (reflectData.empty()) {
        ERR("cannot generate reflection map for file: %s", file.c_str());
        return 1;
    }

    FileUtil::writeFile(outFileName, reflectData.dump(4));

    LOG("generated reflection map: %s", outFileName.c_str());
    return 0;
#endif
    return nullptr;
}

static string toGLSLType(const SPIRType& type) {
    if (type.vecsize == 1 && type.columns == 1) {
        switch (type.basetype) {
        case SPIRType::Float:
            return "float";
        case SPIRType::Int:
            return "int";
        default:
            ERR();
        }
    }
    else if (type.vecsize > 1 && type.columns == 1) {
        switch (type.basetype) {
        case SPIRType::Float:
            return "vec" + to_string(type.vecsize);
        case SPIRType::Int:
            return "ivec" + to_string(type.vecsize);
        default:
            ERR();
        }
    }
    else if (type.vecsize == type.columns) {
        switch (type.basetype) {
        case SPIRType::Float:
            return "mat" + to_string(type.vecsize);
        case SPIRType::Int:
            return "imat" + to_string(type.vecsize);
        default:
            ERR();
        }
    }
    ERR();
    return "";
}

string ShaderTools::parseReflectionData(const CompilerReflection *reflectData, string ext) {
    string contents = "";
    auto get = [&](ID id, spv::Decoration decoration) { return reflectData->get_decoration(id, decoration); };
    const auto &shaderResources = reflectData->get_shader_resources();
    if (ext == ".vert") {
        const auto &inputs = shaderResources.stage_inputs;
        contents += "INPUT_ATTRIBUTES "+to_string(inputs.size())+"\n";
        for (const auto &input: inputs) {
            string inputName = input.name;
            string inputSemantic = "";
            string inputNameLower = toLower(inputName);
            inputSemantic = "UNDEFINED";
            //if (input.find("semantic") != input.end()) inputSemantic = input["semantic"];
            map<string, string> inputTypeMap = {
                { "float", "VERTEXFORMAT_FLOAT" }, { "vec2", "VERTEXFORMAT_FLOAT2" }, { "vec3" , "VERTEXFORMAT_FLOAT3" }, { "vec4", "VERTEXFORMAT_FLOAT4" },
                { "ivec2", "VERTEXFORMAT_INT2" }, { "ivec3", "VERTEXFORMAT_INT3" }, { "ivec4", "VERTEXFORMAT_INT4" },
                { "mat2", "VERTEXFORMAT_MAT2" }, { "mat3", "VERTEXFORMAT_MAT3" }, { "mat4", "VERTEXFORMAT_MAT4" }
            };
            const auto &spirType = reflectData->get_type(input.type_id);
            string inputType = inputTypeMap[toGLSLType(spirType)];
            int inputLocation = get(input.id, spv::DecorationLocation);
            contents += "\t"+inputName+" "+inputSemantic+" "+to_string(inputLocation)+" "+inputType+"\n";
        }
    }
    shaderResources.sampled_images;
    const auto &textures = shaderResources.sampled_images,
        &ubos = shaderResources.uniform_buffers,
        &ssbos = shaderResources.storage_buffers,
        &images = shaderResources.storage_images;
    const auto &types = getEntry(reflectData, "types");
    json uniformBufferInfos;
    json shaderStorageBufferInfos;

    std::function<void(const json&, json&, uint32_t, string)> parseMembers = [&](const json &membersData, json &members, uint32_t baseOffset = 0, string baseName = "") {
        for (const json &memberData: membersData) {
            const map<string, int> typeSizeMap = {
               {"int",4}, {"uint",4}, {"float",4},
               {"vec2",8}, {"vec3",12}, {"vec4",16},
               {"ivec2",8}, {"ivec3",12}, {"ivec4",16},
               {"uvec2",8}, {"uvec3",12}, {"uvec4",16},
               {"mat2",16}, {"mat3",36}, {"mat4",64}
            };
            string memberType = memberData["type"];
            if (typeSizeMap.find(memberType) != typeSizeMap.end()) {
                json member = memberData;
                member["name"] = baseName + member["name"].get<string>();
                member["size"] = typeSizeMap.at(memberType);
                member["offset"] = member["offset"].get<int>() + baseOffset;
                member["array_count"] = (member.find("array") != member.end()) ? member["array"][0].get<int>() : 0;
                member["array_stride"] = (member.find("array_stride") != member.end()) ? member["array_stride"].get<int>(): 0;
                members.push_back(member);
            }
            else if (types->find(memberType) != types->end()) {
                const json &type = (*types)[memberType];
                parseMembers(
                    type["members"],
                    members,
                    baseOffset + memberData["offset"].get<int>(),
                    baseName + memberData["name"].get<string>() + "."
                );
            }
            else ERR("unrecognized type: {memberType}");
        }
    };

    auto parseBuffers = [&](const SmallVector<Resource> &buffers, json &bufferInfos) {
        for (const Resource &buffer: buffers) {
            const json &bufferType = (*types)[buffer["type"].get<string>()];
            json bufferMembers = {};
            parseMembers(bufferType["members"], bufferMembers, 0, "");
            json bufferInfo = {
                { "name", buffer.name },
                { "set", get(buffer.id, spv::DecorationDescriptorSet) },
                { "binding", get(buffer.id, spv::DecorationBinding) },
                { "members", bufferMembers }
            };
            bufferInfos.push_back(bufferInfo);
        }
    };
    if (!ubos.empty()) parseBuffers(ubos, uniformBufferInfos);
    if (!ssbos.empty()) parseBuffers(ssbos, shaderStorageBufferInfos);

    json textureDescriptors = {};
    json bufferDescriptors = {};
    if (!textures.empty()) {
        for (const Resource &texture: textures) {
            textureDescriptors[to_string(get(texture.id, spv::DecorationDescriptorSet))] = {
                { "type", texture["type"] },
                { "name", texture["name"] },
                { "set", texture["set"] },
                { "binding", texture["binding"] }
            };
        }
    }
    if (images) for (const json &image: *images) {
        textureDescriptors[to_string(image["set"].get<int>())] = {
            { "type", image["type"] },
            { "name", image["name"] },
            { "set", image["set"] },
            { "binding", image["binding"] }
        };
    }
    if (ubos) for (const json &ubo: *ubos) {
        bufferDescriptors[to_string(ubo["set"].get<int>())] = {
            { "type", "uniformBuffer" },
            { "name", ubo["name"] },
            { "set", ubo["set"] },
            { "binding", ubo["binding"] }
        };
    }
    if (ssbos) for (const json &ssbo: *ssbos) {
        bufferDescriptors[to_string(ssbo["set"].get<int>())] = {
            { "type", "shaderStorageBuffer" },
            { "name", ssbo["name"] },
            { "set", ssbo["set"] },
            { "binding", ssbo["binding"] }
        };
    }
    contents += "DESCRIPTORS "+to_string(textureDescriptors.size() + bufferDescriptors.size()) + "\n";
    map<string, string> descriptorTypeMap = {
        { "sampler2D", "DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER" },
        { "sampler3D", "DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER" },
        { "samplerCube", "DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER" },
        { "image2D", "DESCRIPTOR_TYPE_STORAGE_IMAGE" },
        { "uniformBuffer", "DESCRIPTOR_TYPE_UNIFORM_BUFFER" },
        { "shaderStorageBuffer", "DESCRIPTOR_TYPE_STORAGE_BUFFER" }
    };
    for (auto &[key, val]: textureDescriptors.items()) {
        string descriptorType = descriptorTypeMap[val["type"]];
        contents += "\t" + val["name"].get<string>() + " " + descriptorType + " " + to_string(val["set"].get<int>()) + "\n";
    }
    for (auto &[key, val]: bufferDescriptors.items()) {
        string descriptorType = descriptorTypeMap[val["type"]];
        contents += "\t" + val["name"].get<string>() + " " + descriptorType + " " + to_string(val["set"].get<int>()) + "\n";
    }
    auto processBufferInfos = [&](const json &bufferInfo) -> string {
        string contents = "";
        const json &memberInfos = bufferInfo["members"];
        contents += bufferInfo["name"].get<string>() + " " + to_string(bufferInfo["set"].get<int>()) + " " + to_string(memberInfos.size()) + "\n";
        for (const json &m: memberInfos) {
            contents += m["name"].get<string>() + " " + to_string(m["offset"].get<int>()) + " " + to_string(m["size"].get<int>()) + " " + to_string(m["array_count"].get<int>()) + " " + to_string(m["array_stride"].get<int>()) + "\n";
        }
        return contents;
    };
    contents += "UNIFORM_BUFFER_INFOS "+to_string(uniformBufferInfos.size())+"\n";
    for (const json &bufferInfo: uniformBufferInfos) {
        contents += processBufferInfos(bufferInfo);
    }

    contents += "SHADER_STORAGE_BUFFER_INFOS "+ to_string(shaderStorageBufferInfos.size())+"\n";
    for (const json &bufferInfo: shaderStorageBufferInfos) {
        contents += processBufferInfos(bufferInfo);
    }
    return contents;
}

int ShaderTools::generateShaderMapGLSL(const string &file, string outDir, vector<string> &outFiles) {
    string filename = fs::path(file).filename().string();
    string ext = FileUtil::splitExt(filename)[1];

    string inFileName = fs::path(outDir + "/" + filename + ".spv").make_preferred().string();
    string outFileName = fs::path(outDir + "/" + filename + ".spv.map").make_preferred().string();
    if (!FileUtil::srcFileNewerThanOutFile(inFileName, outFileName)) {
        outFiles.push_back(outFileName);
        return 0;
    }

    auto reflectData = genShaderReflectionGLSL(inFileName, outDir);
    string contents = parseReflectionData(reflectData.get(), ext);

    FileUtil::writeFile(outFileName, contents);
    outFiles.push_back(outFileName);
    return 0;
}

int ShaderTools::generateShaderMapMSL(const string &file, string outDir, vector<string> &outFiles) {
    auto splitFilename = FileUtil::splitExt(fs::path(file).filename().string());
    string filename = splitFilename[0];
    string ext = FileUtil::splitExt(splitFilename[0])[1];

    string inFileName = fs::path(outDir + "/" + filename + ".metal").make_preferred().string();
    string outFileName = fs::path(outDir + "/" + filename + ".metal.map").make_preferred().string();
    if (!FileUtil::srcFileNewerThanOutFile(inFileName, outFileName)) {
        outFiles.push_back(outFileName);
        return 0;
    }

    auto reflectData = genShaderReflectionMSL(file, outDir);
    string contents = parseReflectionData(reflectData.get(), ext);

    FileUtil::writeFile(outFileName, contents);
    outFiles.push_back(outFileName);
    return 0;
}

int ShaderTools::generateShaderMapHLSL(const string &file, string outDir, vector<string> &outFiles) {
    string dataPath = fs::path(file).parent_path().string();
    auto splitFilename = FileUtil::splitExt(fs::path(file).filename().string());
    string filename = splitFilename[0];
    string ext = FileUtil::splitExt(splitFilename[0])[1];

    string inFileName = fs::path(outDir + "/" + filename + ".hlsl.reflect").make_preferred().string();
    string outFileName = fs::path(outDir + "/" + filename + ".hlsl.map").make_preferred().string();
    if (!FileUtil::srcFileNewerThanOutFile(inFileName, outFileName)) {
        outFiles.push_back(outFileName);
        return 0;
    }

    auto reflectData = genShaderReflectionHLSL(file, outDir);
    string contents = parseReflectionData(reflectData.get(), ext);

    FileUtil::writeFile(outFileName, contents);
    outFiles.push_back(outFileName);
    return 0;
}

vector<string> ShaderTools::convertShaders(const vector<string> &files, string outDir, Format fmt) {
    vector<string> outFiles;
    for (const string &file: files) convertShader(file, "", outDir, fmt, outFiles);
    return outFiles;
}

vector<string> ShaderTools::compileShaders(const vector<string> &files, string outDir, Format fmt, const MacroDefinitions &defines, int flags) {
#ifdef GRAPHICS_BACKEND_VULKAN
    defines += " -DGRAPHICS_BACKEND_VULKAN=1";
#endif
    vector<string> outFiles;
    for (const string &file: files) {
        if (fmt == FORMAT_GLSL) compileShaderGLSL(file, defines, outDir, outFiles, flags);
        else if (fmt == FORMAT_MSL) compileShaderMTL(file, defines, outDir, outFiles);
        else if (fmt == FORMAT_HLSL) compileShaderHLSL(file, defines, outDir, outFiles);
    }
    return outFiles;
}

void ShaderTools::applyPatches(const vector<string> &patchFiles, string outDir) {
    for (const string &patchFile: patchFiles) {
        string filename = FileUtil::splitExt(fs::path(patchFile).string())[0];
        LOG("filename: %s", filename.c_str());
        string outFile = fs::path(outDir+"/"+filename).make_preferred().string();
        if (fs::exists(outFile)) {
            LOG("applying patch: {patchFile}");
            string cmdStr = PATCH+" -N -u "+outFile+" -i "+patchFile;
            cmd(cmdStr);
        }
    }
}

vector<string> ShaderTools::generateShaderMaps(const vector<string> &files, string outDir, Format fmt) {
    vector<string> outFiles;
    for (const string &file: files) {
        if (fmt == FORMAT_GLSL) generateShaderMapGLSL(file, outDir, outFiles);
        else if (fmt == FORMAT_MSL) generateShaderMapMSL(file, outDir, outFiles);
        else if (fmt == FORMAT_HLSL) generateShaderMapHLSL(file, outDir, outFiles);
    }
    return outFiles;
}
