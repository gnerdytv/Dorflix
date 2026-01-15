#ifndef MEDIA_CLOCK_H
#define MEDIA_CLOCK_H

#include <mutex>
#include <atomic>
#include <chrono>

extern "C" {
#include <libavutil/time.h>
}

/**
 * MediaClock - Multi-Source Clock System
 * Provides synchronized timing for A/V playback with drift detection
 */
class MediaClock {
public:
    // Clock source enumeration
    enum class ClockSource {
        AUDIO_MASTER,    // Audio drives sync (preferred)
        VIDEO_MASTER,    // Video drives sync (fallback)
        SYSTEM_FALLBACK  // System time (last resort)
    };

    MediaClock();
    ~MediaClock();

    // Clock updates
    void updateAudioClock(double pts);
    void updateVideoClock(double pts);

    // Clock management
    void updateMasterClock();
    double getDrift() const;
    bool needsResync() const;
    void setSource(ClockSource newSource);
    double getMasterClock() const;

    // Clock reset
    void reset();

    // Utility functions
    double getSystemTime() const;
    const char* sourceName(ClockSource source) const;

private:
    // Clock values
    double audioClock;
    double videoClock;
    double masterClock;
    ClockSource activeSource;

    // Configuration
    double driftThreshold;  // Maximum allowed drift before resync (100ms)

    // Thread safety
    mutable std::mutex clockMutex;

    // Timing utilities
    std::chrono::steady_clock::time_point startTime;
};

#endif // MEDIA_CLOCK_H