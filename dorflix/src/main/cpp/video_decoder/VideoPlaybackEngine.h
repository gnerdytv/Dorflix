#ifndef VIDEO_PLAYBACK_ENGINE_H
#define VIDEO_PLAYBACK_ENGINE_H

#include <atomic>
#include <mutex>
#include <thread>
#include <memory>
#include <string>
#include <functional>
#include <vector>
#include <android/native_window.h>
#include "PlaybackConfig.h"
#include "VideoQualityManager.h"
#include "ScrollingPredictor.h"
#include "PerformanceMonitor.h"

class VideoPlayer;
class FramePool;
class MemoryManager;

class VideoPlaybackEngine {
public:
    VideoPlaybackEngine();
    ~VideoPlaybackEngine();

    // Initialization
    bool initialize(PlaybackConfig config);

    // Player management
    VideoPlayer* createVideoPlayer(const std::string& videoUrl, ANativeWindow* surface);
    void destroyVideoPlayer(VideoPlayer* player);

    // Engine control
    PlaybackConfig getOptimalConfig() const;
    void predictScrollDirection();
    void optimizeForScroll(ScrollingPredictor::ScrollDirection direction);

    // Access to components
    ScrollingPredictor* getScrollPredictor() { return &predictor; }
    VideoQualityManager* getQualityManager() { return &qualityMgr; }
    PerformanceMonitor* getPerformanceMonitor() { return &perfMonitor; }

    // Engine state
    bool isActive() const { return isActiveFlag; }

private:
    std::atomic<bool> isActiveFlag;
    std::mutex engineMutex;

    // Core components
    PlaybackConfig config;
    VideoQualityManager qualityMgr;
    ScrollingPredictor predictor;
    PerformanceMonitor perfMonitor;

    // Memory management
    std::unique_ptr<FramePool> framePool;
    std::unique_ptr<MemoryManager> memoryMgr;

    // Player management
    std::vector<VideoPlayer*> activePlayers;
    static constexpr int MAX_ACTIVE_PLAYERS = 3;

    // Engine methods
    void setupHardwareAcceleration();
    void preloadVideosForDirection(ScrollingPredictor::ScrollDirection direction);
    void balanceResourceAllocation();
    void increaseDecodePriority();
    void decreaseBackgroundDecodePriority();

    // Device capabilities
    struct DeviceCapabilities {
        bool hasHardwareAccel;
        int maxFps;
        int performanceScore;
        size_t memoryMB;
        bool supportsAdaptiveBitrate;
    };

    DeviceCapabilities detectDeviceCapabilities() const;
};

#endif // VIDEO_PLAYBACK_ENGINE_H