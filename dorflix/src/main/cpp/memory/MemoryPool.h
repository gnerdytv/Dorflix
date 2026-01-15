#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>
#include <unordered_map>

/**
 * High-performance memory pool for video processing
 */

class MemoryPool {
public:
    explicit MemoryPool(size_t poolSize);
    ~MemoryPool();
    
    // Memory allocation
    void* allocate(size_t size);
    void deallocate(void* ptr);
    
    // Pool management
    void reset();
    void clear();
    
    // Statistics
    size_t getPoolSize() const;
    size_t getUsedSize() const;
    size_t getFreeSize() const;
    size_t getAllocationCount() const;
    double getUtilization() const;
    
    // Configuration
    void setAlignment(size_t alignment);
    bool isThreadSafe() const { return true; }
    
private:
    // Memory block structure
    struct MemoryBlock {
        size_t size;
        bool isFree;
        MemoryBlock* next;
        MemoryBlock* prev;
        
        MemoryBlock(size_t s) : size(s), isFree(true), next(nullptr), prev(nullptr) {}
    };
    
    // Internal methods
    void initializePool();
    MemoryBlock* findBestFit(size_t size);
    void splitBlock(MemoryBlock* block, size_t size);
    void mergeFreeBlocks();
    void* getBlockData(MemoryBlock* block);
    MemoryBlock* getBlockFromData(void* data);
    
    // Pool data
    std::unique_ptr<uint8_t[]> m_pool;
    size_t m_poolSize;
    size_t m_usedSize;
    size_t m_alignment;
    
    // Block management
    MemoryBlock* m_head;
    size_t m_allocationCount;
    
    // Threading
    mutable std::mutex m_mutex;
    
    // Statistics
    size_t m_peakUsage;
    size_t m_totalAllocations;
    size_t m_totalDeallocations;
};

/**
 * Frame buffer manager for video frame allocation
 */
class FrameBufferManager {
public:
    FrameBufferManager();
    ~FrameBufferManager();
    
    // Frame buffer allocation
    uint8_t* allocateFrameBuffer(size_t width, size_t height, int format);
    void deallocateFrameBuffer(uint8_t* buffer);
    
    // Buffer management
    void clear();
    size_t getUsedBuffers() const;
    size_t getTotalMemory() const;
    
    // Frame format information
    struct FrameFormat {
        size_t width;
        size_t height;
        int format; // AVPixelFormat equivalent
        size_t bufferSize;
        size_t stride;
    };
    
    FrameFormat getFrameFormat(uint8_t* buffer) const;
    
private:
    struct FrameBuffer {
        uint8_t* data;
        FrameFormat format;
        bool inUse;
        
        FrameBuffer(uint8_t* d, const FrameFormat& f) : data(d), format(f), inUse(true) {}
    };
    
    MemoryPool m_memoryPool;
    std::vector<FrameBuffer> m_buffers;
    std::mutex m_mutex;
    
    size_t calculateFrameBufferSize(size_t width, size_t height, int format);
    int getBytesPerPixel(int format);
};

/**
 * Buffer allocator for efficient memory management
 */
class BufferAllocator {
public:
    BufferAllocator();
    ~BufferAllocator();
    
    // Buffer allocation
    template<typename T>
    T* allocate(size_t count) {
        std::lock_guard<std::mutex> lock(m_mutex);
        size_t size = count * sizeof(T);
        void* ptr = m_memoryPool.allocate(size);
        if (ptr) {
            m_allocations[ptr] = size;
        }
        return static_cast<T*>(ptr);
    }
    
    template<typename T>
    void deallocate(T* ptr) {
        if (!ptr) return;
        
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_allocations.find(ptr);
        if (it != m_allocations.end()) {
            m_memoryPool.deallocate(ptr);
            m_allocations.erase(it);
        }
    }
    
    // Statistics
    size_t getUsedMemory() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_memoryPool.getUsedSize();
    }
    
    size_t getAllocationCount() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_allocations.size();
    }
    
private:
    MemoryPool m_memoryPool;
    std::unordered_map<void*, size_t> m_allocations;
    mutable std::mutex m_mutex;
};

#endif // MEMORY_POOL_H
