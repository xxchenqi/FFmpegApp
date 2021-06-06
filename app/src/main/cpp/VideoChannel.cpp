//
// Created by vampirecq on 2021/5/30.
//

#include "VideoChannel.h"

void dropAVFrame(queue<AVFrame *> &q) {
    if (!q.empty()) {
        AVFrame *frame = q.front();
        BaseChannel::releaseAVFrame(&frame);
        q.pop();
    }
}

void dropAVPacket(queue<AVPacket *> &q) {
    while (!q.empty()) {
        AVPacket *pkt = q.front();
        //非关键帧可以丢弃
        if (pkt->flags != AV_PKT_FLAG_KEY) {
            BaseChannel::releaseAVPacket(&pkt);
            q.pop();
        } else {
            break;
        }
    }
}


VideoChannel::VideoChannel(int streamIndex, AVCodecContext *codecContext, AVRational time_base,
                           int fps)
        : BaseChannel(streamIndex, codecContext, time_base),
          fps(fps) {
    frames.setSyncCallback(dropAVFrame);
    packets.setSyncCallback(dropAVPacket);
}


void VideoChannel::stop() {

}


void *task_video_decode(void *args) {
    auto *videoChannel = static_cast<VideoChannel *>(args);
    videoChannel->video_decode();
    return 0;
}


void *task_video_play(void *args) {
    auto *videoChannel = static_cast<VideoChannel *>(args);
    videoChannel->video_play();
    return 0;
}

void VideoChannel::start() {
    isPlaying = 1;
    packets.setWork(1);
    frames.setWork(1);
    pthread_create(&pid_video_decode, 0, task_video_decode, this);
    pthread_create(&pid_video_play, 0, task_video_play, this);
}


void VideoChannel::video_decode() {
    AVPacket *pkt = 0;
    while (isPlaying) {
        //如果队列大于100就休眠,防止内存泄漏
        if (isPlaying && frames.size() > 100) {
            av_usleep(10 * 1000);
            continue;
        }

        int ret = packets.getQueueAndDel(pkt);
        if (!isPlaying) {
            break;
        }
        if (!ret) {
            continue;
        }
        //发送压缩包给缓冲区
        ret = avcodec_send_packet(codecContext, pkt);
        if (ret) {
            break;
        }

        //从缓冲区获取原始包
        AVFrame *frame = av_frame_alloc();
        ret = avcodec_receive_frame(codecContext, frame);
        if (ret == AVERROR(EAGAIN)) {
            continue;
        } else if (ret != 0) {
            //解码出错就释放
            if (frame) {
                releaseAVFrame(&frame);
            }
            break;
        }
        frames.insertQueue(frame);

        //减引用，等于0时,释放成员指向的堆区
        av_packet_unref(pkt);
        //释放AVPacket 本身的堆区空间
        releaseAVPacket(&pkt);
    }
    av_packet_unref(pkt);
    releaseAVPacket(&pkt);
}


void VideoChannel::video_play() {
    AVFrame *frame = 0;
    uint8_t *dst_data[4];
    int dst_linesize[4];
//    //给dst_data申请内存
    av_image_alloc(dst_data, dst_linesize, codecContext->width, codecContext->height,
                   AV_PIX_FMT_RGBA, 1);

    //SWS_FAST_BILINEAR 快速算法，会模糊
    //SWS_BILINEAR 适中算法
    SwsContext *sws_ctx = sws_getContext(codecContext->width, codecContext->height,
                                         codecContext->pix_fmt,
                                         codecContext->width, codecContext->height,
                                         AV_PIX_FMT_RGBA,
                                         SWS_BILINEAR, NULL, NULL, NULL);

    while (isPlaying) {
        int ret = frames.getQueueAndDel(frame);
        if (!isPlaying) {
            break;
        }
        if (!ret) {
            continue;
        }
        //格式转换 : yuv -> rgba
        sws_scale(sws_ctx, frame->data, frame->linesize, 0, codecContext->height,
                  dst_data, dst_linesize);

        //获取额外延迟时间
        double extra_delay = frame->repeat_pict / (2 * fps);
        //计算每一帧的延迟时间
        double fps_delay = 1.0 / fps;
        //当前帧的延时时间
        double real_delay = fps_delay + extra_delay;

        double video_time = frame->best_effort_timestamp * av_q2d(time_base);
        double audio_time = audio_channel->audio_time;

        double time_diff = video_time - audio_time;
        if (time_diff > 0) {
            if (time_diff > 1) {
                //音视频差距很大
                av_usleep((real_delay * 2) * 1000000);
            } else {
                //音视频差距不大
                av_usleep((real_delay + time_diff) * 1000000);
            }
        } else if (time_diff < 0) {
            if (fabs(time_diff) <= 0.05) {
                frames.sync();
                continue;
            }
        }

        renderCallback(dst_data[0], codecContext->width, codecContext->height, dst_linesize[0]);

        av_frame_unref(frame);
        releaseAVFrame(&frame);
    }

    av_frame_unref(frame);
    releaseAVFrame(&frame);

    isPlaying = 0;
    av_free(&dst_data[0]);
    sws_freeContext(sws_ctx);
}

void VideoChannel::setRenderCallback(RenderCallback renderCallback) {
    this->renderCallback = renderCallback;
}

void VideoChannel::setAudioChannel(AudioChannel *audio_channel) {
    this->audio_channel = audio_channel;
}
