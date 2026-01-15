#ifndef FRAME_POOL_H
#define FRAME_POOL_H

#include <vector>
#include <mutex>
#include <atomic>
#include <android/log.h>

extern "C" {
#include <libavcodec/avcodec.h>
}

class FramePool {
public:
    struct PoolStats {
        size_t totalFrames;
        size_t availableFrames;
        size_t memoryUsage;
    };

    FramePool();
    ~FramePool();

    // Initialization
    void initialize(size_t maxFrames = 50);

    // Frame management
    AVFrame* acquireFrame(int width, int height, AVPixelFormat format);
    void releaseFrame(AVFrame* frame);

    // Pool statistics
    PoolStats getPoolStats() const;
    size_t getMemoryUsage() const;

    // Pool management
    void cleanup();

private:
    mutable std::mutex poolMutex;
    std::vector<AVFrame*> availableFrames;
    std::vector<AVFrame*> allFrames;
    size_t maxFrames;
    std::atomic<size_t> frameSizeBytes;

    size_t calculateFrameSize(int width, int height, AVPixelFormat format) const;
};

#endif // FRAME_POOL_H