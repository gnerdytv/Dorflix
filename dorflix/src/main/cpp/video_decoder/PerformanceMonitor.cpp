#include "PerformanceMonitor.h"
#include <android/log.h>
#include <limits>

// Forward declaration for VideoPlayer (to avoid circular dependency)
class VideoPlayer;

#define LOG_TAG "PerformanceMonitor"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

PerformanceMonitor::PerformanceMonitor() : isMonitoring(false) {
    LOGI("PerformanceMonitor initialized");
}

PerformanceMonitor::~PerformanceMonitor() {
    stop();
    LOGI("PerformanceMonitor destroyed");
}

void PerformanceMonitor::start() {
    if (isMonitoring) {
        LOGW("Performance monitor already running");
        return;
    }

    isMonitoring = true;
    monitorThread = std::thread(&PerformanceMonitor::monitorThreadLoop, this);
    LOGI("Performance monitor started");
}

void PerformanceMonitor::stop() {
    if (!isMonitoring) {
        return;
    }

    isMonitoring = false;

    if (monitorThread.joinable()) {
        monitorThread.join();
    }

    LOGI("Performance monitor stopped");
}

void PerformanceMonitor::registerPlayer(VideoPlayer* player) {
    std::lock_guard<std::mutex> lock(monitorMutex);
    registeredPlayers.push_back(player);
    LOGD("Player registered, total players: %zu", registeredPlayers.size());
}

void PerformanceMonitor::unregisterPlayer(VideoPlayer* player) {
    std::lock_guard<std::mutex> lock(monitorMutex);
    auto it = std::find(registeredPlayers.begin(), registeredPlayers.end(), player);
    if (it != registeredPlayers.end()) {
        registeredPlayers.erase(it);
        LOGD("Player unregistered, total players: %zu", registeredPlayers.size());
    }
}

void PerformanceMonitor::updateMetrics() {
    std::lock_guard<std::mutex> lock(monitorMutex);

    // Update global metrics
    updateCpuUsage();
    updateMemoryUsage();

    // Update player-specific metrics
    for (VideoPlayer* player : registeredPlayers) {
        updatePlayerMetrics(player);
    }
}

void PerformanceMonitor::updateMetric(const std::string& name, double value) {
    PerformanceMetrics& metric = metrics[name];

    metric.lastUpdate = getCurrentTime();
    metric.sampleCount++;
    metric.minValue = std::min(metric.minValue, value);
    metric.maxValue = std::max(metric.maxValue, value);

    // Exponential moving average
    double alpha = 0.1;
    metric.avgValue = alpha * value + (1.0 - alpha) * metric.avgValue;
}

PerformanceMetrics PerformanceMonitor::getMetric(const std::string& name) const {
    std::lock_guard<std::mutex> lock(monitorMutex);
    auto it = metrics.find(name);
    return (it != metrics.end()) ? it->second : PerformanceMetrics{};
}

double PerformanceMonitor::getCpuUsage() const {
    return getMetric("cpu_usage").avgValue;
}

size_t PerformanceMonitor::getMemoryUsage() const {
    return static_cast<size_t>(getMetric("memory_usage_mb").avgValue * 1024 * 1024);
}

double PerformanceMonitor::getAverageDecodeTime() const {
    return getMetric("avg_decode_time").avgValue;
}

double PerformanceMonitor::getAverageRenderTime() const {
    return getMetric("avg_render_time").avgValue;
}

double PerformanceMonitor::getFrameDropRate() const {
    return getMetric("frame_drop_rate").avgValue / 100.0;
}

void PerformanceMonitor::monitorThreadLoop() {
    LOGI("Monitor thread started");

    while (isMonitoring) {
        updateMetrics();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));  // 10Hz monitoring
    }

    LOGI("Monitor thread exiting");
}

void PerformanceMonitor::updateCpuUsage() {
    double cpuUsage = getCurrentCpuUsage();
    updateMetric("cpu_usage", cpuUsage);

    if (cpuUsage > 0.9) {  // 90% CPU usage
        LOGW("High CPU usage detected: %.1f%%", cpuUsage * 100.0);
    }
}

void PerformanceMonitor::updateMemoryUsage() {
    size_t memoryUsage = getCurrentMemoryUsage();
    updateMetric("memory_usage_mb", memoryUsage / (1024.0 * 1024.0));

    if (memoryUsage > 300 * 1024 * 1024) {  // 300MB
        LOGW("High memory usage detected: %.1fMB", memoryUsage / (1024.0 * 1024.0));
    }
}

void PerformanceMonitor::updatePlayerMetrics(VideoPlayer* player) {
    // This would need access to VideoPlayer's performance stats
    // For now, we'll skip this implementation as VideoPlayer doesn't exist yet
    // In the full implementation, this would call player->getPerformanceStats()
}

double PerformanceMonitor::getCurrentTime() const {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() / 1000.0;
}

double PerformanceMonitor::getCurrentCpuUsage() const {
    // Basic CPU usage estimation
    // In a real implementation, this would read from /proc/stat or similar
    // For now, return a placeholder value
    return 0.3;  // 30% placeholder
}

size_t PerformanceMonitor::getCurrentMemoryUsage() const {
    // Basic memory usage estimation
    // In a real implementation, this would read from /proc/self/status or similar
    // For now, return a placeholder value
    return 100 * 1024 * 1024;  // 100MB placeholder
}