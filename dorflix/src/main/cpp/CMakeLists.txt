# CMake minimum version
cmake_minimum_required(VERSION 3.22.1)

# Project name
project(dorflix_video)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable exceptions and RTTI
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions -frtti")

# Find required packages
find_library(log-lib log)
find_library(android-lib android)
find_library(jnigraphics-lib jnigraphics)
find_library(glesv3-lib GLESv3)
find_library(egl-lib EGL)
find_library(mediacodec-lib mediandk)  # For MediaCodec NDK

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg/include)

# FFmpeg library paths (now in Android's standard jniLibs directory)
set(FFMPEG_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../main/jniLibs/${ANDROID_ABI})

# Video Decoder Library (static to avoid circular dependencies)
add_library(video_decoder STATIC
    video_decoder/VideoDecoder.cpp
    video_decoder/AudioDecoder.cpp
    video_decoder/VideoCacheManager.cpp
    video_decoder/FrameBufferManager.cpp
    video_decoder/ScrollingPredictor.cpp
    video_decoder/PerformanceMonitor.cpp
    video_decoder/VideoQualityManager.cpp
    video_decoder/AudioQualityManager.cpp
    video_decoder/MemoryManager.cpp
    video_decoder/FramePool.cpp
    video_decoder/VideoPlaybackEngine.cpp
    video_decoder/OpenGLVideoRenderer.cpp
    video_decoder/GPUCapabilitiesDetector.cpp
    # 32-bit address space detection and memory containment
    video_decoder/AddressSpaceDetector.cpp
    video_decoder/MemoryConstrainedBufferManager.cpp
    # Hierarchical decoder system
    video_decoder/VideoDecoderFactory.cpp
    video_decoder/MediaCodecHardwareDecoder.cpp
    video_decoder/FFmpegDecoder.cpp
    video_decoder/ANativeWindowRenderer.cpp
    video_decoder/VideoRendererFactory.cpp
    # Temporarily disabled problematic audio components
    # video_decoder/AudioPlaybackEngine.cpp
    # video_decoder/AudioBufferPool.cpp
    video_decoder/VideoPlayer.h
    video_decoder/AudioPlayer.h
    video_decoder/OpenGLVideoRenderer.h
    video_decoder/GPUCapabilitiesDetector.h
    # Hierarchical system headers
    video_decoder/VideoDecoderFactory.h
    video_decoder/VideoDecoderBase.h
    video_decoder/MediaCodecHardwareDecoder.h
    video_decoder/VideoRenderer.h
    video_decoder/ANativeWindowRenderer.h
    video_decoder/VideoRendererFactory.h
)

# A/V Sync Library
add_library(av_sync STATIC
    av_sync/MediaClock.cpp
    av_sync/AVBuffer.cpp
    av_sync/AVSyncController.cpp
    av_sync/SyncLogger.cpp
    av_sync/PerformanceProfiler.cpp
    av_sync/ExceptionSafety.h
)

# Video Preloader Library
add_library(video_preloader SHARED
    video_preloader/Preloader.cpp
)

# Memory Management Library
add_library(memory_manager SHARED
    memory/MemoryPool.cpp
)

# JNI Interface Library
# Note: This is the main library name referenced in System.loadLibrary("dorflix-native")
add_library(dorflix-native SHARED
    jni/jni_interface.cpp
)

# Export ALL symbols from dorflix-native so video_decoder can link to them
target_compile_options(dorflix-native PRIVATE -fvisibility=default)
target_link_options(dorflix-native PRIVATE -Wl,--export-dynamic)

# Link libraries
target_link_libraries(video_decoder
    ${log-lib}
    ${android-lib}
    ${jnigraphics-lib}
    ${glesv3-lib}
    ${egl-lib}
    ${mediacodec-lib}  # MediaCodec NDK
    OpenSLES
    ${FFMPEG_LIB_DIR}/libavformat.so
    ${FFMPEG_LIB_DIR}/libavcodec.so
    ${FFMPEG_LIB_DIR}/libavutil.so
    ${FFMPEG_LIB_DIR}/libswscale.so
    ${FFMPEG_LIB_DIR}/libswresample.so
)

target_link_libraries(video_preloader
    ${log-lib}
)

target_link_libraries(memory_manager
    ${log-lib}
)

target_link_libraries(dorflix-native
    video_decoder
    av_sync
    video_preloader
    memory_manager
    ${log-lib}
    ${android-lib}
)