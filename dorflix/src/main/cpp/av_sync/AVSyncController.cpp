#include "AVSyncController.h"
#include "SyncLogger.h"
#include "PerformanceProfiler.h"
#include "ExceptionSafety.h"
#include <android/log.h>
#include <chrono>
#include <thread>
#include <cinttypes>

// Logging macros
#define LOG_TAG "AVSyncController"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

AVSyncController::AVSyncController()
    : quality(SyncQuality::PERFECT)
    , consecutiveSyncFailures(0)
    , syncRunning(false)
    , syncThreadActive(false)
{
    LOGI("AVSyncController initialized");
}

AVSyncController::~AVSyncController() {
    stopSync();
    LOGI("AVSyncController destroyed");
}

void AVSyncController::setVideoRenderCallback(VideoRenderCallback callback) {
    videoRenderCallback = callback;
    LOGI("Video render callback set");
}

void AVSyncController::setAudioPlayCallback(AudioPlayCallback callback) {
    audioPlayCallback = callback;
    LOGI("Audio play callback set");
}

void AVSyncController::processVideoFrame(AVFrame* frame) {
    // CRITICAL: Multi-layer frame validation - prevent crashes from corrupted frames
    if (!frame) {
        LOGE("CRITICAL: AVSyncController::processVideoFrame called with null frame!");
        recoverFromError();
        return;
    }

    // Validate AVFrame structure integrity before ANY access
    if (frame->width <= 0 || frame->height <= 0) {
        LOGE("CRITICAL: Frame has invalid dimensions: %dx%d", frame->width, frame->height);
        recoverFromError();
        return;
    }

    if (!frame->data[0]) {
        LOGE("CRITICAL: Frame missing primary data buffer!");
        recoverFromError();
        return;
    }

    // Check for reasonable PTS values to detect corruption
    if (frame->pts != AV_NOPTS_VALUE && (frame->pts < -1000000000LL || frame->pts > 1000000000LL)) {
        LOGE("CRITICAL: Frame has suspicious PTS value: %" PRId64, frame->pts);
        recoverFromError();
        return;
    }

    // Additional corruption checks
    if (frame->format < 0 || frame->format >= AV_PIX_FMT_NB) {
        LOGE("CRITICAL: Frame has invalid pixel format: %d", frame->format);
        recoverFromError();
        return;
    }

    // Check for valid linesize
    if (frame->linesize[0] <= 0 || frame->linesize[0] > (frame->width * 4)) {
        LOGE("CRITICAL: Frame has suspicious linesize[0]: %d for width %d", frame->linesize[0], frame->width);
        recoverFromError();
        return;
    }

    LOGI("Frame validation passed: %dx%d format=%d pts=%" PRId64, frame->width, frame->height, frame->format, frame->pts);

    PerformanceProfiler::getInstance().startTiming("process_video_frame");

    try {
        // SAFE PTS ACCESS: Calculate PTS from frame with corruption checks
        double pts = 0.0;

        // Double-check PTS before accessing (paranoia validation)
        if (frame->pts != AV_NOPTS_VALUE) {
            if (frame->pts >= -1000000000LL && frame->pts <= 1000000000LL) {
                pts = frame->pts / 1000000.0;
                LOGD("PTS calculated from frame->pts: %.6f", pts);
            } else {
                LOGE("CRITICAL: Frame PTS became corrupted during validation: %" PRId64, frame->pts);
                recoverFromError();
                return;
            }
        } else if (frame->best_effort_timestamp != AV_NOPTS_VALUE) {
            if (frame->best_effort_timestamp >= -1000000000LL && frame->best_effort_timestamp <= 1000000000LL) {
                pts = frame->best_effort_timestamp / 1000000.0;
                LOGD("PTS calculated from best_effort_timestamp: %.6f", pts);
            } else {
                LOGE("CRITICAL: Frame best_effort_timestamp corrupted: %" PRId64, frame->best_effort_timestamp);
                recoverFromError();
                return;
            }
        } else {
            LOGW("Frame has no valid PTS, using 0.0");
            pts = 0.0;
        }

        double masterClock = clock.getMasterClock();
        double drift = clock.getDrift();

        // Log sync event
        SyncLogger::getInstance().logSyncEvent("VIDEO_FRAME_PROCESSED",
                                              pts, 0.0, masterClock, drift);

        // Update clock and add to buffer
        clock.updateVideoClock(pts);
        bool bufferResult = buffer.addVideoFrame(frame, pts);

        // Log buffer event
        SyncLogger::getInstance().logBufferEvent("VIDEO_FRAME_ADDED",
                                               buffer.getVideoBufferSize(),
                                               buffer.getAudioBufferSize());

        // Check sync quality
        checkSyncQuality();

        // Try to render if ready
        renderIfReady();

        PerformanceProfiler::getInstance().endTiming("process_video_frame");
        SyncLogger::getInstance().log(SyncLogger::LogLevel::DEBUG, "SAFE_EXEC",
                                    "process_video_frame completed successfully");
    } catch (const std::exception& e) {
        PerformanceProfiler::getInstance().endTiming("process_video_frame");
        SyncLogger::getInstance().log(SyncLogger::LogLevel::ERROR, "SAFE_EXEC",
                                    std::string("process_video_frame failed: ") + e.what());
        this->recoverFromError();
    } catch (...) {
        PerformanceProfiler::getInstance().endTiming("process_video_frame");
        SyncLogger::getInstance().log(SyncLogger::LogLevel::ERROR, "SAFE_EXEC",
                                    "process_video_frame failed: Unknown exception");
        this->recoverFromError();
    }
}

