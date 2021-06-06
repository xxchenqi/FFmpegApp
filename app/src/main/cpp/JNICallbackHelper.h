//
// Created by vampirecq on 2021/5/30.
//

#ifndef FFMPEGAPP_JNICALLBACKHELPER_H
#define FFMPEGAPP_JNICALLBACKHELPER_H

#include <jni.h>
#include "util.h"

class JNICallbackHelper {
private:
    JavaVM *vm = 0;
    JNIEnv *env = 0;
    jobject job;
    jmethodID jmd_prepared;
    jmethodID jmd_onError;

public:
    JNICallbackHelper(JavaVM *vm, JNIEnv *env, jobject job);
    ~JNICallbackHelper();
    void onPrepared(int thread_mode);
    void onError(int thread_mode, int error_code);
};

#endif //FFMPEGAPP_JNICALLBACKHELPER_H
