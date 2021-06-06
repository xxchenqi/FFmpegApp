//
// Created by vampirecq on 2021/5/30.
//

#include "AudioChannel.h"

AudioChannel::AudioChannel(int streamIndex, AVCodecContext *codecContext, AVRational time_base)
        : BaseChannel(streamIndex, codecContext, time_base) {
    //初始化缓冲区
    //音频三要素：采样率，位声/采样格式大小，声道数
    //AAC默认为32位,但是播放的目标是16位,所以需要重采样

    //获取声道数量2
    out_channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
    //每个样本是16位 = 2字节
    out_sample_size = av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
    //采样率
    out_sample_rate = 44100;
    //计算buffer大小
    out_buffer_size = out_sample_rate * out_sample_size * out_channels;
    out_buffers = static_cast<uint8_t *>(malloc(out_buffer_size));

    //音频重采样上下文
    swr_ctx = swr_alloc_set_opts(
            0,
            //输出
            AV_CH_LAYOUT_STEREO,//双声道
            AV_SAMPLE_FMT_S16,//采样大小 16位
            out_sample_rate,//采样率
            //输入
            codecContext->channel_layout,//声道布局类型
            codecContext->sample_fmt,//采样大小
            codecContext->sample_rate,//采样率
            0,
            0
    );
    //初始化重采样上下文
    swr_init(swr_ctx);
}

AudioChannel::~AudioChannel() {

}

void *task_audio_decode(void *args) {
    auto *audio_channel = static_cast<AudioChannel *>(args);
    audio_channel->audio_decode();
    return 0;
}

void *task_audio_play(void *args) {
    auto *audio_channel = static_cast<AudioChannel *>(args);
    audio_channel->audio_play();
    return 0;
}

void AudioChannel::start() {
    isPlaying = 1;
    packets.setWork(1);
    frames.setWork(1);

    pthread_create(&pid_audio_decode, 0, task_audio_decode, this);
    pthread_create(&pid_audio_play, 0, task_audio_play, this);
}


void AudioChannel::audio_decode() {
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
        ret = avcodec_send_packet(codecContext, pkt);
//        releaseAVPacket(&pkt);
        if (ret) {
            break;
        }
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

void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *args) {
    auto *audio_channel = static_cast<AudioChannel *>(args);
    int pcm_size = audio_channel->getPCM();
    //添加数据到缓冲区
    (*bq)->Enqueue(bq,
                   audio_channel->out_buffers,
                   pcm_size);
}

//获取PCM数据,PCM数据在frame里
int AudioChannel::getPCM() {
    int pcm_data_size = 0;
    AVFrame *frame = 0;
    while (isPlaying) {
        int ret = frames.getQueueAndDel(frame);
        if (!isPlaying) {
            break;
        }
        if (!ret) {
            continue;
        }

        //计算目标样本数
        int dst_nb_samples = av_rescale_rnd(
                swr_get_delay(swr_ctx, frame->sample_rate) + frame->nb_samples,
                out_sample_rate,
                frame->sample_rate,
                AV_ROUND_UP
        );
        //重采样
        int samples_per_channel = swr_convert(
                swr_ctx,
                //输出
                &out_buffers,//重采样后的buffer
                dst_nb_samples,//输出的样本数
                //输入
                (const uint8_t **) frame->data,//PCM未采样的数据
                frame->nb_samples//输入的样本数
        );

        pcm_data_size = samples_per_channel * out_sample_size * out_channels;
        //音频播放的时间戳
        audio_time = frame->best_effort_timestamp * av_q2d(time_base);
        break;
    }

    av_frame_unref(frame);
    releaseAVFrame(&frame);

    return pcm_data_size;
}

void AudioChannel::audio_play() {
    //接收返回值
    SLresult result;

    //创建引擎
    result = slCreateEngine(&engineObject, 0, 0, 0, 0, 0);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("创建引擎 slCreateEngine error")
        return;
    }

    //初始化引擎
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("创建引擎 Realize error")
        return;
    }

    //获取引擎接口
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineInterface);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("创建引擎 GetInterface error")
        return;
    }

    if (engineInterface) {
        LOGE("创建引擎接口 success")
    } else {
        LOGE("创建引擎接口 fail")
        return;
    }

    //创建混音器
    result = (*engineInterface)->CreateOutputMix(engineInterface, &outputMixObject, 0, 0, 0);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("初始化混音器 CreateOutputMix error")
        return;
    }

    //初始化混音器
    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("初始化混音器 Realize error")
        return;
    }

    //创建缓存队列
    SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
                                                       10};
    //pcm数据格式
    SLDataFormat_PCM format_pcm = {
            SL_DATAFORMAT_PCM,//PCM数据格式
            2,//声道数
            SL_SAMPLINGRATE_44_1,//采样率
            SL_PCMSAMPLEFORMAT_FIXED_16,//每个采样样本存放大小
            SL_PCMSAMPLEFORMAT_FIXED_16,//每个样本位数
            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,//前左右声道
            SL_BYTEORDER_LITTLEENDIAN//小端
    };

    //音频配置信息-数据源
    SLDataSource audioSrc = {&loc_bufq, &format_pcm};

    //设置混音器
    SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    SLDataSink audioSnk = {&loc_outmix, NULL};

    const SLInterfaceID ids[1] = {SL_IID_BUFFERQUEUE};
    const SLboolean req[1] = {SL_BOOLEAN_TRUE};

    //创建播放器
    result = (*engineInterface)->CreateAudioPlayer(
            engineInterface,
            &bqPlayerObject,
            &audioSrc,
            &audioSnk,
            1,
            ids,
            req
    );
    if (result != SL_RESULT_SUCCESS) {
        LOGE("创建播放器 CreateAudioPlayer error")
        return;
    }

    //初始化播放器
    result = (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("初始化播放器 Realize error")
        return;
    }

    //获取播放器接口
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerPlay);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("获取播放器接口 GetInterface error")
        return;
    }

    LOGE("初始化播放器 Success")

    //获取播放器队列接口
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE,
                                             &bqPlayerBufferQueue);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("获取播放器队列接口 GetInterface error")
        return;
    }

    //设置回调
    (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, bqPlayerCallback, this);

    LOGE("设置播放回调 Success")

    //设置播放器状态为播放
    (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);
    LOGE("设置播放状态为播放 Success")

    //手动激活回调
    bqPlayerCallback(bqPlayerBufferQueue, this);

}


void AudioChannel::stop() {

}
