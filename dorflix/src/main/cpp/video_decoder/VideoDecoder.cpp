#include "VideoDecoder.h"
#include <android/log.h>

#define LOG_TAG "VideoDecoder"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

VideoDecoder::VideoDecoder() {}
VideoDecoder::~VideoDecoder() {}

bool VideoDecoder::init(const char* path) {
    LOGI("Initializing decoder for: %s", path);
    return true;
}

void VideoDecoder::decode() {
    // Stub implementation
}

// Stub for FrameBufferManager if referenced
void FrameBufferManager_stub() {}
