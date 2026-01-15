#ifndef AV_SYNC_CONTROLLER_H
#define AV_SYNC_CONTROLLER_H

#include "MediaClock.h"
#include "AVBuffer.h"
#include <functional>
#include <thread>
#include <atomic>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

/**
 * AVSyncController - Quality-Aware Sync Controller
 * Coordinates MediaClock and AVBuffer for perfect A/V synchronization
 */
class AVSyncController {
public:
    // Sync quality enumeration
    enum class SyncQuality {
        PERFECT,    // < 10ms drift
        GOOD,       // < 50ms drift
        POOR,       // < 200ms drift
        BROKEN      // > 200ms drift
    };

    // Callback types
    using VideoRenderCallback = std::function<void(AVFrame*)>;
    using AudioPlayCallback = std::function<void(AVPacket*)>;

    AVSyncController();
    ~AVSyncController();

    // Initialization
    void setVideoRenderCallback(VideoRenderCallback callback);
    void setAudioPlayCallback(AudioPlayCallback callback);

    // Frame/packet processing
    void processVideoFrame(AVFrame* frame);
    void processAudioPacket(AVPacket* packet);

    // Sync management
    void startSync();
    void stopSync();
    void resetSync();

    // Quality monitoring
    SyncQuality getCurrentQuality() const;
    double getCurrentDrift() const;
    bool isResyncNeeded() const;

    // Access to components
    MediaClock& getClock() { return clock; }
    AVBuffer& getBuffer() { return buffer; }

    // Quality thresholds (can be adjusted)
    static constexpr double PERFECT_THRESHOLD = 0.010;  // 10ms
    static constexpr double GOOD_THRESHOLD = 0.050;     // 50ms
    static constexpr double POOR_THRESHOLD = 0.200;     // 200ms

private:
    // Core components
    MediaClock clock;
    AVBuffer buffer;

    // State
    SyncQuality quality;
    int consecutiveSyncFailures;
    static constexpr int MAX_SYNC_FAILURES = 10;

    // Callbacks
    VideoRenderCallback videoRenderCallback;
    AudioPlayCallback audioPlayCallback;

    // Sync thread
    std::thread syncThread;
    std::atomic<bool> syncRunning;
    std::atomic<bool> syncThreadActive;

    // Methods
    void syncThreadLoop();
    void renderIfReady();
    void checkSyncQuality();
    void onQualityChanged(SyncQuality oldQuality, SyncQuality newQuality);
    void triggerResync();
    void enableFrameSkipping();
    void resyncClocks();

    // Utility
    const char* qualityName(SyncQuality quality) const;
    bool hasAudio() const;  // Check if audio is available
    void recoverFromError();  // Error recovery method
};

#endif // AV_SYNC_CONTROLLER_H