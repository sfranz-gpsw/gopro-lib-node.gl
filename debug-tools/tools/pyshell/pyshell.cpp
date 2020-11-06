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
#include <Python.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    //Convert arguments to UTF8 string
    wchar_t **wargv = (wchar_t**)malloc(sizeof(wchar_t*) * argc);
    for (int j = 0; j<argc; j++) {
        int len = strlen(argv[j]);
        wargv[j] = (wchar_t*)malloc(sizeof(wchar_t) * (len+1));
        mbstowcs(wargv[j], argv[j], len);
        wargv[j][len] = 0;
    }
    Py_Initialize();
    Py_Main(argc, wargv);
    Py_Finalize();
    for (uint32_t j = 0; j<argc; j++) free(wargv[j]);
    free(wargv);
}
