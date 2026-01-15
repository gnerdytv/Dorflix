#ifndef JNI_LOGGING_H
#define JNI_LOGGING_H

#include <android/log.h>
#include <ctime>

// Enhanced JNI logging macros for comprehensive monitoring
#define JNI_LOG_TAG "DorflixJNI"

#define JNI_LOG_ENTRY(methodName) \
    LOGI("[%s_ENTRY] → %s (thread: %s)", JNI_LOG_TAG, methodName, getThreadInfo())

#define JNI_LOG_EXIT(methodName, result) \
    LOGI("[%s_EXIT] ← %s (result: %s)", JNI_LOG_TAG, methodName, result ? "SUCCESS" : "FAILED")

#define JNI_LOG_PERF(operation, startTime) \
    LOGI("[%s_PERF] ⏱️ %s took %lldms", JNI_LOG_TAG, operation, getTimeDiff(startTime))

#define JNI_LOG_ERROR(methodName, error) \
    LOGE("[%s_ERROR] ❌ %s failed: %s", JNI_LOG_TAG, methodName, error)

#define JNI_LOG_MEMORY(operation, ptr, size) \
    LOGI("[%s_MEM] 💾 %s: ptr=%p, size=%zu", JNI_LOG_TAG, operation, ptr, size)

#define JNI_LOG_THREAD(operation) \
    LOGI("[%s_THREAD] 🧵 %s (thread: %s)", JNI_LOG_TAG, operation, getThreadInfo())

// Helper functions for logging
inline const char* getThreadInfo() {
    // Simple thread info - could be enhanced with thread IDs
    return "main"; // For now, simplified
}

inline long long getTimeDiff(long long startTime) {
    return (clock() - startTime) * 1000 / CLOCKS_PER_SEC;
}

#endif // JNI_LOGGING_H