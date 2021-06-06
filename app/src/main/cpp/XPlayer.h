#ifndef FFMPEGAPP_XPLAYER_H
#define FFMPEGAPP_XPLAYER_H

#include <cstring>
#include <pthread.h>
#include "AudioChannel.h"
#include "VideoChannel.h"
#include "JNICallbackHelper.h"
extern "C"{
#include <libavformat/avformat.h>
}

class XPlayer {
private:
    char *source = 0;
    pthread_t pid_prepare;
    pthread_t pid_start;
    AVFormatContext *formatContext = 0;
    AudioChannel *audioChannel = 0;
    VideoChannel *videoChannel = 0;
    JNICallbackHelper *helper = 0;
    bool isPlaying; // 是否播放
    RenderCallback renderCallback;

public:
    XPlayer(const char *source, JNICallbackHelper *jniCallbackHelper);

    ~XPlayer();

    void prepare();

    void prepare_();

    void start();

    void start_();

    void setRenderCallback(RenderCallback renderCallback);
};


#endif //FFMPEGAPP_XPLAYER_H
