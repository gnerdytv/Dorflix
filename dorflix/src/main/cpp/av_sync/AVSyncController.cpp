#include "AVSyncController.h"
#include "SyncLogger.h"
#include "PerformanceProfiler.h"
#include "ExceptionSafety.h"

#include <android/log.h>
#include <chrono>
#include <thread>
#include <cinttypes>
#include <cmath>
#include <algorithm>

// Logging macros
#define LOG_TAG "AVSyncController"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

AVSyncController::AVSyncController()
    : quality(SyncQuality::PERFECT),
      consecutiveSyncFailures(0),
      syncRunning(false),
      syncThreadActive(false) {
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
    if (!frame) {
        LOGE("processVideoFrame: null frame");
        recoverFromError();
        return;
    }

    if (frame->width <= 0 || frame->height <= 0) {
        LOGE("Invalid frame dimensions: %dx%d", frame->width, frame->height);
        recoverFromError();
        return;
    }

    if (!frame->data[0]) {
        LOGE("Frame missing data buffer");
        recoverFromError();
        return;
    }

    if (frame->format < 0 || frame->format >= AV_PIX_FMT_NB) {
        LOGE("Invalid pixel format: %d", frame->format);
        recoverFromError();
        return;
    }

    PerformanceProfiler::getInstance().startTiming("process_video_frame");

    try {
        double pts = 0.0;

        if (frame->pts != AV_NOPTS_VALUE) {
            pts = frame->pts / 1000000.0;
        } else if (frame->best_effort_timestamp != AV_NOPTS_VALUE) {
            pts = frame->best_effort_timestamp / 1000000.0;
        }

        clock.updateVideoClock(pts);
        buffer.addVideoFrame(frame, pts);

        SyncLogger::getInstance().logSyncEvent(
            "VIDEO_FRAME_PROCESSED",
            pts, 0.0,
            clock.getMasterClock(),
            clock.getDrift()
        );

        checkSyncQuality();
        renderIfReady();

        PerformanceProfiler::getInstance().endTiming("process_video_frame");
    }
    catch (...) {
        PerformanceProfiler::getInstance().endTiming("process_video_frame");
        recoverFromError();
    }
}

void AVSyncController::processAudioPacket(AVPacket* packet) {
    if (!packet) {
        LOGE("processAudioPacket: null packet");
        recoverFromError();
        return;
    }

    if (packet->size < 0) {
        LOGE("Invalid audio packet size: %d", packet->size);
        recoverFromError();
        return;
    }

    PerformanceProfiler::getInstance().startTiming("process_audio_packet");

    try {
        double pts = 0.0;
        if (packet->pts != AV_NOPTS_VALUE) {
            pts = packet->pts / 1000000.0;
        }

        clock.updateAudioClock(pts);
        buffer.addAudioPacket(packet, pts);

        SyncLogger::getInstance().logSyncEvent(
            "AUDIO_PACKET_PROCESSED",
            0.0, pts,
            clock.getMasterClock(),
            clock.getDrift()
        );

        checkSyncQuality();
        PerformanceProfiler::getInstance().endTiming("process_audio_packet");
    }
    catch (...) {
        PerformanceProfiler::getInstance().endTiming("process_audio_packet");
        recoverFromError();
    }
}

void AVSyncController::startSync() {
    if (syncRunning) {
        LOGW("Sync already running");
        return;
    }

    syncRunning = true;
    syncThreadActive = true;

    syncThread = std::thread(&AVSyncController::syncThreadLoop, this);
    LOGI("Sync thread started");
}

void AVSyncController::stopSync() {
    if (!syncRunning) return;

    syncRunning = false;
    syncThreadActive = false;

    if (syncThread.joinable()) {
        syncThread.join();
    }

    LOGI("Sync stopped");
}

void AVSyncController::resetSync() {
    stopSync();
    clock.reset();
    buffer.clear();
    quality = SyncQuality::PERFECT;
    consecutiveSyncFailures = 0;
    LOGI("Sync reset");
}

void AVSyncController::syncThreadLoop() {
    while (syncRunning) {
        try {
            renderIfReady();
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        } catch (...) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

void AVSyncController::renderIfReady() {
    double now = clock.getMasterClock();

    LOGD("renderIfReady: time=%.6f videoCB=%s audioCB=%s",
         now,
         videoRenderCallback ? "set" : "null",
         audioPlayCallback  ? "set" : "null");

    auto vf = buffer.getNextVideoFrame(now);
    if (vf && videoRenderCallback && vf->frame) {
        videoRenderCallback(vf->frame);
    }

    auto ap = buffer.getNextAudioPacket(now);
    if (ap && audioPlayCallback && ap->packet) {
        audioPlayCallback(ap->packet);
    }
}

void AVSyncController::checkSyncQuality() {
    double drift = clock.getDrift();
    SyncQuality newQuality;

    if (std::abs(drift) < PERFECT_THRESHOLD)
        newQuality = SyncQuality::PERFECT;
    else if (std::abs(drift) < GOOD_THRESHOLD)
        newQuality = SyncQuality::GOOD;
    else if (std::abs(drift) < POOR_THRESHOLD)
        newQuality = SyncQuality::POOR;
    else
        newQuality = SyncQuality::BROKEN;

    if (newQuality != quality) {
        onQualityChanged(quality, newQuality);
        quality = newQuality;
    }
}

void AVSyncController::onQualityChanged(SyncQuality oldQ, SyncQuality newQ) {
    LOGI("Sync quality: %s -> %s",
         qualityName(oldQ),
         qualityName(newQ));

    switch (newQ) {
        case SyncQuality::PERFECT:
            buffer.enableFrameSkipping(false);
            break;
        case SyncQuality::POOR:
            buffer.enableFrameSkipping(true);
            break;
        case SyncQuality::BROKEN:
            triggerResync();
            break;
        default:
            break;
    }
}

void AVSyncController::triggerResync() {
    consecutiveSyncFailures++;
    if (consecutiveSyncFailures >= MAX_SYNC_FAILURES) {
        resetSync();
    } else {
        resyncClocks();
    }
}

void AVSyncController::resyncClocks() {
    clock.setSource(hasAudio()
        ? MediaClock::ClockSource::AUDIO_MASTER
        : MediaClock::ClockSource::VIDEO_MASTER);

    buffer.clear();
    LOGI("Clocks resynced");
}

bool AVSyncController::hasAudio() const {
    return audioPlayCallback != nullptr;
}

const char* AVSyncController::qualityName(SyncQuality q) const {
    switch (q) {
        case SyncQuality::PERFECT: return "PERFECT";
        case SyncQuality::GOOD:    return "GOOD";
        case SyncQuality::POOR:    return "POOR";
        case SyncQuality::BROKEN:  return "BROKEN";
        default:                   return "UNKNOWN";
    }
}

void AVSyncController::recoverFromError() {
    LOGW("Recovering from sync error");
    buffer.clear();
    consecutiveSyncFailures = std::max(0, consecutiveSyncFailures - 1);
    quality = SyncQuality::POOR;
}