void AVSyncController::processAudioPacket(AVPacket* packet) {
    // Comprehensive packet validation - prevent crashes from corrupted packets
    if (!packet) {
        LOGE("AVSyncController::processAudioPacket called with null packet!");
        recoverFromError();
        return;
    }

    // Validate AVPacket structure integrity
    if (packet->size < 0) {
        LOGE("AVSyncController::processAudioPacket called with invalid packet size: %d", packet->size);
        recoverFromError();
        return;
    }

    // Check for reasonable PTS values to detect corruption
    if (packet->pts != AV_NOPTS_VALUE && (packet->pts < -1000000000LL || packet->pts > 1000000000LL)) {
        LOGE("AVSyncController::processAudioPacket called with suspicious PTS value: %" PRId64, packet->pts);
        recoverFromError();
        return;
    }

    PerformanceProfiler::getInstance().startTiming("process_audio_packet");

    try {
        // Calculate PTS from packet with proper time_base handling
        double pts = 0.0;
        if (packet->pts != AV_NOPTS_VALUE) {
            // Use proper time_base from format context if available
            // For now, assume microseconds (common for many formats)
            pts = packet->pts / 1000000.0;
        }

        double masterClock = clock.getMasterClock();
        double drift = clock.getDrift();

        // Log sync event
        SyncLogger::getInstance().logSyncEvent("AUDIO_PACKET_PROCESSED",
                                              0.0, pts, masterClock, drift);

        // Update clock and add to buffer
        clock.updateAudioClock(pts);
        bool bufferResult = buffer.addAudioPacket(packet, pts);

        // Log buffer event
        SyncLogger::getInstance().logBufferEvent("AUDIO_PACKET_ADDED",
                                               buffer.getVideoBufferSize(),
                                               buffer.getAudioBufferSize());

        // Check sync quality
        checkSyncQuality();

        PerformanceProfiler::getInstance().endTiming("process_audio_packet");
        SyncLogger::getInstance().log(SyncLogger::LogLevel::DEBUG, "SAFE_EXEC",
                                    "process_audio_packet completed successfully");
    } catch (const std::exception& e) {
        PerformanceProfiler::getInstance().endTiming("process_audio_packet");
        SyncLogger::getInstance().log(SyncLogger::LogLevel::ERROR, "SAFE_EXEC",
                                    std::string("process_audio_packet failed: ") + e.what());
        this->recoverFromError();
    } catch (...) {
        PerformanceProfiler::getInstance().endTiming("process_audio_packet");
        SyncLogger::getInstance().log(SyncLogger::LogLevel::ERROR, "SAFE_EXEC",
                                    "process_audio_packet failed: Unknown exception");
        this->recoverFromError();
    }
}

