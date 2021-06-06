#ifndef FFMPEGAPP_LOG4C_H
#define FFMPEGAPP_LOG4C_H

#include <android/log.h>

#define TAG "xxchenqi"

#define LOGD(...) __android_log_print(ANDROID_LOG_DEBBUG,TAG,__VA_ARGS__);
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,TAG,__VA_ARGS__);
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,TAG,__VA_ARGS__);


#endif //FFMPEGAPP_LOG4C_H
