#ifndef AV_BUFFER_H
#define AV_BUFFER_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <chrono>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

/**
 * AVBuffer - Adaptive Buffering System
 * Manages synchronized buffering of video frames and audio packets
 */
class AVBuffer {
public:
    // Video frame structure
    struct VideoFrame {
        AVFrame* frame;
        double pts;
        std::chrono::steady_clock::time_point deadline;

        VideoFrame(AVFrame* f, double p, std::chrono::steady_clock::time_point d)
            : frame(f), pts(p), deadline(d) {}

        ~VideoFrame() {
            if (frame) {
                av_frame_free(&frame);
                frame = nullptr;
            }
        }
    };

    // Audio packet structure
    struct AudioPacket {
        AVPacket* packet;
        double pts;
        size_t dataSize;

        AudioPacket(AVPacket* p, double t, size_t s)
            : packet(p), pts(t), dataSize(s) {}
    };

    AVBuffer();
    ~AVBuffer();

    // Video buffer operations
    bool addVideoFrame(AVFrame* frame, double pts);
    std::unique_ptr<VideoFrame> getNextVideoFrame(double currentTime);

    // Audio buffer operations
    bool addAudioPacket(AVPacket* packet, double pts);
    std::unique_ptr<AudioPacket> getNextAudioPacket(double currentTime);

    // Buffer management
    void clear();
    size_t getVideoBufferSize() const;
    size_t getAudioBufferSize() const;

    // Configuration
    void setMaxVideoBuffers(size_t maxBuffers);
    void setMaxAudioBuffers(size_t maxBuffers);
    size_t getMaxVideoBuffers() const { return maxVideoBuffers; }
    size_t getMaxAudioBuffers() const { return maxAudioBuffers; }

    // Frame skipping for poor sync quality
    void enableFrameSkipping(bool enable);
    bool isFrameSkippingEnabled() const { return frameSkippingEnabled; }

private:
    // Video buffer
    std::queue<std::unique_ptr<VideoFrame>> videoFrames;
    size_t maxVideoBuffers;

    // Audio buffer
    std::queue<std::unique_ptr<AudioPacket>> audioPackets;
    size_t maxAudioBuffers;

    // Thread safety
    mutable std::mutex bufferMutex;
    std::condition_variable bufferCV;

    // Frame skipping
    bool frameSkippingEnabled;

    // Helper functions
    std::chrono::steady_clock::time_point calculateDeadline(double pts);
    void dropOldestVideoFrame();
    void dropOldestAudioPacket();
};

#endif // AV_BUFFER_H