#include "VideoQualityManager.h"
#include "PerformanceMonitor.h"
#include "PlaybackConfig.h"
#include <android/log.h>
extern "C" {
#include <libswscale/swscale.h>
}

// Logging macros
#define LOG_TAG "VideoQualityManager"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

VideoQualityManager::VideoQualityManager()
    : currentLevel(QualityLevel::HIGH)
    , perfMonitor(nullptr)
    , adaptiveBitrate(true)
    , devicePerformanceScore(70)
    , maxQueuedFrames(10)
    , threadCount(2)
    , hardwareAccel(true)
    , scalingAlgorithm(SWS_BILINEAR)
    , monitoring(false)
{
    LOGI("VideoQualityManager initialized");
}

VideoQualityManager::~VideoQualityManager() {
    stopMonitoring();
    if (perfMonitor) {
        perfMonitor->stop();
        delete perfMonitor;
        perfMonitor = nullptr;
    }
    LOGI("VideoQualityManager destroyed");
}

void VideoQualityManager::initialize(const PlaybackConfig& config) {
    LOGI("Initializing VideoQualityManager with config: adaptiveBitrate=%d, hardwareAccel=%d",
         config.adaptiveBitrate, config.enableHardwareAccel);

    adaptiveBitrate = config.adaptiveBitrate;
    hardwareAccel = config.enableHardwareAccel;

    // Create performance monitor
    if (!perfMonitor) {
        perfMonitor = new PerformanceMonitor();
        perfMonitor->start();
    }

    // Detect device capabilities
    devicePerformanceScore = detectDevicePerformanceScore();
    LOGI("Device performance score: %d", devicePerformanceScore);

    // Set optimal initial quality level
    QualityLevel optimal = assessOptimalQuality();
    applyQualityLevel(optimal);

    // Start monitoring thread
    startMonitoring();

    LOGI("VideoQualityManager initialization complete");
}

VideoQualityManager::QualityLevel VideoQualityManager::getOptimalQualityLevel() {
    std::lock_guard<std::mutex> lock(qualityMutex);
    return assessOptimalQuality();
}

void VideoQualityManager::applyQualityLevel(QualityLevel level) {
    std::lock_guard<std::mutex> lock(qualityMutex);

    if (level != currentLevel) {
        LOGI("Changing quality level: %s -> %s",
             qualityName(currentLevel), qualityName(level));
        updateQualitySettings(level);
        currentLevel = level;
    }
}

int VideoQualityManager::getMaxQueuedFrames() const {
    std::lock_guard<std::mutex> lock(qualityMutex);
    return maxQueuedFrames;
}

int VideoQualityManager::getThreadCount() const {
    std::lock_guard<std::mutex> lock(qualityMutex);
    return threadCount;
}

bool VideoQualityManager::shouldEnableHardwareAccel() const {
    std::lock_guard<std::mutex> lock(qualityMutex);
    return hardwareAccel;
}

int VideoQualityManager::getScalingAlgorithm() const {
    std::lock_guard<std::mutex> lock(qualityMutex);
    return scalingAlgorithm;
}

void VideoQualityManager::startMonitoring() {
    if (monitoring) {
        LOGW("Quality monitoring already running");
        return;
    }

    monitoring = true;
    monitorThread = std::thread([this]() {
        while (monitoring) {
            std::this_thread::sleep_for(std::chrono::seconds(2));  // Check every 2 seconds

            if (shouldAdaptQuality()) {
                QualityLevel optimal = getOptimalQualityLevel();
                applyQualityLevel(optimal);
            }
        }
    });

    LOGI("Quality monitoring started");
}

void VideoQualityManager::stopMonitoring() {
    if (!monitoring) {
        return;
    }

    monitoring = false;

    if (monitorThread.joinable()) {
        monitorThread.join();
    }

    LOGI("Quality monitoring stopped");
}

bool VideoQualityManager::shouldAdaptQuality() const {
    if (!perfMonitor || !adaptiveBitrate) {
        return false;
    }

    float currentCpu = perfMonitor->getCpuUsage();
    size_t currentMemory = perfMonitor->getMemoryUsage();

    QualityThresholds thresholds = getThresholdsForLevel(currentLevel);

    switch (currentLevel) {
        case QualityLevel::ULTRA_HIGH:
            return currentCpu > thresholds.ultraHighCpuUsage ||
                   currentMemory > thresholds.ultraHighMemory;
        case QualityLevel::HIGH:
            return currentCpu > thresholds.highCpuUsage ||
                   currentMemory > thresholds.highMemory;
        case QualityLevel::MEDIUM:
            return currentCpu > thresholds.mediumCpuUsage ||
                   currentMemory > thresholds.mediumMemory;
        case QualityLevel::LOW:
            return currentCpu > thresholds.lowCpuUsage ||
                   currentMemory > thresholds.lowMemory;
        case QualityLevel::ULTRA_LOW:
            return false;  // Can't go lower
        default:
            return false;
    }
}

