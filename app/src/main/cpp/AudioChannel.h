//
// Created by vampirecq on 2021/5/30.
//

#ifndef FFMPEGAPP_AUDIOCHANNEL_H
#define FFMPEGAPP_AUDIOCHANNEL_H

#include "BaseChannel.h"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

extern "C" {
#include <libswresample//swresample.h>
};

class AudioChannel : public BaseChannel {
private:
    pthread_t pid_audio_decode;
    pthread_t pid_audio_play;

public:

    int out_channels;
    int out_sample_size;
    int out_sample_rate;
    int out_buffer_size;
    uint8_t *out_buffers = 0;
    SwrContext *swr_ctx = 0;

    //引擎
    SLObjectItf engineObject = 0;
    //引擎接口
    SLEngineItf engineInterface = 0;
    //混音器
    SLObjectItf outputMixObject = 0;
    //播放器
    SLObjectItf bqPlayerObject = 0;
    //播放器接口
    SLPlayItf bqPlayerPlay = 0;
    //播放器队列接口
    SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue = 0;

    double audio_time;

public:
    AudioChannel(int streamIndex, AVCodecContext *codecContext, AVRational time_base);

    ~AudioChannel();

    void start();

    void stop();

    void audio_decode();

    void audio_play();

    int getPCM();

};

#endif //FFMPEGAPP_AUDIOCHANNEL_H
