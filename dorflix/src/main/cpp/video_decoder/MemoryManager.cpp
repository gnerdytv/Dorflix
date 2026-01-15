#include "MemoryManager.h"
#include <android/log.h>

// Logging macros
#define LOG_TAG "MemoryManager"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

MemoryManager::MemoryManager()
    : totalMemoryLimit(100 * 1024 * 1024)  // 100MB default
    , currentUsage(0)
    , pressure(MemoryPressure::NORMAL)
{
    LOGI("MemoryManager initialized with limit: %zu MB", totalMemoryLimit / (1024 * 1024));
}

MemoryManager::~MemoryManager() {
    LOGI("MemoryManager destroyed");
}

void MemoryManager::setMemoryLimit(size_t limitBytes) {
    std::lock_guard<std::mutex> lock(memoryMutex);
    totalMemoryLimit = limitBytes;
    updateMemoryPressure();
    LOGI("Memory limit set to: %zu MB", limitBytes / (1024 * 1024));
}

bool MemoryManager::allocate(const std::string& tag, size_t sizeBytes) {
    std::lock_guard<std::mutex> lock(memoryMutex);

    if (currentUsage + sizeBytes > totalMemoryLimit) {
        // Try to free memory
        if (!attemptFreeMemory(sizeBytes)) {
            LOGE("Memory allocation failed: %s (%zu bytes)", tag.c_str(), sizeBytes);
            return false;
        }
    }

    currentUsage += sizeBytes;
    allocationSizes[tag] = sizeBytes;

    updateMemoryPressure();
    LOGD("Allocated %zu bytes for %s, total usage: %zu MB",
         sizeBytes, tag.c_str(), currentUsage / (1024 * 1024));

    return true;
}

void MemoryManager::deallocate(const std::string& tag) {
    std::lock_guard<std::mutex> lock(memoryMutex);

    if (allocationSizes.count(tag)) {
        size_t size = allocationSizes[tag];
        currentUsage -= size;
        allocationSizes.erase(tag);

        updateMemoryPressure();
        LOGD("Deallocated %zu bytes for %s, total usage: %zu MB",
             size, tag.c_str(), currentUsage / (1024 * 1024));
    }
}

MemoryManager::MemoryPressure MemoryManager::getMemoryPressure() const {
    std::lock_guard<std::mutex> lock(memoryMutex);
    return pressure;
}

bool MemoryManager::attemptFreeMemory(size_t requiredBytes) {
    // Strategy 1: Clear frame caches
    size_t freed = clearFrameCaches();
    if (currentUsage + requiredBytes <= totalMemoryLimit) {
        return true;
    }

    // Strategy 2: Reduce buffer sizes
    freed += reduceBufferSizes();
    if (currentUsage + requiredBytes <= totalMemoryLimit) {
        return true;
    }

    // Strategy 3: Stop background decoding
    stopBackgroundDecoding();
    freed += estimateBackgroundMemory();

    LOGI("Freed %zu MB through memory management strategies", freed / (1024 * 1024));

    return currentUsage + requiredBytes <= totalMemoryLimit;
}

size_t MemoryManager::clearFrameCaches() {
    // Implementation would clear various frame caches
    // For now, simulate freeing 10MB
    LOGI("Clearing frame caches");
    return 10 * 1024 * 1024;
}

size_t MemoryManager::reduceBufferSizes() {
    // Implementation would reduce buffer sizes
    // For now, simulate freeing 5MB
    LOGI("Reducing buffer sizes");
    return 5 * 1024 * 1024;
}

void MemoryManager::stopBackgroundDecoding() {
    // Implementation would stop background decoding
    LOGI("Stopping background decoding");
}

size_t MemoryManager::estimateBackgroundMemory() const {
    // Estimate memory used by background processes
    // For now, simulate 8MB
    return 8 * 1024 * 1024;
}

void MemoryManager::updateMemoryPressure() {
    double usageRatio = static_cast<double>(currentUsage) / totalMemoryLimit;

    if (usageRatio < 0.5) {
        pressure = MemoryPressure::LOW;
    } else if (usageRatio < 0.75) {
        pressure = MemoryPressure::NORMAL;
    } else if (usageRatio < 0.9) {
        pressure = MemoryPressure::HIGH;
    } else {
        pressure = MemoryPressure::CRITICAL;
    }

    if (pressure >= MemoryPressure::HIGH) {
        LOGW("Memory pressure is %s (%.1f%% usage)",
             pressure == MemoryPressure::HIGH ? "HIGH" : "CRITICAL",
             usageRatio * 100.0);
    }
}