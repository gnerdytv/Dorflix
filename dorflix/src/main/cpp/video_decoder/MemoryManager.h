#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <map>
#include <string>
#include <mutex>
#include <atomic>

class MemoryManager {
public:
    enum class MemoryPressure {
        LOW,
        NORMAL,
        HIGH,
        CRITICAL
    };

    MemoryManager();
    ~MemoryManager();

    // Initialization
    void setMemoryLimit(size_t limitBytes);

    // Memory allocation/deallocation
    bool allocate(const std::string& tag, size_t sizeBytes);
    void deallocate(const std::string& tag);

    // Memory pressure management
    MemoryPressure getMemoryPressure() const;
    size_t getCurrentUsage() const { return currentUsage; }
    size_t getMemoryLimit() const { return totalMemoryLimit; }

    // Memory freeing strategies
    bool attemptFreeMemory(size_t requiredBytes);
    size_t clearFrameCaches();
    size_t reduceBufferSizes();
    void stopBackgroundDecoding();

    size_t estimateBackgroundMemory() const;

private:
    mutable std::mutex memoryMutex;
    size_t totalMemoryLimit;
    std::atomic<size_t> currentUsage;
    std::map<std::string, size_t> allocationSizes;
    MemoryPressure pressure;

    void updateMemoryPressure();
};

#endif // MEMORY_MANAGER_H