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
#include "FileUtil.h"
#include "RegexUtil.h"
#include "StringUtil.h"
#include <filesystem>
#include <regex>
#include <fstream>
#include <set>
#include <cctype>
using namespace std;
using namespace ngfx;
auto readFile = FileUtil::readFile;
auto toLower = StringUtil::toLower;
namespace fs = std::filesystem;

#ifdef _WIN32
    #define PATCH string("patch.exe")
    #define GLSLANG_VALIDATOR string("glslangValidator.exe")
    #define SPIRV_CROSS string("spirv-cross.exe")
#else
    #define PATCH string("patch")
    #define GLSLANG_VALIDATOR string("glslangValidator")
    #define SPIRV_CROSS string("spirv-cross")
#endif

ShaderTools::ShaderTools() {
    verbose = (string(getenv("V"))=="1");
    defaultIncludePaths = { "ngfx/data/shaders", "nodegl/data/shaders" };
}

int ShaderTools::cmd(string str) {
    if (verbose) { printf(">> %s\n", str.c_str()); }
    else str += " > /dev/null 2>&1";
    return system(str.c_str());
}

bool ShaderTools::findIncludeFile(const string& includeFilename, const vector<string> &includePaths,
        string& includeFile) {
    for (const string& includePath : includePaths) {
        fs::path filename = includePath  / fs::path(includeFilename);
        if (fs::exists(filename)) {
            includeFile = filename;
            return true;
        }
    }
    return false;
}

string ShaderTools::preprocess(const string& dataPath, const string& inFile) {
    string contents = "";
    vector<string> includePaths = defaultIncludePaths;
    includePaths.push_back(dataPath);
    istringstream sstream(inFile);
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
            contents += line;
        }
    }
    return contents;
}

int ShaderTools::compileShaderGLSL(string filename, const string& defines, const string& outDir, vector<string>& outFiles) {
    string parentPath = fs::path(filename).parent_path();
    filename = fs::path(filename).filename();
    string inFileName = fs::path(parentPath + "/" + filename);
    string outFileName = fs::path(outDir + "/" + filename + ".spv");
    if (FileUtil::srcFileChanged(inFileName, outFileName)) return 0;

    ofstream outFile(fs::path(outDir + "/" + filename));
    string contents = preprocess(parentPath, inFileName);
    outFile<< contents;
    outFile.close();
    int result = cmd(GLSLANG_VALIDATOR+" " + defines + " -V "+
                     fs::path(outDir + "/" + filename).string()+
                     fs::path(" -o"+ outFileName).string());
    if (result == 0) {
        printf("compiled file: %s\n", filename.c_str());
    }
    else {
        fprintf(stderr, "ERROR: cannot compile file: %s\n", filename.c_str());
    }
    outFiles.push_back(outFileName);
    return result;
}

int ShaderTools::compileShaderMTL(const string& file, const string& defines, string outDir, vector<string> &outFiles) {
    string strippedFilename = FileUtil::splitExt(fs::path(file).filename())[0];
    string inFileName = fs::path(outDir +"/" + strippedFilename + ".metal");
    string outFileName = fs::path(outDir + "/" + strippedFilename + ".metallib");
    if (FileUtil::srcFileChanged(inFileName, outFileName)) return 0;

    string debugFlags = "-gline-tables-only -MO";
    int result = cmd(
          "xcrun -sdk macosx metal {debugFlags} -c {inFileName} -o {outDir}/{filename}.air && "
          "xcrun -sdk macosx metallib {outDir}/{filename}.air -o {outFileName}"
    );
    if (result == 0)
        printf("compiled file: %s\n", file.c_str());
    else
        fprintf(stderr, "ERROR: cannot compile file: %s\n", file.c_str());
    outFiles.push_back(outFileName);
    return result;
}

