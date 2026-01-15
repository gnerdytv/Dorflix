#ifndef AUDIO_BUFFER_POOL_H
#define AUDIO_BUFFER_POOL_H

#include <vector>
#include <mutex>
#include <atomic>
#include <android/log.h>

class AudioBufferPool {
public:
    struct AudioBuffer {
        int16_t* data;
        size_t sizeBytes;
        double pts;
        int channels;
        int sampleRate;
    };

    struct PoolStats {
        size_t totalBuffers;
        size_t availableBuffers;
        size_t memoryUsage;
    };

    AudioBufferPool();
    ~AudioBufferPool();

    // Initialization
    void initialize(size_t maxBuffers = 20, size_t bufferSize = 8192);

    // Buffer management
    AudioBuffer* acquireBuffer();
    void releaseBuffer(AudioBuffer* buffer);

    // Pool statistics
    PoolStats getPoolStats() const;
    size_t getMemoryUsage() const;

    // Pool management
    void cleanup();

private:
    mutable std::mutex poolMutex;
    std::vector<AudioBuffer*> availableBuffers;
    std::vector<AudioBuffer*> allBuffers;
    size_t maxBuffers;
    std::atomic<size_t> bufferMemoryBytes;
    size_t defaultBufferSize;
};

#endif // AUDIO_BUFFER_POOL_H