void AVSyncController::startSync() {
    if (syncRunning) {
        LOGW("Sync already running");
        return;
    }

    syncRunning = true;
    syncThreadActive = true;

    try {
        syncThread = std::thread(&AVSyncController::syncThreadLoop, this);
        LOGI("Sync thread started");
    } catch (const std::exception& e) {
        LOGE("Failed to start sync thread: %s", e.what());
        syncRunning = false;
        syncThreadActive = false;
    }
}

void AVSyncController::stopSync() {
    if (!syncRunning) {
        return;
    }

    LOGI("Stopping sync controller...");
    syncRunning = false;

    if (syncThread.joinable()) {
        syncThread.join();
        LOGI("Sync thread joined");
    }

    syncThreadActive = false;
}

void AVSyncController::resetSync() {
    LOGI("Resetting sync state");

    stopSync();

    clock.reset();
    buffer.clear();
    quality = SyncQuality::PERFECT;
    consecutiveSyncFailures = 0;

    LOGI("Sync reset complete");
}

AVSyncController::SyncQuality AVSyncController::getCurrentQuality() const {
    return quality;
}

double AVSyncController::getCurrentDrift() const {
    return clock.getDrift();
}

bool AVSyncController::isResyncNeeded() const {
    return clock.needsResync();
}

