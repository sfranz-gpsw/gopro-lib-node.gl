/*
 * Copyright 2017 GoPro Inc.
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

#include <jni.h>

#include "android_choreographer.h"
#include "android_utils.h"
#include "jni_utils.h"
#include "log.h"
#include "memory.h"

struct android_choreographer {
    jobject choreographer;
    jmethodID init_id;
    jmethodID start_id;
    jmethodID stop_id;
    jmethodID release_id;
    android_choreographer_cb user_cb;
    void *user_data;
};

#define CHECK_EXCEPTION() do {                  \
    if (ngli_jni_exception_check(env, 1) < 0)   \
        goto fail;                              \
} while (0)                                     \

static void native_do_frame(JNIEnv *env, jobject object, jlong native_ptr, jlong frame_time_ns)
{
    struct android_choreographer *s = (struct android_choreographer *)native_ptr;
    if (s->user_cb)
        s->user_cb(s->user_data, frame_time_ns);
}

struct android_choreographer *ngli_android_choreographer_new(android_choreographer_cb user_cb, void *user_data)
{
    struct android_choreographer *ret = ngli_calloc(1, sizeof(*ret));
    if (!ret) {
        return NULL;
    }
    ret->user_cb = user_cb;
    ret->user_data = user_data;

    static const JNINativeMethod methods[] = {
        {"nativeDoFrame", "(JJ)V", (void *)&native_do_frame},
    };

    JNIEnv *env = ngli_jni_get_env();
    if (!env) {
        ngli_free(ret);
        return NULL;
    }

    jclass choreographer_class = NULL;
    jobject choreographer = NULL;

    choreographer_class = ngli_android_find_application_class(env, "org/nodegl/AChoreographer");
    if (!choreographer_class) {
        goto fail;
    }

    (*env)->RegisterNatives(env, choreographer_class, methods, NGLI_ARRAY_NB(methods));
    CHECK_EXCEPTION();

    ret->init_id = (*env)->GetMethodID(env, choreographer_class, "<init>", "(J)V");
    CHECK_EXCEPTION();

    ret->start_id = (*env)->GetMethodID(env, choreographer_class, "start", "()V");
    CHECK_EXCEPTION();

    ret->stop_id = (*env)->GetMethodID(env, choreographer_class, "stop", "()V");
    CHECK_EXCEPTION();

    ret->release_id = (*env)->GetMethodID(env, choreographer_class, "release", "()V");
    CHECK_EXCEPTION();

    choreographer = (*env)->NewObject(env, choreographer_class, ret->init_id, ret);
    CHECK_EXCEPTION();

    ret->choreographer = (*env)->NewGlobalRef(env, choreographer);
    if (!ret->choreographer)
        goto fail;

    (*env)->DeleteLocalRef(env, choreographer_class);
    (*env)->DeleteLocalRef(env, choreographer);

    return ret;

fail:
    (*env)->DeleteLocalRef(env, choreographer_class);
    (*env)->DeleteLocalRef(env, choreographer);
    ngli_free(ret);

    return NULL;
}

int ngli_android_choreographer_start(struct android_choreographer *choreographer)
{
    JNIEnv *env = ngli_jni_get_env();

    (*env)->CallVoidMethod(env, choreographer->choreographer, choreographer->start_id);
    if (ngli_jni_exception_check(env, 1) < 0)
        return -1;

    return 0;
}

int ngli_android_choreographer_stop(struct android_choreographer *choreographer)
{
    JNIEnv *env = ngli_jni_get_env();

    (*env)->CallVoidMethod(env, choreographer->choreographer, choreographer->stop_id);
    if (ngli_jni_exception_check(env, 1) < 0)
        return -1;

    return 0;
}

void ngli_android_choreographer_free(struct android_choreographer **choreographer)
{
    if (!*choreographer)
        return;

    JNIEnv *env = ngli_jni_get_env();
    if (!env) {
        ngli_free(*choreographer);
        *choreographer = NULL;
        return;
    }
    if ((*choreographer)->choreographer) {
        (*env)->CallVoidMethod(env, (*choreographer)->choreographer, (*choreographer)->release_id);
        ngli_jni_exception_check(env, 1);

        (*env)->DeleteGlobalRef(env, (*choreographer)->choreographer);
    }

    ngli_free(*choreographer);
    *choreographer = NULL;
}