int ShaderTools::compileShaderHLSL(const string& file, const string& defines, string outDir, vector<string>& outFiles) {
    string strippedFilename = FileUtil::splitExt(fs::path(file).filename())[0];
    string inFileName = outDir+"/"+strippedFilename +".hlsl";
    string outFileName = outDir +"/" +strippedFilename +".dxc";
    if (FileUtil::srcFileChanged(inFileName, outFileName)) return 0;

    string shaderModel = "";
    if (strstr(inFileName.c_str(), ".vert")) shaderModel = "vs_5_0";
    else if (strstr(inFileName.c_str(), ".frag")) shaderModel = "ps_5_0";
    else if (strstr(inFileName.c_str(), ".comp")) shaderModel = "cs_5_0";
    int result = cmd("dxc.exe /T "+shaderModel+" /Fo "+outFileName+" "+inFileName);
    if (result == 0)
        printf("compiled file: %s\n", file.c_str());
    else
        fprintf(stderr, "ERROR: cannot compile file: %s\n", file.c_str());
    outFiles.push_back(outFileName);
    return result;
}

int ShaderTools::convertShader(const string& file, const string& extraArgs, string outDir, string fmt, vector<string>& outFiles) {
    string strippedFilename = FileUtil::splitExt(fs::path(file).filename())[0];
    string inFileName = fs::path(outDir + "/" + strippedFilename + ".spv");
    string outFileName = fs::path(outDir + "/" + strippedFilename + (fmt == "msl" ? ".metal" : ".hlsl"));
    if (FileUtil::srcFileChanged(inFileName, outFileName)) return 0;

    string args = (fmt == "msl" ? "--msl" : "--hlsl --shader-model 60") + extraArgs;
    int result = cmd(SPIRV_CROSS + " " + args + " " + inFileName + " " + "--output" + " " + outFileName);
    if (result == 0)
        printf("converted file: %s to %s\n", inFileName.c_str(), outFileName.c_str());
    else
        fprintf(stderr, "ERROR: cannot convert file: %s\n", file.c_str());
    outFiles.push_back(outFileName);
    return result;
}

int ShaderTools::genShaderReflectionGLSL(const string& file, string outDir) {
    string filename = FileUtil::splitExt(fs::path(file).filename())[0];
    string inFileName = outDir + "/" + filename + ".spv";
    string outFileName = outDir + "/" + filename + ".spv.reflect";
    if (FileUtil::srcFileChanged(inFileName, outFileName)) return 0;

    int result = cmd(SPIRV_CROSS+" "+inFileName+" --reflect --output "+outFileName);

    inFileName = outDir + "/" + filename + ".spv.reflect";
    outFileName = inFileName;

    auto reflectData = json::parse(readFile(inFileName));

    ofstream outFile(outFileName);
    string contents = reflectData.dump(4);
    outFile<<contents;
    outFile.close();

    if (result == 0)
        printf("generated reflection map for file: %s\n", file.c_str());
    else {
        fprintf(stderr, "ERROR: cannot generate reflection map for file: %s", file.c_str());
    }

    return result;
}

bool ShaderTools::findMetalReflectData(const vector<smatch>& metalReflectData, const string& name, smatch& match) {
    for (const smatch& data: metalReflectData) {
        if (data.str(1) == name) { match = data; return true; }
        else if (strstr(data[0].str().c_str(), name.c_str())) { match = data; return true; }
    }
    return false;
}

