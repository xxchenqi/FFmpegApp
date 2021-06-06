//
// Created by vampirecq on 2021/5/30.
//

#ifndef FFMPEGAPP_BASECHANNEL_H
#define FFMPEGAPP_BASECHANNEL_H
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/time.h>
}

#include "safe_queue.h"
#include "log4c.h"

class BaseChannel {
public:
    int stream_index;
    SafeQueue<AVPacket *> packets;
    SafeQueue<AVFrame *> frames;
    bool isPlaying;
    AVCodecContext *codecContext = 0;
    AVRational time_base;

    BaseChannel(int stream_index, AVCodecContext *codecContext, AVRational time_base);

    ~BaseChannel();

    static void releaseAVPacket(AVPacket **p);

    static void releaseAVFrame(AVFrame **f);
};

#endif //FFMPEGAPP_BASECHANNEL_H
