package com.example.ffmpegapp

import android.Manifest
import android.content.pm.PackageManager
import android.os.Build
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.os.Environment
import android.view.SurfaceView
import android.widget.Toast
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import java.io.File

class MainActivity : AppCompatActivity() {

    private var player: XPlayer? = null
    private var surfaceView: SurfaceView? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        surfaceView = findViewById(R.id.surfaceView)
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
                runOnUiThread {
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

}