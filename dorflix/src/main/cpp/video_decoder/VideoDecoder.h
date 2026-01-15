#ifndef VIDEO_DECODER_H
#define VIDEO_DECODER_H

#include <ctime>
#include <time.h>
#include <string>
#include <memory>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>

class VideoCacheManager;
class AudioDecoder;
class VideoRenderer;
class AVSyncController;
class VideoDecoderBase; // Base class for different decoder implementations

class Preloader; // Forward declaration for preloader

// Forward declarations for hierarchical system
enum class RendererType;
class VideoRendererFactory;

/**
 * Video decoder interface for C++ video processing
 */

// FFmpeg headers with extern "C" to prevent C++ name mangling
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
#include <libswscale/swscale.h>
#include <libavutil/pixfmt.h>
#include <libavcodec/codec_par.h>
#include <libswresample/swresample.h>
}

// <� TIKTOK-STYLE: Constants for packet validation
const int MAX_PACKET_SIZE = 10 * 1024 * 1024; // 10MB max packet size

// Callback types
using VideoPreparedCallback = std::function<void(int duration)>;
using VideoStartedCallback = std::function<void()>;
using VideoPausedCallback = std::function<void()>;
using VideoStoppedCallback = std::function<void()>;
using VideoCompletedCallback = std::function<void()>;
using VideoErrorCallback = std::function<void(int errorCode, const std::string& errorMessage)>;
using VideoProgressCallback = std::function<void(int position)>;
using VideoBufferingCallback = std::function<void()>;
using VideoSeekCallback = std::function<void()>;
using VideoSizeCallback = std::function<void(int width, int height)>;

class VideoDecoder {
public:
    VideoDecoder();
    ~VideoDecoder();
    
    // Core playback functions
    bool loadVideo(const std::string& videoPath);
    void play();
    void pause();
    void stop();
    void seekTo(int position);
    void destroy(); // TikTok-style complete destruction
    
    // State queries
    bool isPlaying() const;
    int getCurrentPosition() const;
    int getDuration() const;
    bool isPrepared() const;
    bool isDestroyed() const;
    
    // Surface management
    void setSurface(void* surface); // ANativeWindow*
    
    // Audio control
    void setVolume(float volume);
    void setMute(bool mute);
    
    // Callback setters
    void setVideoPreparedCallback(VideoPreparedCallback callback);
    void setVideoStartedCallback(VideoStartedCallback callback);
    void setVideoPausedCallback(VideoPausedCallback callback);
    void setVideoStoppedCallback(VideoStoppedCallback callback);
    void setVideoCompletedCallback(VideoCompletedCallback callback);
    void setVideoErrorCallback(VideoErrorCallback callback);
    void setVideoProgressCallback(VideoProgressCallback callback);
    void setVideoBufferingStartedCallback(VideoBufferingCallback callback);
    void setVideoBufferingEndedCallback(VideoBufferingCallback callback);
    void setVideoSeekCompleteCallback(VideoSeekCallback callback);
    void setVideoSizeChangedCallback(VideoSizeCallback callback);
    
private:
    // Internal state
    bool m_isPlaying = false;
    bool m_isPrepared = false;
    bool m_isDestroyed = false;
    bool m_isMuted = false;
    float m_volume = 1.0f;
    
    // Video properties
    int m_duration = 0;
    int m_currentPosition = 0;
    int m_videoWidth = 0;
    int m_videoHeight = 0;
    int m_videoStreamIndex = -1;

    // FFmpeg context pointers
    AVFormatContext* m_formatContext = nullptr;
    AVFrame* m_frame = nullptr;
    SwsContext* m_swsContext = nullptr;

    // Audio decoder
    AudioDecoder* m_audioDecoder = nullptr;

    // A/V sync controller
    AVSyncController* m_syncController = nullptr;

    // Hierarchical decoder system
    std::unique_ptr<VideoDecoderBase> m_videoDecoder;

    // Hierarchical renderer system
    std::unique_ptr<VideoRenderer> m_renderer;
    RendererType m_rendererType;
    bool m_rendererTypeSelected = false;

    // Surface for rendering
    void* m_surface = nullptr; // ANativeWindow*

    // Cache manager for HTTPS downloads
    VideoCacheManager* m_cacheManager = nullptr;

    // Preloader for background video caching
    Preloader* m_preloader = nullptr;

    // Threading
    std::thread m_decodeThread;
    std::atomic<bool> m_isDecoding;
    std::atomic<bool> m_isSeeking;
    std::atomic<bool> m_isAlive;  // Thread safety flag to prevent use-after-free
    
    // Callbacks
    VideoPreparedCallback m_onVideoPrepared;
    VideoStartedCallback m_onVideoStarted;
    VideoPausedCallback m_onVideoPaused;
    VideoStoppedCallback m_onVideoStopped;
    VideoCompletedCallback m_onVideoCompleted;
    VideoErrorCallback m_onVideoError;
    VideoProgressCallback m_onVideoProgress;
    VideoBufferingCallback m_onVideoBufferingStarted;
    VideoBufferingCallback m_onVideoBufferingEnded;
    VideoSeekCallback m_onVideoSeekComplete;
    VideoSizeCallback m_onVideoSizeChanged;

    // Validation and error concealment functions
    bool isValidPacket(AVPacket* packet);
    bool isFrameValid(AVFrame* frame);
    AVFrame* concealCorruptedFrame(AVFrame* previousFrame);

    
    // Internal methods
    bool openVideoFile(const std::string& videoPath);
    bool findVideoStream();
    bool initializeVideoCodec();
    void decodeLoop();
    void renderFrame(AVFrame* frame);
    void notifyError(int errorCode, const std::string& message);
    void cleanup();

    // Critical error handling
    void shutdownOnCriticalError(int errorCode, const std::string& message);

    // Utility functions
    static int interruptCallback(void* ctx);
    bool shouldStopDecoding() const;
};

#endif // VIDEO_DECODER_H