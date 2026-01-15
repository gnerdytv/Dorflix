#ifndef FRAME_BUFFER_MANAGER_H
#define FRAME_BUFFER_MANAGER_H

#include <ctime>
#include <time.h>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>

// Forward declarations
struct AVFrame;

/**
 * Frame buffer manager for efficient video frame memory management
 */
class FrameBufferManager {
public:
    FrameBufferManager();
    ~FrameBufferManager();
    
    // Frame buffer allocation and management
    AVFrame* acquireFrame();
    void releaseFrame(AVFrame* frame);
    void clear();
    
    // Memory management
    size_t getUsedMemory() const;
    size_t getMaxMemory() const;
    void setMaxMemory(size_t maxBytes);
    
    // Frame recycling
    void recycleFrame(AVFrame* frame);
    size_t getRecycledCount() const;
    
    // Statistics
    struct Statistics {
        size_t totalAllocated;
        size_t totalFreed;
        size_t currentUsed;
        size_t maxUsed;
        size_t recycledCount;
    };
    
    Statistics getStatistics() const;
    
private:
    // Frame buffer pool
    std::vector<AVFrame*> m_framePool;
    std::vector<AVFrame*> m_recycledFrames;
    
    // Memory tracking
    std::atomic<size_t> m_currentMemory{0};
    std::atomic<size_t> m_maxMemory{50 * 1024 * 1024}; // 50MB default
    std::atomic<size_t> m_totalAllocated{0};
    std::atomic<size_t> m_totalFreed{0};
    std::atomic<size_t> m_recycledCount{0};
    
    // Synchronization
    mutable std::mutex m_mutex;
    
    // Internal methods
    AVFrame* createNewFrame();
    void destroyFrame(AVFrame* frame);
    size_t calculateFrameSize(AVFrame* frame) const;
    void cleanupOldFrames();
};

#endif // FRAME_BUFFER_MANAGER_H
