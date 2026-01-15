#include "FramePool.h"
#include <algorithm>
#include <android/log.h>
extern "C" {
#include <libavutil/imgutils.h>
}

// Logging macros
#define LOG_TAG "FramePool"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

FramePool::FramePool()
    : maxFrames(50)
    , frameSizeBytes(0)
{
    LOGI("FramePool initialized");
}

FramePool::~FramePool() {
    cleanup();
    LOGI("FramePool destroyed");
}

void FramePool::initialize(size_t maxFrames) {
    std::lock_guard<std::mutex> lock(poolMutex);
    this->maxFrames = maxFrames;
    LOGI("FramePool initialized with max frames: %zu", maxFrames);
}

AVFrame* FramePool::acquireFrame(int width, int height, AVPixelFormat format) {
    std::lock_guard<std::mutex> lock(poolMutex);

    // Try to find existing frame with matching dimensions
    for (auto it = availableFrames.begin(); it != availableFrames.end(); ++it) {
        AVFrame* frame = *it;
        if (frame->width == width && frame->height == height && frame->format == format) {
            availableFrames.erase(it);
            LOGD("Reused frame from pool: %dx%d", width, height);
            return frame;
        }
    }

    // Allocate new frame if under limit
    if (allFrames.size() < maxFrames) {
        AVFrame* newFrame = av_frame_alloc();
        if (newFrame) {
            av_frame_get_buffer(newFrame, 0);
            newFrame->width = width;
            newFrame->height = height;
            newFrame->format = format;

            allFrames.push_back(newFrame);
            frameSizeBytes += calculateFrameSize(width, height, format);

            LOGD("Allocated new frame: %dx%d, total frames: %zu", width, height, allFrames.size());
            return newFrame;
        }
    }

    // Pool exhausted - allocate temporary frame (will be freed immediately)
    AVFrame* tempFrame = av_frame_alloc();
    if (tempFrame) {
        av_frame_get_buffer(tempFrame, 0);
        tempFrame->width = width;
        tempFrame->height = height;
        tempFrame->format = format;
        LOGW("Pool exhausted, allocated temporary frame: %dx%d", width, height);
    }

    return tempFrame;
}

void FramePool::releaseFrame(AVFrame* frame) {
    if (!frame) return;

    std::lock_guard<std::mutex> lock(poolMutex);

    // Check if this frame belongs to our pool
    auto it = std::find(allFrames.begin(), allFrames.end(), frame);
    if (it != allFrames.end()) {
        // Reset frame data but keep buffer allocated
        av_frame_unref(frame);
        availableFrames.push_back(frame);
        LOGD("Released frame back to pool, available: %zu", availableFrames.size());
    } else {
        // Temporary frame - free it
        av_frame_free(&frame);
        LOGD("Freed temporary frame");
    }
}

FramePool::PoolStats FramePool::getPoolStats() const {
    std::lock_guard<std::mutex> lock(poolMutex);
    return PoolStats {
        allFrames.size(),
        availableFrames.size(),
        frameSizeBytes.load()
    };
}

size_t FramePool::getMemoryUsage() const {
    return frameSizeBytes.load();
}

void FramePool::cleanup() {
    std::lock_guard<std::mutex> lock(poolMutex);

    for (AVFrame* frame : allFrames) {
        av_frame_free(&frame);
    }

    availableFrames.clear();
    allFrames.clear();
    frameSizeBytes = 0;

    LOGI("FramePool cleaned up");
}

size_t FramePool::calculateFrameSize(int width, int height, AVPixelFormat format) const {
    // Calculate frame size for memory tracking
    return av_image_get_buffer_size(format, width, height, 1);
}