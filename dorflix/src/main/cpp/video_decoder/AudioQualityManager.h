#ifndef AUDIO_QUALITY_MANAGER_H
#define AUDIO_QUALITY_MANAGER_H

#include <mutex>
#include <atomic>
#include <thread>

class PerformanceMonitor;

class AudioQualityManager {
public:
    enum class QualityLevel {
        ULTRA_HIGH,
        HIGH,
        MEDIUM,
        LOW
    };

    struct AudioQualityThresholds {
        float ultraHighCpuUsage = 0.2f;
        float highCpuUsage = 0.4f;
        float mediumCpuUsage = 0.6f;
        size_t ultraHighMemory = 10 * 1024 * 1024;  // 10MB
        size_t highMemory = 20 * 1024 * 1024;       // 20MB
        size_t mediumMemory = 40 * 1024 * 1024;     // 40MB
    };

    AudioQualityManager();
    ~AudioQualityManager();

    // Initialization
    void initialize(const struct PlaybackConfig& config);

    // Quality management
    QualityLevel getOptimalQualityLevel();
    void applyQualityLevel(QualityLevel level);
    QualityLevel getCurrentLevel() const { return currentLevel; }

    // Audio quality control methods
    int getMaxQueuedBuffers() const;
    int getSampleRate() const;
    bool shouldUseHighQualityResampling() const;
    size_t getBufferSize() const;

    // Monitoring
    void startMonitoring();
    void stopMonitoring();
    bool shouldAdaptQuality() const;

private:
    mutable std::mutex qualityMutex;
    QualityLevel currentLevel;
    AudioQualityThresholds thresholds;
    PerformanceMonitor* perfMonitor;

    // Configuration
    bool adaptiveBitrate;
    int devicePerformanceScore;

    // Current settings
    int maxQueuedBuffers;
    int sampleRate;
    bool highQualityResampling;
    size_t bufferSize;

    std::thread monitorThread;
    std::atomic<bool> monitoring;

    // Methods
    QualityLevel assessOptimalQuality();
    void updateQualitySettings(QualityLevel level);
    int detectDevicePerformanceScore() const;
    const char* qualityName(QualityLevel level) const;
};

#endif // AUDIO_QUALITY_MANAGER_H