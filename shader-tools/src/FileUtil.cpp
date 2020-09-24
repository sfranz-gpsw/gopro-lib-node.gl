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
#include "FileUtil.h"
#include <fstream>
#include <filesystem>
#include <cstring>
using namespace std;
namespace fs = std::filesystem;
using namespace ngfx;

string FileUtil::readFile(const string& path) {
    string contents;
    ifstream in(path, ios::ate);
    auto size = in.tellg();
    in.seekg(0);
    contents.resize(size);
    in.read(&contents[0], size);
    in.close();
    return contents;
}

vector<string> FileUtil::splitExt(const string& filename) {
    auto it = filename.find_last_of('.');
    return { filename.substr(0, it), filename.substr(it + 1) };
}

vector<string> FileUtil::findFiles(const string& path) {
    vector<string> files;
    for (const auto & entry : fs::directory_iterator(path)) {
        files.push_back(entry.path());
    }
    return files;
}

vector<string> FileUtil::filterFiles(const vector<string>& files, const string& fileFilter) {
    vector<string> filteredFiles;
    for (const string& file: files) {
      if (strstr(file.c_str(), fileFilter.c_str()))
          filteredFiles.push_back(file);
    }
    return filteredFiles;
}

vector<string> FileUtil::findFiles(const vector<string>& paths, const vector<string>& extensions) {
    vector<string> files;
    for (const string& path: paths) {
        vector<string> dirFiles = findFiles(path);
        for (const string& ext: extensions) {
            vector<string> filteredFiles = filterFiles(dirFiles, ext);
            files.insert(files.end(),filteredFiles.begin(), filteredFiles.end());
        }
    }
    return files;
}
