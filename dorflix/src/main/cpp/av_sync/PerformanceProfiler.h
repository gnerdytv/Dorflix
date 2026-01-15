#ifndef PERFORMANCE_PROFILER_H
#define PERFORMANCE_PROFILER_H

#include <string>
#include <map>
#include <vector>
#include <mutex>
#include <chrono>
#include <atomic>

/**
 * PerformanceProfiler - Performance Monitoring
 * Tracks operation timing and identifies bottlenecks
 */
class PerformanceProfiler {
public:
    // Timing statistics structure
    struct TimingStats {
        long totalTime = 0;
        int callCount = 0;
        long minTime = LONG_MAX;
        long maxTime = 0;
        long lastStartTime = 0;

        double getAverageTime() const {
            return callCount > 0 ? static_cast<double>(totalTime) / callCount : 0.0;
        }

        void reset() {
            totalTime = 0;
            callCount = 0;
            minTime = LONG_MAX;
            maxTime = 0;
            lastStartTime = 0;
        }
    };

    static PerformanceProfiler& getInstance();

    // Timing control
    void startTiming(const std::string& operation);
    void endTiming(const std::string& operation);

    // Statistics access
    TimingStats getStats(const std::string& operation) const;
    std::map<std::string, TimingStats> getAllStats() const;

    // Reporting
    void reportPerformance();
    void resetAll();

    // Threshold monitoring
    bool isOperationSlow(const std::string& operation, long threshold_us) const;
    std::vector<std::string> getSlowOperations(long threshold_us) const;

private:
    PerformanceProfiler();
    ~PerformanceProfiler() = default;
    PerformanceProfiler(const PerformanceProfiler&) = delete;
    PerformanceProfiler& operator=(const PerformanceProfiler&) = delete;

    std::map<std::string, TimingStats> operationStats;
    mutable std::mutex statsMutex;

    long getCurrentTimeMicros() const;
};

#endif // PERFORMANCE_PROFILER_H