json ShaderTools::patchShaderReflectionDataMSL(const string& metalFile, json& reflectData, const string& ext) {
    string metalContents = readFile(metalFile);
    MetalReflectData metalReflectData;
    if (ext == ".vert") {
       metalReflectData.attributes = RegexUtil::findAll(regex("([^\\s]*)[\\s]*([^\\s]*)[\\s]*\\[\\[attribute\\(([0-9]+)\\)\\]\\]"), metalContents);
    }
    metalReflectData.buffers = RegexUtil::findAll(regex("([^\\s]*)[\\s]*([^\\s]*)[\\s]*\\[\\[buffer\(([0-9]+)\\)\\]\\]"), metalContents);
    metalReflectData.textures = RegexUtil::findAll(regex("([^\\s]*)[\\s]*([^\\s]*)[\\s]*\\[\\[texture\(([0-9]+)\\)\\]\\]"), metalContents);

    json &textures = reflectData["textures"],
               &ubos = reflectData["ubos"],
               &ssbos = reflectData["ssbos"],
               &images = reflectData["images"],
               &types = reflectData["types"];
    uint32_t numDescriptors = textures.size() + images.size() + ubos.size() + ssbos.size();

    //update input bindings
    if (ext == ".vert") {
        json& inputs = reflectData["inputs"];
        for (json& input : inputs) {
            smatch metalInputReflectData;
            bool foundMatch = findMetalReflectData(metalReflectData.attributes, input["name"], metalInputReflectData);
            if (!foundMatch) {
                fprintf(stderr, "ERROR: cannot patch shader reflection data for file: %s", metalFile.c_str());
                return {};
            }
            input["location"] = stoi(metalInputReflectData.str(2)) + numDescriptors;
        }
    }

    //update descriptor bindings
    for (json& descriptor : textures) {
        smatch metalTextureReflectData;
        bool foundMatch = findMetalReflectData(metalReflectData.textures, descriptor["name"], metalTextureReflectData);
        assert(foundMatch);
        descriptor["set"] = stoi(metalTextureReflectData.str(2));
    }
    for (json& descriptor: ubos) {
        smatch metalBufferReflectData;
        bool foundMatch = findMetalReflectData(metalReflectData.buffers, descriptor["name"], metalBufferReflectData);
        assert(foundMatch);
        descriptor["set"] = stoi(metalBufferReflectData.str(2));
    }
    for (json& descriptor: ssbos) {
        smatch metalBufferReflectData;
        bool foundMatch = findMetalReflectData(metalReflectData.buffers, descriptor["name"], metalBufferReflectData);
        assert(foundMatch);
        descriptor["set"] = stoi(metalBufferReflectData.str(2));
    }
    for (json& descriptor: images) {
        smatch metalTextureReflectData;
        bool foundMatch = findMetalReflectData(metalReflectData.textures, descriptor["name"], metalTextureReflectData);
        descriptor["set"] = stoi(metalTextureReflectData.str(2));
    }

    return reflectData;
}

json ShaderTools::patchShaderReflectionDataHLSL(const string& hlslFile, json& reflectData, string ext) {
    string hlslContents = readFile(hlslFile);
    HLSLReflectData hlslReflectData;

    //parse semantics
    if (ext == ".vert") {
        json& inputs = reflectData["inputs"];
        for (json& input: inputs) {
            regex p(input["name"].get<string>() + "\\s*:\\s*([^;]*);");
            vector<smatch> hlslReflectData = RegexUtil::findAll(p, hlslContents);
            input["semantic"] = stoi(hlslReflectData[0].str(0));
        }
    }

    //get descriptors
    json &textures = reflectData["textures"],
         &ubos = reflectData["ubos"],
         &ssbos = reflectData["ssbos"],
         &images = reflectData["images"];
    uint32_t numDescriptors = textures.size() + images.size() + ubos.size() + ssbos.size();
    json descriptors;
    for (const auto& desc: textures) descriptors[desc["set"].get<int>()] = desc;
    for (const auto& desc: textures) descriptors[desc["set"].get<int>()] = desc;
    for (const auto& desc: ubos) descriptors[desc["set"].get<int>()] = desc;
    for (const auto& desc: ssbos) descriptors[desc["set"].get<int>()] = desc;
    for (const auto& desc: images) descriptors[desc["set"].get<int>()] = desc;

    //patch descriptor bindings
    set<int> sets;
    set<string> samplerTypes = {"sampler2D", "sampler3D", "samplerCube"};
    for (const int& setValue: descriptors) {
        int set = setValue;
        json& desc = descriptors[set];
        while (sets.find(set) != sets.end()) set += 1;
        desc["set"] = set;
        sets.insert(set);
        if (samplerTypes.find(desc["type"]) != samplerTypes.end())
            sets.insert(set + 1);
    }
    return reflectData;
}

