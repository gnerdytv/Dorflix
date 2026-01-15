#include "AVBuffer.h"
#include <android/log.h>

// Logging macros
#define LOG_TAG "AVBuffer"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

AVBuffer::AVBuffer()
    : maxVideoBuffers(30)  // ~1 second at 30fps
    , maxAudioBuffers(50)  // Audio buffer queue
    , frameSkippingEnabled(false)
{
    LOGI("AVBuffer initialized: maxVideoBuffers=%zu, maxAudioBuffers=%zu",
         maxVideoBuffers, maxAudioBuffers);
}

AVBuffer::~AVBuffer() {
    clear();
    LOGI("AVBuffer destroyed");
}

bool AVBuffer::addVideoFrame(AVFrame* frame, double pts) {
    std::lock_guard<std::mutex> lock(bufferMutex);

    if (videoFrames.size() >= maxVideoBuffers) {
        dropOldestVideoFrame();
    }

    auto deadline = calculateDeadline(pts);
    auto videoFrame = std::make_unique<VideoFrame>(frame, pts, deadline);
    videoFrames.push(std::move(videoFrame));

    LOGD("Added video frame: pts=%.6f, buffer size=%zu/%zu", pts, videoFrames.size(), maxVideoBuffers);

    bufferCV.notify_one();
    return true;
}

std::unique_ptr<AVBuffer::VideoFrame> AVBuffer::getNextVideoFrame(double currentTime) {
    std::lock_guard<std::mutex> lock(bufferMutex);

    if (videoFrames.empty()) {
        return nullptr;
    }

    // Convert currentTime (seconds) to steady_clock time_point
    auto now = std::chrono::steady_clock::now();
    auto currentTimePoint = now - std::chrono::microseconds(static_cast<long long>(
        (std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count() -
         static_cast<long long>(currentTime * 1000000.0))));

    auto& vf = videoFrames.front();

    // Check if frame is ready (deadline has passed)
    if (vf->deadline <= currentTimePoint) {
        auto frame = std::move(videoFrames.front());
        videoFrames.pop();
        LOGD("Retrieved video frame: pts=%.6f, buffer size=%zu", frame->pts, videoFrames.size());
        return frame;
    }

    LOGD("Video frame not ready: pts=%.6f, deadline not reached", vf->pts);
    return nullptr;
}

bool AVBuffer::addAudioPacket(AVPacket* packet, double pts) {
    std::lock_guard<std::mutex> lock(bufferMutex);

    if (audioPackets.size() >= maxAudioBuffers) {
        dropOldestAudioPacket();
    }

    auto audioPacket = std::make_unique<AudioPacket>(packet, pts, packet->size);
    audioPackets.push(std::move(audioPacket));

    LOGD("Added audio packet: pts=%.6f, size=%d, buffer size=%zu/%zu",
         pts, packet->size, audioPackets.size(), maxAudioBuffers);

    bufferCV.notify_one();
    return true;
}

std::unique_ptr<AVBuffer::AudioPacket> AVBuffer::getNextAudioPacket(double currentTime) {
    std::lock_guard<std::mutex> lock(bufferMutex);

    if (audioPackets.empty()) {
        return nullptr;
    }

    auto& ap = audioPackets.front();

    // Small tolerance for audio (100ms)
    if (ap->pts <= currentTime + 0.1) {
        auto packet = std::move(audioPackets.front());
        audioPackets.pop();
        LOGD("Retrieved audio packet: pts=%.6f, buffer size=%zu", packet->pts, audioPackets.size());
        return packet;
    }

    LOGD("Audio packet too early: pts=%.6f, current=%.6f", ap->pts, currentTime);
    return nullptr;
}

void AVBuffer::clear() {
    std::lock_guard<std::mutex> lock(bufferMutex);

    // Clear video frames
    while (!videoFrames.empty()) {
        auto& vf = videoFrames.front();
        if (vf->frame) {
            av_frame_free(&vf->frame);
        }
        videoFrames.pop();
    }

    // Clear audio packets
    while (!audioPackets.empty()) {
        auto& ap = audioPackets.front();
        if (ap->packet) {
            av_packet_free(&ap->packet);
        }
        audioPackets.pop();
    }

    LOGI("AVBuffer cleared: all frames and packets freed");
}

size_t AVBuffer::getVideoBufferSize() const {
    std::lock_guard<std::mutex> lock(bufferMutex);
    return videoFrames.size();
}

size_t AVBuffer::getAudioBufferSize() const {
    std::lock_guard<std::mutex> lock(bufferMutex);
    return audioPackets.size();
}

void AVBuffer::setMaxVideoBuffers(size_t maxBuffers) {
    std::lock_guard<std::mutex> lock(bufferMutex);
    maxVideoBuffers = maxBuffers;
    LOGI("Max video buffers set to %zu", maxBuffers);
}

void AVBuffer::setMaxAudioBuffers(size_t maxBuffers) {
    std::lock_guard<std::mutex> lock(bufferMutex);
    maxAudioBuffers = maxBuffers;
    LOGI("Max audio buffers set to %zu", maxBuffers);
}

void AVBuffer::enableFrameSkipping(bool enable) {
    std::lock_guard<std::mutex> lock(bufferMutex);
    frameSkippingEnabled = enable;
    LOGI("Frame skipping %s", enable ? "enabled" : "disabled");
}

std::chrono::steady_clock::time_point AVBuffer::calculateDeadline(double pts) {
    // Add small delay for smooth playback (40ms)
    auto now = std::chrono::steady_clock::now();
    auto delay = std::chrono::microseconds(static_cast<long long>((pts + 0.04) * 1000000.0));
    return now + delay;
}

void AVBuffer::dropOldestVideoFrame() {
    if (!videoFrames.empty()) {
        auto& vf = videoFrames.front();
        LOGW("=Ñ Dropping old video frame (buffer full): pts=%.6f, frame=%p", vf->pts, vf->frame);
        if (vf->frame) {
            LOGI("=¥ Freeing dropped video frame %p", vf->frame);
            av_frame_free(&vf->frame);
            vf->frame = nullptr;  // Prevent double-free
        } else {
            LOGW("  Dropped video frame was already null");
        }
        videoFrames.pop();
        LOGI("=Ê Video buffer size after drop: %zu/%zu", videoFrames.size(), maxVideoBuffers);
    } else {
        LOGW("  dropOldestVideoFrame called but videoFrames is empty");
    }
}

void AVBuffer::dropOldestAudioPacket() {
    if (!audioPackets.empty()) {
        auto& ap = audioPackets.front();
        LOGW("Dropping old audio packet (buffer full): pts=%.6f", ap->pts);
        if (ap->packet) {
            av_packet_free(&ap->packet);
        }
        audioPackets.pop();
    }
}