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
#include <cassert>
using namespace std;
namespace fs = std::filesystem;
using namespace ngfx;

time_t FileUtil::getmtime(const string& filename) {
    if (!fs::exists(filename)) { return 0; }
    auto ftime = fs::last_write_time(filename);
    return fs::file_time_type::clock::to_time_t(ftime);
}

bool FileUtil::srcFileChanged(const string& srcFileName, const string& targetFileName) {
    time_t srcTimeStamp = getmtime(srcFileName);
    time_t targetTimeStamp = getmtime(targetFileName);
    if (srcTimeStamp > targetTimeStamp)
        return true;
    return false;
}

string FileUtil::readFile(const string& path) {
    string contents;
    ifstream in(path, ifstream::binary | ios::ate);
    assert(in);
    auto size = in.tellg();
    in.seekg(0);
    contents.resize(size);
    in.read(&contents[0], size);
    in.close();
    return contents;
}

void FileUtil::writeFile(const string& path, const string& contents) {
    ofstream out(path);
    assert(out);
    out.write(contents.data(), contents.size());
    out.close();
}

vector<string> FileUtil::splitExt(const string& filename) {
    auto it = filename.find_last_of('.');
    return { filename.substr(0, it), filename.substr(it) };
}

vector<string> FileUtil::findFiles(const string& path) {
    vector<string> files;
    for (const auto & entry : fs::directory_iterator(path)) {
        files.push_back(entry.path());
    }
    return files;
}

vector<string> FileUtil::findFiles(const string& path, const string& ext) {
    vector<string> files;
    for (const auto & entry : fs::directory_iterator(path)) {
        const fs::path& path = entry.path();
        if (path.extension() != ext) continue;
        files.push_back(path);
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
        for (const string& ext: extensions) {
            vector<string> filteredFiles = findFiles(path, ext);
            files.insert(files.end(),filteredFiles.begin(), filteredFiles.end());
        }
    }
    return files;
}