int ShaderTools::genShaderReflectionMSL(const string& file, string outDir) {
    auto splitFilename = FileUtil::splitExt(fs::path(file).filename());
    string strippedFilename = splitFilename[0];
    string inFileName = outDir + "/" + strippedFilename + ".spv.reflect";
    string outFileName = outDir + "/" + strippedFilename + ".metal.reflect";
    if (FileUtil::srcFileChanged(inFileName, outFileName)) return 0;

    json reflectData = json::parse(readFile(inFileName));

    string ext = FileUtil::splitExt(fs::path(file).filename())[1];
    reflectData = patchShaderReflectionDataMSL(file, reflectData, ext);
    if (reflectData.empty()) {
        fprintf(stderr, "ERROR: cannot generate reflection map for file: %s\n", file.c_str());
        return 1;
    }
        
    ofstream outFile(outFileName);
    string contents = reflectData.dump();
    outFile<<contents;
    outFile.close();

    printf("generated reflection map for file: %s\n", file.c_str());
    return 0;
}

int ShaderTools::genShaderReflectionHLSL(const string& file, string outDir) {
    auto splitFilename = FileUtil::splitExt(fs::path(file).filename());
    string strippedFilename = splitFilename[0];
    string inFileName = outDir + "/" + strippedFilename + ".spv.reflect";;
    string outFileName = outDir + "/" + strippedFilename + ".hlsl.reflect";
    if (FileUtil::srcFileChanged(inFileName, outFileName)) return 0;

    json reflectData = json::parse(readFile(inFileName));

    string ext = FileUtil::splitExt(fs::path(file).filename())[1];
    reflectData = patchShaderReflectionDataHLSL(file, reflectData, ext);
    if (reflectData.empty()) {
        fprintf(stderr, "ERROR: cannot generate reflection map for file: %s\n", file.c_str());
        return 1;
    }

    ofstream outFile(outFileName);
    string contents = reflectData.dump();
    outFile<<contents;
    outFile.close();

    printf("generated reflection map for file: %s\n", file.c_str());
    return 0;
}

