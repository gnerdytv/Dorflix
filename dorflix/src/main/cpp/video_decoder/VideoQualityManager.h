#ifndef VIDEO_QUALITY_MANAGER_H
#define VIDEO_QUALITY_MANAGER_H

#include <mutex>
#include <atomic>
#include <thread>

class PerformanceMonitor;

class VideoQualityManager {
public:
    enum class QualityLevel {
        ULTRA_HIGH,
        HIGH,
        MEDIUM,
        LOW,
        ULTRA_LOW
    };

    struct QualityThresholds {
        float ultraHighCpuUsage = 0.3f;
        float highCpuUsage = 0.5f;
        float mediumCpuUsage = 0.7f;
        float lowCpuUsage = 0.85f;
        // Memory thresholds in MB
        size_t ultraHighMemory = 50;
        size_t highMemory = 100;
        size_t mediumMemory = 150;
        size_t lowMemory = 200;
    };

    VideoQualityManager();
    ~VideoQualityManager();

    // Initialization
    void initialize(const struct PlaybackConfig& config);

    // Quality management
    QualityLevel getOptimalQualityLevel();
    void applyQualityLevel(QualityLevel level);
    QualityLevel getCurrentLevel() const { return currentLevel; }

    // Quality control methods (to be called by video engine)
    int getMaxQueuedFrames() const;
    int getThreadCount() const;
    bool shouldEnableHardwareAccel() const;
    int getScalingAlgorithm() const; // SWS algorithm

    // Monitoring
    void startMonitoring();
    void stopMonitoring();
    bool shouldAdaptQuality() const;

private:
    mutable std::mutex qualityMutex;
    QualityLevel currentLevel;
    QualityThresholds thresholds;
    PerformanceMonitor* perfMonitor;

    // Configuration
    bool adaptiveBitrate;
    int devicePerformanceScore;

    // Current settings
    int maxQueuedFrames;
    int threadCount;
    bool hardwareAccel;
    int scalingAlgorithm;

    std::thread monitorThread;
    std::atomic<bool> monitoring;

    // Methods
    QualityLevel assessOptimalQuality();
    void updateQualitySettings(QualityLevel level);
    QualityThresholds getThresholdsForLevel(QualityLevel level) const;
    int detectDevicePerformanceScore() const;
    const char* qualityName(QualityLevel level) const;
};

#endif // VIDEO_QUALITY_MANAGER_H