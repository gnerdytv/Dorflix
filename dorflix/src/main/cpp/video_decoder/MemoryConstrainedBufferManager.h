#ifndef MEMORY_CONSTRAINED_BUFFER_MANAGER_H
#define MEMORY_CONSTRAINED_BUFFER_MANAGER_H

#include "AddressSpaceDetector.h"
#include <cstddef>
#include <android/log.h>

class MemoryConstrainedBufferManager {
public:
    MemoryConstrainedBufferManager();
    ~MemoryConstrainedBufferManager() = default;

    // Check if an allocation size is safe for current address space
    bool isAllocationSafe(size_t size) const;

    // Constrain an allocation request to safe limits
    size_t constrainAllocation(size_t requested) const;

    // Get maximum safe buffer size for video operations
    size_t getMaxVideoBufferSize() const;

    // Get maximum safe buffer size for audio operations
    size_t getMaxAudioBufferSize() const;

    // Get recommended frame pool size
    int getRecommendedFramePoolSize() const;

    // Check if we should throttle allocations due to memory pressure
    bool shouldThrottleAllocations() const;

    // Log current memory constraints
    void logMemoryConstraints() const;

private:
    bool m_is32Bit;
    size_t m_maxSafeAllocation;
    size_t m_maxVideoBuffer;
    size_t m_maxAudioBuffer;
    int m_framePoolSize;

    // Memory pressure thresholds (as fraction of max safe allocation)
    static constexpr float MEMORY_PRESSURE_THRESHOLD = 0.8f; // 80%

    // Get current memory usage from system
    size_t getCurrentMemoryUsage() const;
};

#endif // MEMORY_CONSTRAINED_BUFFER_MANAGER_H