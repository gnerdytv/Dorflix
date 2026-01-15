#include "MediaClock.h"
#include "SyncLogger.h"
#include <cmath>
#include <android/log.h>

// Logging macros
#define LOG_TAG "MediaClock"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

MediaClock::MediaClock()
    : audioClock(0.0)
    , videoClock(0.0)
    , masterClock(0.0)
    , activeSource(ClockSource::AUDIO_MASTER)
    , driftThreshold(0.1)  // 100ms max drift
    , startTime(std::chrono::steady_clock::now())
{
    LOGI("MediaClock initialized with AUDIO_MASTER source, drift threshold: %.3fs", driftThreshold);
}

MediaClock::~MediaClock() {
    LOGI("MediaClock destroyed");
}

void MediaClock::updateAudioClock(double pts) {
    std::lock_guard<std::mutex> lock(clockMutex);
    audioClock = pts;
    updateMasterClock();
    LOGD("Audio clock updated: %.6f", pts);
}

void MediaClock::updateVideoClock(double pts) {
    std::lock_guard<std::mutex> lock(clockMutex);
    videoClock = pts;
    updateMasterClock();
    LOGD("Video clock updated: %.6f", pts);
}

void MediaClock::updateMasterClock() {
    switch (activeSource) {
        case ClockSource::AUDIO_MASTER:
            masterClock = audioClock;
            break;
        case ClockSource::VIDEO_MASTER:
            masterClock = videoClock;
            break;
        case ClockSource::SYSTEM_FALLBACK:
            masterClock = getSystemTime();
            break;
    }
}

double MediaClock::getDrift() const {
    std::lock_guard<std::mutex> lock(clockMutex);
    return std::abs(audioClock - videoClock);
}

bool MediaClock::needsResync() const {
    return getDrift() > driftThreshold;
}

void MediaClock::setSource(ClockSource newSource) {
    std::lock_guard<std::mutex> lock(clockMutex);
    if (activeSource != newSource) {
        const char* oldSourceName = sourceName(activeSource);
        const char* newSourceName = sourceName(newSource);

        LOGI("Clock source changed from %s to %s", oldSourceName, newSourceName);

        // Log clock event
        SyncLogger::getInstance().logClockEvent("CLOCK_SOURCE_CHANGED",
                                              newSourceName, getDrift());

        activeSource = newSource;
        updateMasterClock();
    }
}

double MediaClock::getMasterClock() const {
    std::lock_guard<std::mutex> lock(clockMutex);
    return masterClock;
}

void MediaClock::reset() {
    std::lock_guard<std::mutex> lock(clockMutex);
    audioClock = 0.0;
    videoClock = 0.0;
    masterClock = 0.0;
    activeSource = ClockSource::AUDIO_MASTER;
    startTime = std::chrono::steady_clock::now();
    LOGI("MediaClock reset to initial state");
}

double MediaClock::getSystemTime() const {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - startTime);
    return elapsed.count() / 1000000.0;  // Convert to seconds
}

const char* MediaClock::sourceName(ClockSource source) const {
    switch (source) {
        case ClockSource::AUDIO_MASTER: return "AUDIO_MASTER";
        case ClockSource::VIDEO_MASTER: return "VIDEO_MASTER";
        case ClockSource::SYSTEM_FALLBACK: return "SYSTEM_FALLBACK";
        default: return "UNKNOWN";
    }
}