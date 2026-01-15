#ifndef PLAYBACK_CONFIG_H
#define PLAYBACK_CONFIG_H

struct PlaybackConfig {
    int targetFps = 60;
    int maxBufferSize = 1024 * 1024 * 100;  // 100MB limit
    float qualityThreshold = 0.8f;          // 80% quality minimum
    bool enableHardwareAccel = true;
    bool adaptiveBitrate = true;
    int maxConcurrentAudios = 3;            // For audio-only engine
};

#endif // PLAYBACK_CONFIG_H