#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#include <string>

// Forward declaration for PerformanceMonitor
class PerformanceMonitor;

class AudioPlayer {
public:
    enum class PlaybackState {
        IDLE,
        PREPARING,
        READY,
        PLAYING,
        PAUSED,
        STOPPED,
        COMPLETED,
        ERROR
    };

    AudioPlayer();
    ~AudioPlayer();

    // Initialization
    bool initialize(const std::string& audioUrl);

    // Playback control
    bool prepare();
    bool play();
    bool pause();
    bool stop();
    bool seekTo(long positionMs);

    // Audio control
    void setVolume(float volume);
    void setMute(bool mute);
    float getVolume() const { return volume; }
    bool isMuted() const { return muted; }

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
    std::string audioUrl;
    PerformanceMonitor* perfMonitor;

    long currentPosition;
    long duration;
    float volume;
    bool muted;

    // TODO: Implement actual audio playback logic
    // This is a stub implementation for the engine architecture
};

#endif // AUDIO_PLAYER_H