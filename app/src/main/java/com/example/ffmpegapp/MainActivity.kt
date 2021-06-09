package com.example.ffmpegapp

import android.Manifest
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.os.Environment
import android.view.SurfaceView
import android.view.View
import android.widget.SeekBar
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import java.io.File

class MainActivity : AppCompatActivity(), SeekBar.OnSeekBarChangeListener {

    private var player: XPlayer? = null
    private var surfaceView: SurfaceView? = null
    private var seekBar: SeekBar? = null
    private var tv_time: TextView? = null
    private var duration = 0 // 获取native层的总时长
    private var isTouch = false

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        surfaceView = findViewById(R.id.surfaceView)
        tv_time = findViewById(R.id.tv_time)
        seekBar = findViewById(R.id.seekBar)
        seekBar?.setOnSeekBarChangeListener(this)
        player = XPlayer()
        request()
    }

    private fun request() {
        val permission = Manifest.permission.READ_EXTERNAL_STORAGE //这个是需要申请的权限信息
        val permissions = arrayOf(Manifest.permission.READ_EXTERNAL_STORAGE)
        val checkSelfPermission = ContextCompat.checkSelfPermission(this, permission)
        if (checkSelfPermission == PackageManager.PERMISSION_GRANTED) {
            init()
        } else {
            if (ActivityCompat.shouldShowRequestPermissionRationale(this, permission)) {
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                    requestPermissions(permissions, 0)
                }
            } else {
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                    requestPermissions(permissions, 0)
                }
            }
        }
    }

    fun init() {
        player?.setSurfaceView(surfaceView)
        player?.source = File(
            Environment.getExternalStorageDirectory().toString()
                    + File.separator + "demo.mp4"
        ).toString()

        player?.onPreparedListener = object : XPlayer.OnPreparedListener {
            override fun onPrepared() {
                duration = player!!.getDuration()
                runOnUiThread {
                    if (duration != 0) {
                        tv_time?.text =
                            "00:00/${DateUtil.getMinutes(duration)}:${DateUtil.getSeconds(duration)}"
                        tv_time!!.visibility = View.VISIBLE
                        seekBar!!.visibility = View.VISIBLE
                    }
                    Toast.makeText(this@MainActivity, "prepare success", Toast.LENGTH_SHORT).show()
                }
                player?.start()
            }
        }

        player?.onErrorListener = object : XPlayer.OnErrorListener {
            override fun onError(errorCode: String?) {
                runOnUiThread {
                    Toast.makeText(this@MainActivity, errorCode, Toast.LENGTH_SHORT).show()
                }
            }
        }
        player?.onProgressListener = object : XPlayer.OnProgressListener {
            override fun onProgress(progress: Int) {
                if (!isTouch) {
                    runOnUiThread {
                        tv_time?.text =
                            "${DateUtil.getMinutes(progress)}:${DateUtil.getSeconds(progress)}/${DateUtil.getMinutes(
                                duration
                            )}:${DateUtil.getSeconds(duration)}"
                        seekBar?.progress = progress * 100 / duration
                    }
                }

            }
        }
    }

    override fun onResume() {
        super.onResume()
        player?.prepare()
    }

    override fun onStop() {
        super.onStop()
        player?.stop()
    }

    override fun onDestroy() {
        super.onDestroy()
        player?.release()
    }

    override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
        if (fromUser) {
            tv_time?.text =
                "${DateUtil.getMinutes(progress * duration / 100)}:${DateUtil.getSeconds(progress * duration / 100)}/${DateUtil
                    .getMinutes(duration)}:${DateUtil.getSeconds(duration)}"
        }
    }

    override fun onStartTrackingTouch(seekBar: SeekBar?) {
        isTouch = true
    }

    override fun onStopTrackingTouch(seekBar: SeekBar?) {
        isTouch = false
        if (seekBar != null) {
            val progress = seekBar.progress
            val playProgress: Int = progress * duration / 100
            player?.seek(playProgress)
        }

    }

}