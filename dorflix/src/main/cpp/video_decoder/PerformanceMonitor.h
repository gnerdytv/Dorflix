#ifndef PERFORMANCE_MONITOR_H
#define PERFORMANCE_MONITOR_H

#include <map>
#include <string>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>
#include <vector>

class VideoPlayer;

struct PerformanceMetrics {
    double avgValue = 0.0;
    double minValue = std::numeric_limits<double>::max();
    double maxValue = std::numeric_limits<double>::lowest();
    int sampleCount = 0;
    double lastUpdate = 0.0;
};

class PerformanceMonitor {
public:
    PerformanceMonitor();
    ~PerformanceMonitor();

    // Lifecycle
    void start();
    void stop();

    // Player registration
    void registerPlayer(VideoPlayer* player);
    void unregisterPlayer(VideoPlayer* player);

    // Metrics collection
    void updateMetrics();
    void updateMetric(const std::string& name, double value);

    // Metrics access
    PerformanceMetrics getMetric(const std::string& name) const;
    double getCpuUsage() const;
    size_t getMemoryUsage() const;
    double getAverageDecodeTime() const;
    double getAverageRenderTime() const;
    double getFrameDropRate() const;

private:
    mutable std::mutex monitorMutex;
    std::map<std::string, PerformanceMetrics> metrics;
    std::vector<VideoPlayer*> registeredPlayers;

    std::thread monitorThread;
    std::atomic<bool> isMonitoring;

    // Internal methods
    void monitorThreadLoop();
    void updateCpuUsage();
    void updateMemoryUsage();
    void updatePlayerMetrics(VideoPlayer* player);
    double getCurrentTime() const;

    // Platform-specific implementations
    double getCurrentCpuUsage() const;
    size_t getCurrentMemoryUsage() const;
};

#endif // PERFORMANCE_MONITOR_H