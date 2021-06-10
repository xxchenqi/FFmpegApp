//
// Created by vampirecq on 2021/5/30.
//

#ifndef FFMPEGAPP_VIDEOCHANNEL_H
#define FFMPEGAPP_VIDEOCHANNEL_H

#include "BaseChannel.h"
#include "AudioChannel.h"

extern "C" {
#include <libavutil//imgutils.h>
#include <libswscale/swscale.h>
};

typedef void (*RenderCallback)(uint8_t *, int, int, int);

class VideoChannel : public BaseChannel {

private:
    pthread_t pid_video_decode;
    pthread_t pid_video_play;
    RenderCallback renderCallback;
    int fps;
    AudioChannel *audio_channel = 0;

public:
    VideoChannel(int streamIndex, AVCodecContext *codecContext, AVRational time_base, int fps);

    ~VideoChannel();

    void stop();

    void start();

    void video_decode();

    void video_play();

    void setRenderCallback(RenderCallback renderCallback);

    void setAudioChannel(AudioChannel *audio_channel);
};

#endif //FFMPEGAPP_VIDEOCHANNEL_H