string ShaderTools::parseReflectionData(const json& reflectData, string ext) {
    string contents = "";
    if (ext == ".vert") {
        const json& inputs = reflectData["inputs"];
        contents += "INPUT_ATTRIBUTES "+to_string(inputs.size())+"\n";
        for (const json& input: inputs) {
            string inputName = input["name"];
            string inputSemantic = "";
            string inputNameLower = toLower(inputName);
            inputSemantic = "UNDEFINED";
            if (input.find("semantic") != input.end()) inputSemantic = input["semantic"];
            map<string, string> inputTypeMap = {
                { "float", "VERTEXFORMAT_FLOAT" }, { "vec2", "VERTEXFORMAT_FLOAT2" }, { "vec3" , "VERTEXFORMAT_FLOAT3" }, { "vec4", "VERTEXFORMAT_FLOAT4" },
                { "ivec2", "VERTEXFORMAT_INT2" }, { "ivec3", "VERTEXFORMAT_INT3" }, { "ivec4", "VERTEXFORMAT_INT4" },
                { "mat2", "VERTEXFORMAT_MAT2" }, { "mat3", "VERTEXFORMAT_MAT3" }, { "mat4", "VERTEXFORMAT_MAT4" }
            };
            string inputType = inputTypeMap[input["type"]];
            contents += "\t"+inputName+" "+inputSemantic+" "+input["location"].get<string>()+" "+inputType+"\n";
        }
    }
    const json &textures = reflectData["textures"],
         &ubos = reflectData["ubos"],
         &ssbos = reflectData["ssbos"],
         &images = reflectData["images"],
         &types = reflectData["types"];
    json uniformBufferInfos;
    json shaderStorageBufferInfos;

    std::function<void(const json&, json&, uint32_t, string)> parseMembers = [&](const json& membersData, json& members, uint32_t baseOffset = 0, string baseName = "") {
        for (const json& memberData: membersData) {
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
                member["offset"] += baseOffset;
                member["array_count"] = (member.find("array") != member.end()) ? member["array"][0].get<int>() : 0;
                member["array_stride"] = (member.find("array_stride") != member.end()) ? member["array_stride"].get<int>(): 0;
                members.push_back(member);
            }
            else if (types.find(memberType) != types.end()) {
                const json& type = types[memberType];
                parseMembers(
                    type["members"],
                    members,
                    baseOffset + memberData["offset"].get<int>(),
                    baseName + memberData["name"].get<string>() + "."
                );
            }
            else fprintf(stderr, "ERROR: unrecognized type: {memberType}");
        }
    };

    auto parseBuffers = [&](const json& buffers, json& bufferInfos) {
#if 0 //TODO
        for buffer in buffers:
            bufferType = types[buffer['type']]
            bufferMembers = []
            ParseReflectionUtil::parseMembers(bufferType['members'], bufferMembers)
            bufferInfo = {'name':buffer['name'], 'set': buffer['set'], 'binding': buffer['binding'], 'members': bufferMembers}
            bufferInfos.append(bufferInfo)
#endif
    };
    parseBuffers(ubos, uniformBufferInfos);
    parseBuffers(ssbos, shaderStorageBufferInfos);

    json textureDescriptors = {};
    json bufferDescriptors = {};
    for (const json& texture: textures) {
        textureDescriptors[texture["set"].get<int>()] = {
            { "type", texture["type"] },
            { "name", texture["name"] },
            { "set", texture["set"] },
            { "binding", texture["binding"] }
        };
    }
    for (const json& image: images) {
        textureDescriptors[image["set"].get<int>()] = {
            { "type", image["type"] },
            { "name", image["name"] },
            { "set", image["set"] },
            { "binding", image["binding"] }
        };
    }
    for (const json& ubo: ubos) {
        bufferDescriptors[ubo["set"].get<int>()] = {
            { "type", "uniformBuffer" },
            { "name", ubo["name"] },
            { "set", ubo["set"] },
            { "binding", ubo["binding"] }
        };
    }
    for (const json& ssbo: ssbos) {
        bufferDescriptors[ssbo["set"].get<int>()] = {
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
    for (int set : textureDescriptors) {
        const json& descriptor = textureDescriptors[set];
        string descriptorType = descriptorTypeMap[descriptor["type"]];
        contents += "\t" + descriptor["name"].get<string>() + " " + descriptorType + " " + descriptor["set"].get<string>() + "\n";
    }
    for (int set : bufferDescriptors) {
        const json& descriptor = bufferDescriptors[set];
        string descriptorType = descriptorTypeMap[descriptor["type"]];
        contents += "\t" + descriptor["name"].get<string>() + " " + descriptorType + " " + descriptor["set"].get<string>() + "\n";
    }
    auto processBufferInfos = [&](const json& bufferInfo) -> string {
        string contents = "";
        const json& memberInfos = bufferInfo["members"];
        contents += bufferInfo["name"].get<string>() + " " + bufferInfo["set"].get<string>() + " " + to_string(memberInfos.size()) + "\n";
        for (const json& m: memberInfos) {
            contents += m["name"].get<string>() + " " + m["offset"].get<string>() + " " + m["size"].get<string>() + " " + m["array_count"].get<string>() + " " + m["array_stride"].get<string>() + "\n";
        }
        return contents;
    };
    contents += "UNIFORM_BUFFER_INFOS "+to_string(uniformBufferInfos.size())+"\n";
    for (const json& bufferInfo: uniformBufferInfos) {
        contents += processBufferInfos(bufferInfo);
    }

    contents += "SHADER_STORAGE_BUFFER_INFOS "+ to_string(shaderStorageBufferInfos.size())+"\n";
    for (const json& bufferInfo: shaderStorageBufferInfos) {
        contents += processBufferInfos(bufferInfo);
    }
    return contents;
}

int ShaderTools::generateShaderMapGLSL(const string& file, string outDir, vector<string>& outFiles) {
    genShaderReflectionGLSL(file, outDir);
    string dataPath = fs::path(file).parent_path();
    string filename = fs::path(file).filename();
    string ext = FileUtil::splitExt(filename)[1];

    string inFileName = fs::path(outDir + "/" + filename + ".spv.reflect");
    string outFileName = fs::path(outDir + "/" + filename + ".spv.map");
    if (FileUtil::srcFileChanged(inFileName, outFileName)) return 0;

    auto reflectData = json::parse(readFile(inFileName));
    string contents = parseReflectionData(reflectData, ext);

    ofstream outFile(outFileName);
    outFile<<contents;
    outFile.close();
    outFiles.push_back(outFileName);
    return 0;
}

int ShaderTools::generateShaderMapMSL(const string& file, string outDir, vector<string>& outFiles) {
    genShaderReflectionMSL(file, outDir);
    string dataPath = fs::path(file).parent_path();
    auto splitFilename = FileUtil::splitExt(fs::path(file).filename());
    string filename = splitFilename[0];
    string ext = splitFilename[1];

    string inFileName = fs::path(outDir + "/" + filename + ".metal.reflect");
    string outFileName = fs::path(outDir + "/" + filename + ".metal.map");
    if (FileUtil::srcFileChanged(inFileName, outFileName)) return 0;

    auto reflectData = json::parse(readFile(inFileName));
    string contents = parseReflectionData(reflectData, ext);

    ofstream outFile(outFileName);
    outFile<<contents;
    outFile.close();
    outFiles.push_back(outFileName);
    return 0;
}

int ShaderTools::generateShaderMapHLSL(const string& file, string outDir, vector<string>& outFiles) {
    genShaderReflectionHLSL(file, outDir);
    string dataPath = fs::path(file).parent_path();
    auto splitFilename = FileUtil::splitExt(fs::path(file).filename());
    string filename = splitFilename[0];
    string ext = splitFilename[1];

    string inFileName = fs::path(outDir + "/" + filename + ".hlsl.reflect");
    string outFileName = fs::path(outDir + "/" + filename + ".hlsl.map");
    if (FileUtil::srcFileChanged(inFileName, outFileName)) return 0;

    auto reflectData = json::parse(readFile(inFileName));
    string contents = parseReflectionData(reflectData, ext);

    ofstream outFile(outFileName);
    outFile<<contents;
    outFile.close();
    outFiles.push_back(outFileName);
    return 0;
}

vector<string> ShaderTools::convertShaders(const vector<string> &files, string outDir, string fmt) {
    vector<string> outFiles;
    for (const string& file: files) convertShader(file, "", outDir, fmt, outFiles);
    return outFiles;
}

vector<string> ShaderTools::compileShaders(const vector<string>& files, const string& defines, string outDir, string fmt) {
    vector<string> outFiles;
    for (const string& file: files) {
        if (fmt == "glsl") compileShaderGLSL(file, defines, outDir, outFiles);
        else if (fmt == "msl") compileShaderMTL(file, defines, outDir, outFiles);
        else if (fmt == "hlsl") compileShaderHLSL(file, defines, outDir, outFiles);
    }
    return outFiles;
}

void ShaderTools::applyPatches(const vector<string> &patchFiles, string outDir) {
    for (const string& patchFile: patchFiles) {
        string filename = FileUtil::splitExt(fs::path(patchFile))[0];
        printf("filename: %s\n", filename.c_str());
        string outFile = fs::path(outDir+"/"+filename);
        if (fs::exists(outFile)) {
            printf("applying patch: {patchFile}");
            string cmdStr = PATCH+" -N -u "+outFile+" -i "+patchFile;
            cmd(cmdStr);
        }
    }
}

vector<string> ShaderTools::generateShaderMaps(const vector<string>& files, string outDir, string fmt) {
    vector<string> outFiles;
    for (const string& file: files) {
        if (fmt == "glsl") generateShaderMapGLSL(file, outDir, outFiles);
        else if (fmt == "msl") generateShaderMapMSL(file, outDir, outFiles);
        else if (fmt == "hlsl") generateShaderMapHLSL(file, outDir, outFiles);
    }
    return outFiles;
}
