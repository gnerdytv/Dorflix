#ifndef VIDEO_PLAYER_H
#define VIDEO_PLAYER_H

#include <string>
#include <android/native_window.h>

// Forward declaration for PerformanceMonitor
class PerformanceMonitor;

class VideoPlayer {
public:
    enum class PlaybackState {
        IDLE,
        PREPARING,
        READY,
        PLAYING,
        PAUSED,
        STOPPED,
        ERROR,
        COMPLETED
    };

    VideoPlayer();
    ~VideoPlayer();

    // Initialization
    bool initialize(const std::string& videoUrl, ANativeWindow* surface);

    // Playback control
    bool prepare();
    bool play();
    bool pause();
    bool stop();
    bool seekTo(long positionMs);

    // State queries
    PlaybackState getState() const { return state; }
    long getCurrentPosition() const;
    long getDuration() const;
    bool isPlaying() const { return state == PlaybackState::PLAYING; }

    // Performance monitoring
    void registerWithMonitor(PerformanceMonitor* monitor);
    void unregisterFromMonitor();

private:
    PlaybackState state;
    std::string videoUrl;
    ANativeWindow* surface;
    PerformanceMonitor* perfMonitor;

    long currentPosition;
    long duration;

    // TODO: Implement actual video playback logic
    // This is a stub implementation for the engine architecture
};

#endif // VIDEO_PLAYER_H