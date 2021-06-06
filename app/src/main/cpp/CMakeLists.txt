cmake_minimum_required(VERSION 3.10.2)
project("ffmpegapp")

include_directories(${CMAKE_SOURCE_DIR}/ffmpeg/include)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${CMAKE_SOURCE_DIR}/../jniLibs/${CMAKE_ANDROID_ARCH_ABI}")

# 批量导入 源文件
file(GLOB src_files *.cpp)

add_library(
        native-lib
        SHARED
        ${src_files})


target_link_libraries(
        native-lib
        # 忽略顺序的方式，导入
        -Wl,--start-group
        avformat avcodec avfilter avutil swresample swscale
        -Wl,--end-group
        log
        z #ffmepg需要用到z库
#        rtmp # rtmp 后面会专门介绍
        android # android 后面会专门介绍，目前先要明白的是 ANativeWindow 用来渲染画面的
        OpenSLES # OpenSLES 后面会专门介绍，目前先要明白的是 OpenSLES 用来播放声音的

)