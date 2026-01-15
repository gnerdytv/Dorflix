#ifndef AUDIO_PLAYBACK_ENGINE_H
#define AUDIO_PLAYBACK_ENGINE_H

#include <atomic>
#include <mutex>
#include <thread>
#include <memory>
#include <string>
#include <functional>
#include <vector>
#include "PlaybackConfig.h"
#include "AudioQualityManager.h"
#include "ScrollingPredictor.h"
#include "PerformanceMonitor.h"

class AudioPlayer;
class AudioBufferPool;
class MemoryManager;

class AudioPlaybackEngine {
public:
    AudioPlaybackEngine();
    ~AudioPlaybackEngine();

    // Initialization
    bool initialize(PlaybackConfig config);

    // Player management
    AudioPlayer* createAudioPlayer(const std::string& audioUrl);
    void destroyAudioPlayer(AudioPlayer* player);

    // Engine control
    PlaybackConfig getOptimalConfig() const;
    void predictScrollDirection();
    void optimizeForScroll(ScrollingPredictor::ScrollDirection direction);

    // Access to components
    ScrollingPredictor* getScrollPredictor() { return &predictor; }
    AudioQualityManager* getQualityManager() { return &qualityMgr; }
    PerformanceMonitor* getPerformanceMonitor() { return &perfMonitor; }

    // Engine state
    bool isActive() const { return isActiveFlag; }

private:
    std::atomic<bool> isActiveFlag;
    std::mutex engineMutex;

    // Core components
    PlaybackConfig config;
    AudioQualityManager qualityMgr;
    ScrollingPredictor predictor;
    PerformanceMonitor perfMonitor;

    // Audio memory management
    std::unique_ptr<AudioBufferPool> bufferPool;
    std::unique_ptr<MemoryManager> memoryMgr;

    // Player management
    std::vector<AudioPlayer*> activePlayers;
    static constexpr int MAX_ACTIVE_AUDIOS = 3;  // As per PlaybackConfig

    // Engine methods
    void preloadAudiosForDirection(ScrollingPredictor::ScrollDirection direction);
    void balanceResourceAllocation();
    void increaseDecodePriority();
    void decreaseBackgroundDecodePriority();

    // Device capabilities for audio
    struct AudioDeviceCapabilities {
        bool supportsHighQualityAudio;
        int maxSampleRate;
        int performanceScore;
        size_t memoryMB;
        bool supportsGaplessPlayback;
    };

    AudioDeviceCapabilities detectAudioDeviceCapabilities() const;
};

#endif // AUDIO_PLAYBACK_ENGINE_H