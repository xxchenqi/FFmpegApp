package com.example.ffmpegapp

import android.view.Surface
import android.view.SurfaceHolder
import android.view.SurfaceView

class XPlayer : SurfaceHolder.Callback {

    companion object {
        // 打不开视频
        // #define FFMPEG_CAN_NOT_OPEN_URL 1
        private const val FFMPEG_CAN_NOT_OPEN_URL = 1

        // 找不到流媒体
        // #define FFMPEG_CAN_NOT_FIND_STREAMS 2
        private const val FFMPEG_CAN_NOT_FIND_STREAMS = 2

        // 找不到解码器
        // #define FFMPEG_FIND_DECODER_FAIL 3
        private const val FFMPEG_FIND_DECODER_FAIL = 3

        // 无法根据解码器创建上下文
        // #define FFMPEG_ALLOC_CODEC_CONTEXT_FAIL 4
        private const val FFMPEG_ALLOC_CODEC_CONTEXT_FAIL = 4

        //  根据流信息 配置上下文参数失败
        // #define FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL 6
        private const val FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL = 6

        // 打开解码器失败
        // #define FFMPEG_OPEN_DECODER_FAIL 7
        private const val FFMPEG_OPEN_DECODER_FAIL = 7

        // 没有音视频
        // #define FFMPEG_NOMEDIA 8
        private const val FFMPEG_NOMEDIA = 8

        init {
            System.loadLibrary("native-lib")
        }
    }

    var source: String? = null

    var surfaceHolder: SurfaceHolder? = null

    var onPreparedListener: OnPreparedListener? = null

    var onErrorListener: OnErrorListener? = null

    var onProgressListener: OnProgressListener? = null

    fun prepare() {
        prepareNative(source)
    }

    fun start() {
        startNative()
    }

    fun stop() {
        stopNative()
    }

    fun release() {
        releaseNative()
    }

    fun onPrepared() {
        onPreparedListener?.onPrepared()
    }

    fun onError(errorCode: Int) {
        if (null != onErrorListener) {
            var msg: String? = null
            when (errorCode) {
                FFMPEG_CAN_NOT_OPEN_URL -> msg = "打不开视频"
                FFMPEG_CAN_NOT_FIND_STREAMS -> msg = "找不到流媒体"
                FFMPEG_FIND_DECODER_FAIL -> msg = "找不到解码器"
                FFMPEG_ALLOC_CODEC_CONTEXT_FAIL -> msg = "无法根据解码器创建上下文"
                FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL -> msg =
                    "根据流信息 配置上下文参数失败"
                FFMPEG_OPEN_DECODER_FAIL -> msg = "打开解码器失败"
                FFMPEG_NOMEDIA -> msg = "没有音视频"
            }
            onErrorListener?.onError(msg)
        }
    }

    fun setSurfaceView(surfaceView: SurfaceView?) {
        if (surfaceHolder != null) {
            surfaceHolder?.removeCallback(this)
        }
        surfaceHolder = surfaceView?.holder
        surfaceHolder?.addCallback(this)
    }

    fun getDuration(): Int {
        return getDurationNative()
    }

    /**
     * 让jni反射调用进度条
     */
    fun onProgress(progress: Int) {
        onProgressListener?.onProgress(progress)
    }

    fun seek(playProgress: Int) {
        seekNative(playProgress)
    }

    override fun surfaceCreated(surfaceHolder: SurfaceHolder?) {
    }

    override fun surfaceChanged(surfaceHolder: SurfaceHolder?, p1: Int, p2: Int, p3: Int) {
        //界面发生改变
        if (surfaceHolder != null) {
            setSurfaceNative(surfaceHolder.surface)
        }
    }

    override fun surfaceDestroyed(surfaceHolder: SurfaceHolder?) {
    }

    interface OnPreparedListener {
        fun onPrepared()
    }

    interface OnErrorListener {
        fun onError(errorCode: String?)
    }

    interface OnProgressListener {
        fun onProgress(progress: Int)
    }

    private external fun prepareNative(source: String?)
    private external fun startNative()
    private external fun stopNative()
    private external fun releaseNative()
    private external fun setSurfaceNative(surface: Surface)
    private external fun getDurationNative(): Int
    private external fun seekNative(progress: Int)

}

