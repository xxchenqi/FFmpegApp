
#include "XPlayer.h"


XPlayer::XPlayer(const char *source, JNICallbackHelper *jniCallbackHelper) {
    this->source = new char[strlen(source) + 1];
    this->helper = jniCallbackHelper;
    strcpy(this->source, source);
    pthread_mutex_init(&seek_mutex, nullptr);
}

XPlayer::~XPlayer() {
    if (source) {
        delete source;
        source = nullptr;
    }
    if (helper) {
        delete helper;
        helper = nullptr;
    }

    pthread_mutex_destroy(&seek_mutex);
}

void *task_prepare(void *args) {
    auto *player = static_cast<XPlayer *>(args);
    player->prepare_();
    return 0;
}

void XPlayer::prepare() {
    pthread_create(&pid_prepare, 0, task_prepare, this);
}

void XPlayer::prepare_() {
    formatContext = avformat_alloc_context();
    AVDictionary *dictionary = 0;
    av_dict_set(&dictionary, "timeout", "5000000", 0);

    //打开媒体文件地址
    int r = avformat_open_input(&formatContext, source, 0, &dictionary);

    av_dict_free(&dictionary);

    if (r) {
        if (helper) {
            helper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_OPEN_URL);
            // char * errorInfo = av_err2str(r); // 根据你的返回值 得到错误详情
        }
        avformat_close_input(&formatContext);
        return;
    }

    //查找媒体中的音视频流
    r = avformat_find_stream_info(formatContext, 0);
    if (r < 0) {
        if (helper) {
            helper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_FIND_STREAMS);
        }
        avformat_close_input(&formatContext);
        return;
    }

    //转成有理数
    this->duration = formatContext->duration / AV_TIME_BASE;
    AVCodecContext *codecContext = nullptr;
    //遍历流的个数
    for (int i = 0; i < formatContext->nb_streams; ++i) {
        //获取媒体流（音频，视频）
        AVStream *stream = formatContext->streams[i];
        //获取编解码的参数（宽高等）
        AVCodecParameters *parameters = stream->codecpar;
        //获取编解码器
        AVCodec *codec = avcodec_find_decoder(parameters->codec_id);
        if (!codec) {
            if (helper) {
                helper->onError(THREAD_CHILD, FFMPEG_FIND_DECODER_FAIL);
            }
            avformat_close_input(&formatContext);
            return;
        }

        //创建编解码器上下文
        codecContext = avcodec_alloc_context3(codec);
        if (!codecContext) {
            if (helper) {
                helper->onError(THREAD_CHILD, FFMPEG_ALLOC_CODEC_CONTEXT_FAIL);
            }
            //释放了codecContext,codec也会一起释放
            avcodec_free_context(&codecContext);
            avformat_close_input(&formatContext);
            return;
        }
        //设置解码器上下文参数
        r = avcodec_parameters_to_context(codecContext, parameters);
        if (r < 0) {
            if (helper) {
                helper->onError(THREAD_CHILD, FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL);
            }
            avcodec_free_context(&codecContext);
            avformat_close_input(&formatContext);
            return;
        }
        //打开解码器
        r = avcodec_open2(codecContext, codec, 0);
        if (r) {
            if (helper) {
                helper->onError(THREAD_CHILD, FFMPEG_OPEN_DECODER_FAIL);
            }
            avcodec_free_context(&codecContext);
            avformat_close_input(&formatContext);
            return;
        }

        //时间基
        AVRational time_base = stream->time_base;

        if (parameters->codec_type == AVMediaType::AVMEDIA_TYPE_AUDIO) {
            audioChannel = new AudioChannel(i, codecContext, time_base);
            if (this->duration != 0) {
                audioChannel->setJNICallbackHelper(helper);
            }
        } else if (parameters->codec_type == AVMediaType::AVMEDIA_TYPE_VIDEO) {
            // 视频类型,有一帧封面
            if (stream->disposition & AV_DISPOSITION_ATTACHED_PIC) {
                continue;
            }
            AVRational fps_rational = stream->avg_frame_rate;
            int fps = av_q2d(fps_rational);

            videoChannel = new VideoChannel(i, codecContext, time_base, fps);
            videoChannel->setRenderCallback(renderCallback);
        }
    }
    if (!audioChannel && !videoChannel) {
        if (helper) {
            helper->onError(THREAD_CHILD, FFMPEG_NOMEDIA);
        }
        if (codecContext) {
            avcodec_free_context(&codecContext);
        }
        avformat_close_input(&formatContext);
        return;
    }

    if (helper) {
        helper->onPrepared(THREAD_CHILD);
    }
}