VideoQualityManager::QualityLevel VideoQualityManager::assessOptimalQuality() {
    if (!perfMonitor) {
        return QualityLevel::HIGH;  // Default fallback
    }

    float cpuUsage = perfMonitor->getCpuUsage();
    size_t memoryUsage = perfMonitor->getMemoryUsage();

    // Ultra high-end devices
    if (devicePerformanceScore >= 90 && cpuUsage < 0.3f && memoryUsage < 50 * 1024 * 1024) {
        return QualityLevel::ULTRA_HIGH;
    }
    // High-end devices
    else if (devicePerformanceScore >= 70 && cpuUsage < 0.5f && memoryUsage < 100 * 1024 * 1024) {
        return QualityLevel::HIGH;
    }
    // Mid-range devices
    else if (devicePerformanceScore >= 50 && cpuUsage < 0.7f && memoryUsage < 150 * 1024 * 1024) {
        return QualityLevel::MEDIUM;
    }
    // Low-end devices
    else if (devicePerformanceScore >= 30 && cpuUsage < 0.85f && memoryUsage < 200 * 1024 * 1024) {
        return QualityLevel::LOW;
    }
    // Ultra low-end or heavily constrained
    else {
        return QualityLevel::ULTRA_LOW;
    }
}

void VideoQualityManager::updateQualitySettings(QualityLevel level) {
    switch (level) {
        case QualityLevel::ULTRA_HIGH:
            maxQueuedFrames = 15;
            threadCount = 4;  // Use more cores on high-end devices
            hardwareAccel = true;
            scalingAlgorithm = SWS_LANCZOS;
            break;

        case QualityLevel::HIGH:
            maxQueuedFrames = 12;
            threadCount = 2;
            hardwareAccel = true;
            scalingAlgorithm = SWS_BILINEAR;
            break;

        case QualityLevel::MEDIUM:
            maxQueuedFrames = 8;
            threadCount = 2;
            hardwareAccel = false;  // Disable HW accel on mid-range
            scalingAlgorithm = SWS_BILINEAR;
            break;

        case QualityLevel::LOW:
            maxQueuedFrames = 5;
            threadCount = 1;
            hardwareAccel = false;
            scalingAlgorithm = SWS_FAST_BILINEAR;
            break;

        case QualityLevel::ULTRA_LOW:
            maxQueuedFrames = 3;
            threadCount = 1;
            hardwareAccel = false;
            scalingAlgorithm = SWS_POINT;
            break;
    }

    LOGI("Quality settings updated for level %s: frames=%d, threads=%d, hwaccel=%d",
         qualityName(level), maxQueuedFrames, threadCount, hardwareAccel);
}

VideoQualityManager::QualityThresholds VideoQualityManager::getThresholdsForLevel(QualityLevel level) const {
    // Return level-specific thresholds
    QualityThresholds thresholds;

    switch (level) {
        case QualityLevel::ULTRA_HIGH:
            thresholds.ultraHighCpuUsage = 0.3f;
            thresholds.ultraHighMemory = 50 * 1024 * 1024;
            break;
        case QualityLevel::HIGH:
            thresholds.highCpuUsage = 0.5f;
            thresholds.highMemory = 100 * 1024 * 1024;
            break;
        case QualityLevel::MEDIUM:
            thresholds.mediumCpuUsage = 0.7f;
            thresholds.mediumMemory = 150 * 1024 * 1024;
            break;
        case QualityLevel::LOW:
            thresholds.lowCpuUsage = 0.85f;
            thresholds.lowMemory = 200 * 1024 * 1024;
            break;
        case QualityLevel::ULTRA_LOW:
            // No thresholds - can't adapt lower
            break;
    }

    return thresholds;
}

int VideoQualityManager::detectDevicePerformanceScore() const {
    // Basic device detection based on common Android device capabilities
    // In a real implementation, this would use Android's Build class and device features

    // Placeholder implementation - returns 70 (high-end device)
    // Real implementation would check:
    // - CPU cores and frequency
    // - RAM size
    // - GPU capabilities
    // - Android API level
    // - Device model/family

    return 70;
}

const char* VideoQualityManager::qualityName(QualityLevel level) const {
    switch (level) {
        case QualityLevel::ULTRA_HIGH: return "ULTRA_HIGH";
        case QualityLevel::HIGH: return "HIGH";
        case QualityLevel::MEDIUM: return "MEDIUM";
        case QualityLevel::LOW: return "LOW";
        case QualityLevel::ULTRA_LOW: return "ULTRA_LOW";
        default: return "UNKNOWN";
    }
}