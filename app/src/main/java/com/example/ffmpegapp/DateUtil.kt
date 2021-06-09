package com.example.ffmpegapp

object DateUtil {
     fun getMinutes(duration: Int): String? {
        val minutes = duration / 60
        return if (minutes <= 9) {
            "0$minutes"
        } else "" + minutes
    }

     fun getSeconds(duration: Int): String? {
        val seconds = duration % 60
        return if (seconds <= 9) {
            "0$seconds"
        } else "" + seconds
    }

}