void AVSyncController::syncThreadLoop() {
    LOGI("Sync thread started");

    while (syncRunning && syncThreadActive) {
        try {
            // Check if we need to render
            renderIfReady();

            // Small sleep to prevent busy waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(5));

        } catch (const std::exception& e) {
            LOGE("Sync thread exception: %s", e.what());
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    LOGI("Sync thread exiting");
}

void AVSyncController::renderIfReady() {
    double currentTime = clock.getMasterClock();
    LOGD("<¯ renderIfReady called, currentTime=%.6f, videoRenderCallback=%p", currentTime, videoRenderCallback);

    // Check if we have video frame ready
    auto videoFrame = buffer.getNextVideoFrame(currentTime);
    if (videoFrame) {
        LOGD("=ù Video frame retrieved: pts=%.6f, frame=%p", videoFrame->pts, videoFrame->frame);
        if (videoRenderCallback) {
            LOGI("<¨ Calling videoRenderCallback with frame %p", videoFrame->frame);
            // CRITICAL: Validate frame before rendering to prevent crashes
            if (!videoFrame->frame) {
                LOGE("CRITICAL: Retrieved video frame has null AVFrame!");
                return;
            }

            // Validate frame integrity before rendering
            if (videoFrame->frame->width <= 0 || videoFrame->frame->height <= 0) {
                LOGE("CRITICAL: Retrieved video frame has invalid dimensions: %dx%d",
                     videoFrame->frame->width, videoFrame->frame->height);
                return;
            }

            if (!videoFrame->frame->data[0]) {
                LOGE("CRITICAL: Retrieved video frame missing data buffer!");
                return;
            }

            // Double-check PTS before accessing
            if (videoFrame->frame->pts != AV_NOPTS_VALUE &&
                (videoFrame->frame->pts < -1000000000LL || videoFrame->frame->pts > 1000000000LL)) {
                LOGE("CRITICAL: Retrieved video frame has corrupted PTS: %" PRId64, videoFrame->frame->pts);
                return;
            }

            LOGI(" Rendering validated video frame: pts=%.6f", videoFrame->pts);
            videoRenderCallback(videoFrame->frame);
            LOGD(" videoRenderCallback returned");
            // Note: frame is freed by the callback
        } else {
            LOGW("  videoRenderCallback is null, cannot render frame");
        }
    } else {
        LOGD("=í No video frame ready at currentTime=%.6f", currentTime);
    }

    // Check if we have audio packet ready
    auto audioPacket = buffer.getNextAudioPacket(currentTime);
    if (audioPacket && audioPlayCallback) {
        // Validate audio packet before playing
        if (!audioPacket->packet) {
            LOGE("CRITICAL: Retrieved audio packet has null AVPacket!");
            return;
        }

        if (audioPacket->packet->size < 0) {
            LOGE("CRITICAL: Retrieved audio packet has invalid size: %d", audioPacket->packet->size);
            return;
        }

        LOGD("Playing validated audio packet: pts=%.6f", audioPacket->pts);
        audioPlayCallback(audioPacket->packet);
        // Note: packet is freed by the callback
    }
}

void AVSyncController::checkSyncQuality() {
    double drift = clock.getDrift();

    SyncQuality newQuality;
    if (std::abs(drift) < PERFECT_THRESHOLD) {
        newQuality = SyncQuality::PERFECT;
    } else if (std::abs(drift) < GOOD_THRESHOLD) {
        newQuality = SyncQuality::GOOD;
    } else if (std::abs(drift) < POOR_THRESHOLD) {
        newQuality = SyncQuality::POOR;
    } else {
        newQuality = SyncQuality::BROKEN;
    }

    if (newQuality != quality) {
        onQualityChanged(quality, newQuality);
        quality = newQuality;
    }
}

void AVSyncController::onQualityChanged(SyncQuality oldQuality, SyncQuality newQuality) {
    LOGI("Sync quality changed: %s -> %s (drift: %.3fms)",
         qualityName(oldQuality), qualityName(newQuality), clock.getDrift() * 1000.0);

    // Log quality change event
    SyncLogger::getInstance().logQualityChange(qualityName(oldQuality), qualityName(newQuality));

    switch (newQuality) {
        case SyncQuality::PERFECT:
            // Optimal settings
            buffer.setMaxVideoBuffers(30);
            buffer.setMaxAudioBuffers(50);
            buffer.enableFrameSkipping(false);
            break;

        case SyncQuality::GOOD:
            // Minor adjustments
            buffer.setMaxVideoBuffers(25);
            break;

        case SyncQuality::POOR:
            // Start skipping frames if needed
            enableFrameSkipping();
            break;

        case SyncQuality::BROKEN:
            // Major resync needed
            triggerResync();
            break;
    }
}

void AVSyncController::triggerResync() {
    consecutiveSyncFailures++;

    if (consecutiveSyncFailures >= MAX_SYNC_FAILURES) {
        LOGE("Too many sync failures (%d), resetting completely", consecutiveSyncFailures);
        resetSync();
        return;
    }

    LOGW("Attempting resync (attempt %d/%d)", consecutiveSyncFailures, MAX_SYNC_FAILURES);
    resyncClocks();
}

void AVSyncController::enableFrameSkipping() {
    // Skip every other frame when behind
    buffer.enableFrameSkipping(true);
    LOGW("Frame skipping enabled due to poor sync quality");
}

void AVSyncController::resyncClocks() {
    // Choose best available clock source
    if (hasAudio()) {
        clock.setSource(MediaClock::ClockSource::AUDIO_MASTER);
    } else {
        clock.setSource(MediaClock::ClockSource::VIDEO_MASTER);
    }

    // Reset buffers to prevent accumulation of old data
    buffer.clear();

    LOGI("Clock resync completed");
}

const char* AVSyncController::qualityName(SyncQuality quality) const {
    switch (quality) {
        case SyncQuality::PERFECT: return "PERFECT";
        case SyncQuality::GOOD: return "GOOD";
        case SyncQuality::POOR: return "POOR";
        case SyncQuality::BROKEN: return "BROKEN";
        default: return "UNKNOWN";
    }
}

bool AVSyncController::hasAudio() const {
    // For now, assume audio is available if we have an audio callback
    // In a real implementation, this would check if audio stream exists
    return audioPlayCallback != nullptr;
}

void AVSyncController::recoverFromError() {
    LOGW("Attempting error recovery in AVSyncController");

    // Log the error state
    SyncLogger::getInstance().logSyncEvent("ERROR_RECOVERY_ATTEMPTED",
                                         0.0, 0.0, clock.getMasterClock(), clock.getDrift());

    // Reset quality to POOR to trigger conservative settings
    if (quality != SyncQuality::POOR) {
        SyncQuality oldQuality = quality;
        quality = SyncQuality::POOR;
        onQualityChanged(oldQuality, quality);
    }

    // Clear buffers to prevent corrupted data
    buffer.clear();

    // Reset consecutive failures counter
    consecutiveSyncFailures = std::max(0, consecutiveSyncFailures - 1);

    LOGI("Error recovery completed");
}