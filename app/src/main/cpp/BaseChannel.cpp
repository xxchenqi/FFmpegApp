//
// Created by vampirecq on 2021/5/30.
//

#include "BaseChannel.h"


BaseChannel::BaseChannel(int stream_index, AVCodecContext *codecContext, AVRational time_base) {
    this->stream_index = stream_index;
    this->codecContext = codecContext;
    this->time_base = time_base;
    packets.setReleaseCallback(releaseAVPacket);
    frames.setReleaseCallback(releaseAVFrame);
}

BaseChannel::~BaseChannel() {
    packets.clear();
    frames.clear();
}

void BaseChannel::releaseAVPacket(AVPacket **p) {
    if (p) {
        av_packet_free(p);
        *p = 0;
    }
}

void BaseChannel::releaseAVFrame(AVFrame **f) {
    if (f) {
        av_frame_free(f);
        *f = 0;
    }
}