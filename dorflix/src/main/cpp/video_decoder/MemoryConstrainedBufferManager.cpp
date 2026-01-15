#include "MemoryConstrainedBufferManager.h"
#include <malloc.h>
#include <android/log.h>

// Logging macros
#define LOG_TAG_MCBM "MemoryConstrainedBM"
#define LOGI_MCBM(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_MCBM, __VA_ARGS__)
#define LOGW_MCBM(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG_MCBM, __VA_ARGS__)

MemoryConstrainedBufferManager::MemoryConstrainedBufferManager() {
    m_is32Bit = AddressSpaceDetector::is32Bit();

    if (m_is32Bit) {
        m_maxSafeAllocation = 512 * 1024 * 1024; // 512MB for 32-bit
        m_maxVideoBuffer = 64 * 1024 * 1024;     // 64MB video buffers
        m_maxAudioBuffer = 8 * 1024 * 1024;      // 8MB audio buffers
        m_framePoolSize = 2;                     // Minimal frame pool
    } else {
        m_maxSafeAllocation = 2 * 1024 * 1024 * 1024ULL; // 2GB for 64-bit
        m_maxVideoBuffer = 256 * 1024 * 1024;    // 256MB video buffers
        m_maxAudioBuffer = 64 * 1024 * 1024;     // 64MB audio buffers
        m_framePoolSize = 8;                     // Larger frame pool
    }

    LOGI_MCBM("Memory constraints initialized: %s-bit address space", m_is32Bit ? "32" : "64");
    LOGI_MCBM("  Max safe allocation: %zu MB", m_maxSafeAllocation / (1024 * 1024));
    LOGI_MCBM("  Max video buffer: %zu MB", m_maxVideoBuffer / (1024 * 1024));
    LOGI_MCBM("  Max audio buffer: %zu MB", m_maxAudioBuffer / (1024 * 1024));
    LOGI_MCBM("  Frame pool size: %d", m_framePoolSize);
}

bool MemoryConstrainedBufferManager::isAllocationSafe(size_t size) const {
    return size <= m_maxSafeAllocation;
}

size_t MemoryConstrainedBufferManager::constrainAllocation(size_t requested) const {
    if (requested > m_maxSafeAllocation) {
        LOGW_MCBM("Constraining allocation from %zu MB to %zu MB for %s-bit safety",
                 requested / (1024 * 1024), m_maxSafeAllocation / (1024 * 1024),
                 m_is32Bit ? "32" : "64");
        return m_maxSafeAllocation;
    }
    return requested;
}

size_t MemoryConstrainedBufferManager::getMaxVideoBufferSize() const {
    return m_maxVideoBuffer;
}

size_t MemoryConstrainedBufferManager::getMaxAudioBufferSize() const {
    return m_maxAudioBuffer;
}

int MemoryConstrainedBufferManager::getRecommendedFramePoolSize() const {
    return m_framePoolSize;
}

bool MemoryConstrainedBufferManager::shouldThrottleAllocations() const {
    size_t currentUsage = getCurrentMemoryUsage();
    size_t pressureThreshold = static_cast<size_t>(m_maxSafeAllocation * MEMORY_PRESSURE_THRESHOLD);

    if (currentUsage > pressureThreshold) {
        LOGW_MCBM("Memory pressure detected: %zu MB used, threshold %zu MB",
                 currentUsage / (1024 * 1024), pressureThreshold / (1024 * 1024));
        return true;
    }

    return false;
}

void MemoryConstrainedBufferManager::logMemoryConstraints() const {
    size_t currentUsage = getCurrentMemoryUsage();

    LOGI_MCBM("=== MEMORY CONSTRAINTS ===");
    LOGI_MCBM("Address space: %s-bit", m_is32Bit ? "32" : "64");
    LOGI_MCBM("Max safe allocation: %zu MB", m_maxSafeAllocation / (1024 * 1024));
    LOGI_MCBM("Current usage: %zu MB", currentUsage / (1024 * 1024));
    LOGI_MCBM("Video buffer limit: %zu MB", m_maxVideoBuffer / (1024 * 1024));
    LOGI_MCBM("Audio buffer limit: %zu MB", m_maxAudioBuffer / (1024 * 1024));
    LOGI_MCBM("Frame pool size: %d", m_framePoolSize);
    LOGI_MCBM("Pressure threshold: %.0f%% (%zu MB)",
             MEMORY_PRESSURE_THRESHOLD * 100,
             static_cast<size_t>(m_maxSafeAllocation * MEMORY_PRESSURE_THRESHOLD) / (1024 * 1024));
    LOGI_MCBM("=========================");
}

size_t MemoryConstrainedBufferManager::getCurrentMemoryUsage() const {
    // Use mallinfo to get current heap usage
    struct mallinfo mi = mallinfo();
    return static_cast<size_t>(mi.uordblks); // Current heap usage
}