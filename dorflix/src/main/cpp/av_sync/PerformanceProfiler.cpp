#include "PerformanceProfiler.h"
#include <android/log.h>
#include <algorithm>
#include <iomanip>
#include <sstream>

// Logging macros
#define LOG_TAG "PerformanceProfiler"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

PerformanceProfiler& PerformanceProfiler::getInstance() {
    static PerformanceProfiler instance;
    return instance;
}

PerformanceProfiler::PerformanceProfiler() {
    LOGI("PerformanceProfiler initialized");
}

void PerformanceProfiler::startTiming(const std::string& operation) {
    std::lock_guard<std::mutex> lock(statsMutex);
    operationStats[operation].lastStartTime = getCurrentTimeMicros();
}

void PerformanceProfiler::endTiming(const std::string& operation) {
    long endTime = getCurrentTimeMicros();

    std::lock_guard<std::mutex> lock(statsMutex);
    if (operationStats.find(operation) != operationStats.end()) {
        TimingStats& stats = operationStats[operation];
        long duration = endTime - stats.lastStartTime;

        stats.totalTime += duration;
        stats.callCount++;
        stats.minTime = std::min(stats.minTime, duration);
        stats.maxTime = std::max(stats.maxTime, duration);
    }
}

PerformanceProfiler::TimingStats PerformanceProfiler::getStats(const std::string& operation) const {
    std::lock_guard<std::mutex> lock(statsMutex);
    auto it = operationStats.find(operation);
    return (it != operationStats.end()) ? it->second : TimingStats();
}

std::map<std::string, PerformanceProfiler::TimingStats> PerformanceProfiler::getAllStats() const {
    std::lock_guard<std::mutex> lock(statsMutex);
    return operationStats;
}

void PerformanceProfiler::reportPerformance() {
    std::lock_guard<std::mutex> lock(statsMutex);

    LOGI("=== PERFORMANCE REPORT ===");
    for (const auto& pair : operationStats) {
        const std::string& operation = pair.first;
        const TimingStats& stats = pair.second;

        if (stats.callCount > 0) {
            double avgTime = stats.getAverageTime();
            LOGI("%s: avg=%.2fμs, min=%ldμs, max=%ldμs, calls=%d",
                 operation.c_str(), avgTime, stats.minTime, stats.maxTime, stats.callCount);
        }
    }
    LOGI("=========================");
}

void PerformanceProfiler::resetAll() {
    std::lock_guard<std::mutex> lock(statsMutex);
    for (auto& pair : operationStats) {
        pair.second.reset();
    }
    LOGI("All performance statistics reset");
}

bool PerformanceProfiler::isOperationSlow(const std::string& operation, long threshold_us) const {
    TimingStats stats = getStats(operation);
    return stats.callCount > 0 && stats.getAverageTime() > threshold_us;
}

std::vector<std::string> PerformanceProfiler::getSlowOperations(long threshold_us) const {
    std::lock_guard<std::mutex> lock(statsMutex);
    std::vector<std::string> slowOps;

    for (const auto& pair : operationStats) {
        const TimingStats& stats = pair.second;
        if (stats.callCount > 0 && stats.getAverageTime() > threshold_us) {
            slowOps.push_back(pair.first);
        }
    }

    return slowOps;
}

long PerformanceProfiler::getCurrentTimeMicros() const {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
}