void *task_start(void *args) {
    auto *player = static_cast<XPlayer *>(args);
    player->start_();
    return 0;
};

void XPlayer::start() {
    isPlaying = 1;
    if (videoChannel) {
        videoChannel->setAudioChannel(audioChannel);
        videoChannel->start();
    }
    if (audioChannel) {
        audioChannel->start();
    }
    pthread_create(&pid_start, 0, task_start, this);
}

void XPlayer::start_() {
    while (isPlaying) {
        //如果队列大于100就休眠,防止内存泄漏
        if (videoChannel && videoChannel->packets.size() > 100) {
            av_usleep(10 * 1000);
            continue;
        }
        if (audioChannel && audioChannel->packets.size() > 100) {
            av_usleep(10 * 1000);
            continue;
        }

        AVPacket *packet = av_packet_alloc();
        int r = av_read_frame(formatContext, packet);
        if (!r) {
            if (videoChannel && videoChannel->stream_index == packet->stream_index) {
                videoChannel->packets.insertQueue(packet);
            } else if (audioChannel && audioChannel->stream_index == packet->stream_index) {
                audioChannel->packets.insertQueue(packet);
            }
        } else if (r == AVERROR_EOF) {
            //读取完了,退出
            if (videoChannel->packets.empty() && audioChannel->packets.empty()) {
                break;
            }
        } else {
            break;
        }
    }
    isPlaying = 0;
    videoChannel->stop();
    audioChannel->stop();
}

void XPlayer::setRenderCallback(RenderCallback renderCallback) {
    this->renderCallback = renderCallback;
}

int XPlayer::getDuration() {
    return duration;
}

void XPlayer::seek(int progress) {
    // 健壮性判断
    if (progress < 0 || progress > duration) {
        return;
    }
    if (!audioChannel && !videoChannel) {
        return;
    }
    if (!formatContext) {
        return;
    }

    pthread_mutex_lock(&seek_mutex);


    int r = av_seek_frame(formatContext, -1, progress * AV_TIME_BASE, AVSEEK_FLAG_FRAME);
    if (r < 0) {
        return;
    }

    if (audioChannel) {
        audioChannel->packets.setWork(0);
        audioChannel->frames.setWork(0);
        audioChannel->packets.clear();
        audioChannel->frames.clear();
        audioChannel->packets.setWork(1);
        audioChannel->frames.setWork(1);
    }
    if (videoChannel) {
        videoChannel->packets.setWork(0);
        videoChannel->frames.setWork(0);
        videoChannel->packets.clear();
        videoChannel->frames.clear();
        videoChannel->packets.setWork(1);
        videoChannel->frames.setWork(1);
    }

    pthread_mutex_unlock(&seek_mutex);

}

void *task_stop(void *args) {
    auto *player = static_cast<XPlayer *>(args);
    player->stop_(player);
    return nullptr; // 必须返回，坑，错误很难找
}

void XPlayer::stop() {
    helper = nullptr;
    if (audioChannel) {
        audioChannel->jniCallbackHelper = nullptr;
    }
    if (videoChannel) {
        videoChannel->jniCallbackHelper = nullptr;
    }

    //我们要等这两个线程,稳定停下来后在做释放,所以放到子线程去等待
    pthread_create(&pid_stop, nullptr, task_stop, this);
}

void XPlayer::stop_(XPlayer *pPlayer) {
    isPlaying = false;
    pthread_join(pid_prepare, nullptr);
    pthread_join(pid_start, nullptr);
    if (formatContext) {
        avformat_close_input(&formatContext);
        avformat_free_context(formatContext);
        formatContext = nullptr;
    }
    DELETE(audioChannel)
    DELETE(videoChannel)
    DELETE(pPlayer)
}

