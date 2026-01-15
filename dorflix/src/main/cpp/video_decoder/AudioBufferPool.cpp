#include "AudioBufferPool.h"
#include <algorithm>
#include <cstring>

// Logging macros
#define LOG_TAG "AudioBufferPool"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

AudioBufferPool::AudioBufferPool()
    : maxBuffers(20)
    , bufferMemoryBytes(0)
    , defaultBufferSize(8192)
{
    LOGI("AudioBufferPool initialized");
}

AudioBufferPool::~AudioBufferPool() {
    cleanup();
    LOGI("AudioBufferPool destroyed");
}

void AudioBufferPool::initialize(size_t maxBuffers, size_t bufferSize) {
    std::lock_guard<std::mutex> lock(poolMutex);
    this->maxBuffers = maxBuffers;
    this->defaultBufferSize = bufferSize;
    LOGI("AudioBufferPool initialized with max buffers: %zu, buffer size: %zu", maxBuffers, bufferSize);
}

AudioBufferPool::AudioBuffer* AudioBufferPool::acquireBuffer() {
    std::lock_guard<std::mutex> lock(poolMutex);

    // Try to find available buffer
    if (!availableBuffers.empty()) {
        AudioBuffer* buffer = availableBuffers.back();
        availableBuffers.pop_back();
        LOGD("Reused audio buffer from pool, available: %zu", availableBuffers.size());
        return buffer;
    }

    // Allocate new buffer if under limit
    if (allBuffers.size() < maxBuffers) {
        AudioBuffer* buffer = new AudioBuffer();
        buffer->data = new int16_t[defaultBufferSize / 2];  // S16 = 2 bytes per sample
        buffer->sizeBytes = defaultBufferSize;
        buffer->pts = 0.0;
        buffer->channels = 2;  // Stereo default
        buffer->sampleRate = 44100;  // 44.1kHz default

        allBuffers.push_back(buffer);
        bufferMemoryBytes += defaultBufferSize;

        LOGD("Allocated new audio buffer, total: %zu", allBuffers.size());
        return buffer;
    }

    // Pool exhausted - allocate temporary buffer
    AudioBuffer* tempBuffer = new AudioBuffer();
    tempBuffer->data = new int16_t[defaultBufferSize / 2];
    tempBuffer->sizeBytes = defaultBufferSize;
    tempBuffer->pts = 0.0;
    tempBuffer->channels = 2;
    tempBuffer->sampleRate = 44100;

    LOGW("Pool exhausted, allocated temporary audio buffer");
    return tempBuffer;
}

void AudioBufferPool::releaseBuffer(AudioBuffer* buffer) {
    if (!buffer) return;

    std::lock_guard<std::mutex> lock(poolMutex);

    // Check if this buffer belongs to our pool
    auto it = std::find(allBuffers.begin(), allBuffers.end(), buffer);
    if (it != allBuffers.end()) {
        // Reset buffer data but keep allocation
        memset(buffer->data, 0, buffer->sizeBytes);
        buffer->pts = 0.0;
        buffer->channels = 0;
        buffer->sampleRate = 0;
        availableBuffers.push_back(buffer);
        LOGD("Released audio buffer back to pool, available: %zu", availableBuffers.size());
    } else {
        // Temporary buffer - free it
        delete[] buffer->data;
        delete buffer;
        LOGD("Freed temporary audio buffer");
    }
}

AudioBufferPool::PoolStats AudioBufferPool::getPoolStats() const {
    std::lock_guard<std::mutex> lock(poolMutex);
    return PoolStats {
        allBuffers.size(),
        availableBuffers.size(),
        bufferMemoryBytes.load()
    };
}

size_t AudioBufferPool::getMemoryUsage() const {
    return bufferMemoryBytes.load();
}

void AudioBufferPool::cleanup() {
    std::lock_guard<std::mutex> lock(poolMutex);

    for (AudioBuffer* buffer : allBuffers) {
        delete[] buffer->data;
        delete buffer;
    }

    availableBuffers.clear();
    allBuffers.clear();
    bufferMemoryBytes = 0;

    LOGI("AudioBufferPool cleaned up");
}