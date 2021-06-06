
#include "XPlayer.h"


XPlayer::XPlayer(const char *source, JNICallbackHelper *jniCallbackHelper) {
    this->source = new char[strlen(source) + 1];
    this->helper = jniCallbackHelper;
    strcpy(this->source, source);
}

XPlayer::~XPlayer() {
    if (source) {
        delete source;
    }
    if (helper) {
        delete helper;
    }
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
        return;
    }

    //查找媒体中的音视频流
    r = avformat_find_stream_info(formatContext, 0);
    if (r < 0) {
        if (helper) {
            helper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_FIND_STREAMS);
        }
        return;
    }

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
        }

        //创建编解码器上下文
        AVCodecContext *codecContext = avcodec_alloc_context3(codec);
        if (!codecContext) {
            if (helper) {
                helper->onError(THREAD_CHILD, FFMPEG_ALLOC_CODEC_CONTEXT_FAIL);
            }
            return;
        }
        //设置解码器上下文参数
        r = avcodec_parameters_to_context(codecContext, parameters);
        if (r < 0) {
            if (helper) {
                helper->onError(THREAD_CHILD, FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL);
            }
            return;
        }
        //打开解码器
        r = avcodec_open2(codecContext, codec, 0);
        if (r) {
            if (helper) {
                helper->onError(THREAD_CHILD, FFMPEG_OPEN_DECODER_FAIL);
            }
            return;
        }

        //时间基
        AVRational time_base = stream->time_base;

        if (parameters->codec_type == AVMediaType::AVMEDIA_TYPE_AUDIO) {
            audioChannel = new AudioChannel(i, codecContext, time_base);
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

