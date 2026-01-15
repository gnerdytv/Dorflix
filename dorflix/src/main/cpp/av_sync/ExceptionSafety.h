#ifndef EXCEPTION_SAFETY_H
#define EXCEPTION_SAFETY_H

#include "SyncLogger.h"
#include "PerformanceProfiler.h"
#include <functional>
#include <memory>
#include <android/log.h>

// Logging macros (use local definitions to avoid conflicts)

// Safe execution macro
#define SYNC_SAFE_EXECUTE(operation, description, fallback) \
    do { \
        auto& profiler = PerformanceProfiler::getInstance(); \
        auto& logger = SyncLogger::getInstance(); \
        profiler.startTiming(description); \
        try { \
            operation; \
            profiler.endTiming(description); \
            logger.log(SyncLogger::LogLevel::DEBUG, "SAFE_EXEC", \
                      std::string(description) + " completed successfully"); \
        } catch (const std::exception& e) { \
            profiler.endTiming(description); \
            logger.log(SyncLogger::LogLevel::ERROR, "SAFE_EXEC", \
                      std::string(description) + " failed: " + std::string(e.what())); \
            fallback; \
        } catch (...) { \
            profiler.endTiming(description); \
            logger.log(SyncLogger::LogLevel::ERROR, "SAFE_EXEC", \
                      std::string(description) + " failed: Unknown exception"); \
            fallback; \
        } \
    } while(0)

// Resource guard for cleanup
class ResourceGuard {
public:
    explicit ResourceGuard(std::function<void()> cleanupFunc)
        : cleanupFunction(cleanupFunc) {}

    ~ResourceGuard() {
        if (cleanupFunction) {
            try {
                cleanupFunction();
            } catch (const std::exception& e) {
                __android_log_print(ANDROID_LOG_ERROR, "ResourceGuard", "ResourceGuard cleanup failed: %s", e.what());
            } catch (...) {
                __android_log_print(ANDROID_LOG_ERROR, "ResourceGuard", "ResourceGuard cleanup failed: Unknown exception");
            }
        }
    }

private:
    std::function<void()> cleanupFunction;
};

// Thread safety validation
class ThreadValidator {
public:
    explicit ThreadValidator(const std::string& resourceName)
        : resourceName(resourceName), ownerThread(std::thread::id()) {}

    bool acquire() {
        std::thread::id current = std::this_thread::get_id();
        std::thread::id expected = std::thread::id();

        if (ownerThread.compare_exchange_strong(expected, current)) {
            // Successfully acquired
            return true;
        } else if (ownerThread == current) {
            // Already owned by this thread
            return true;
        } else {
            // Owned by another thread
            auto& logger = SyncLogger::getInstance();
            logger.log(SyncLogger::LogLevel::ERROR, "THREAD_SAFETY",
                      "Thread safety violation in " + resourceName +
                      ": owned by " + std::to_string(*reinterpret_cast<const unsigned long*>(&expected)) +
                      ", accessed by " + std::to_string(*reinterpret_cast<const unsigned long*>(&current)));
            return false;
        }
    }

    void release() {
        ownerThread = std::thread::id();
    }

private:
    std::string resourceName;
    std::atomic<std::thread::id> ownerThread;
};

// RAII wrapper for thread validation
class ThreadLock {
public:
    explicit ThreadLock(ThreadValidator& validator) : validator(validator) {
        acquired = validator.acquire();
    }

    ~ThreadLock() {
        if (acquired) {
            validator.release();
        }
    }

    bool isValid() const { return acquired; }

private:
    ThreadValidator& validator;
    bool acquired = false;
};

// Utility functions for safe operations
class SafeOperations {
public:
    // Safe FFmpeg frame allocation/deallocation
    static AVFrame* safeAllocFrame() {
        AVFrame* frame = nullptr;
        SYNC_SAFE_EXECUTE(
            frame = av_frame_alloc(),
            "av_frame_alloc",
            frame = nullptr
        );
        return frame;
    }

    static void safeFreeFrame(AVFrame** frame) {
        if (frame && *frame) {
            SYNC_SAFE_EXECUTE(
                av_frame_free(frame),
                "av_frame_free",
                /* no fallback needed */
            );
            *frame = nullptr;
        }
    }

    // Safe FFmpeg packet allocation/deallocation
    static AVPacket* safeAllocPacket() {
        AVPacket* packet = nullptr;
        SYNC_SAFE_EXECUTE(
            packet = av_packet_alloc(),
            "av_packet_alloc",
            packet = nullptr
        );
        return packet;
    }

    static void safeFreePacket(AVPacket** packet) {
        if (packet && *packet) {
            SYNC_SAFE_EXECUTE(
                av_packet_free(packet),
                "av_packet_free",
                /* no fallback needed */
            );
            *packet = nullptr;
        }
    }

    // Safe memory operations
    template<typename T>
    static std::unique_ptr<T, std::function<void(T*)>> makeSafeUnique(T* ptr, std::function<void(T*)> deleter) {
        return std::unique_ptr<T, std::function<void(T*)>>(ptr, [deleter](T* p) {
            if (p) {
                SYNC_SAFE_EXECUTE(
                    deleter(p),
                    "safe_unique_deleter",
                    /* no fallback needed */
                );
            }
        });
    }
};

#endif // EXCEPTION_SAFETY_H