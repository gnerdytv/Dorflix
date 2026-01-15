#include "AudioQualityManager.h"
#include "PerformanceMonitor.h"
#include "PlaybackConfig.h"
#include <android/log.h>

// Logging macros
#define LOG_TAG "AudioQualityManager"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

AudioQualityManager::AudioQualityManager()
    : currentLevel(QualityLevel::HIGH)
    , perfMonitor(nullptr)
    , adaptiveBitrate(true)
    , devicePerformanceScore(70)
    , maxQueuedBuffers(10)
    , sampleRate(44100)
    , highQualityResampling(true)
    , bufferSize(4096)
    , monitoring(false)
{
    LOGI("AudioQualityManager initialized");
}

AudioQualityManager::~AudioQualityManager() {
    stopMonitoring();
    if (perfMonitor) {
        perfMonitor->stop();
        delete perfMonitor;
        perfMonitor = nullptr;
    }
    LOGI("AudioQualityManager destroyed");
}

void AudioQualityManager::initialize(const PlaybackConfig& config) {
    LOGI("Initializing AudioQualityManager with config: adaptiveBitrate=%d", config.adaptiveBitrate);

    adaptiveBitrate = config.adaptiveBitrate;

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

    LOGI("AudioQualityManager initialization complete");
}

AudioQualityManager::QualityLevel AudioQualityManager::getOptimalQualityLevel() {
    std::lock_guard<std::mutex> lock(qualityMutex);
    return assessOptimalQuality();
}

void AudioQualityManager::applyQualityLevel(QualityLevel level) {
    std::lock_guard<std::mutex> lock(qualityMutex);

    if (level != currentLevel) {
        LOGI("Changing audio quality level: %s -> %s",
             qualityName(currentLevel), qualityName(level));
        updateQualitySettings(level);
        currentLevel = level;
    }
}

int AudioQualityManager::getMaxQueuedBuffers() const {
    std::lock_guard<std::mutex> lock(qualityMutex);
    return maxQueuedBuffers;
}

int AudioQualityManager::getSampleRate() const {
    std::lock_guard<std::mutex> lock(qualityMutex);
    return sampleRate;
}

bool AudioQualityManager::shouldUseHighQualityResampling() const {
    std::lock_guard<std::mutex> lock(qualityMutex);
    return highQualityResampling;
}

size_t AudioQualityManager::getBufferSize() const {
    std::lock_guard<std::mutex> lock(qualityMutex);
    return bufferSize;
}

void AudioQualityManager::startMonitoring() {
    if (monitoring) {
        LOGW("Audio quality monitoring already running");
        return;
    }

    monitoring = true;
    monitorThread = std::thread([this]() {
        while (monitoring) {
            std::this_thread::sleep_for(std::chrono::seconds(2));  // Check every 2 seconds for audio

            if (shouldAdaptQuality()) {
                QualityLevel optimal = getOptimalQualityLevel();
                applyQualityLevel(optimal);
            }
        }
    });

    LOGI("Audio quality monitoring started");
}

void AudioQualityManager::stopMonitoring() {
    if (!monitoring) {
        return;
    }

    monitoring = false;

    if (monitorThread.joinable()) {
        monitorThread.join();
    }

    LOGI("Audio quality monitoring stopped");
}

bool AudioQualityManager::shouldAdaptQuality() const {
    if (!perfMonitor || !adaptiveBitrate) {
        return false;
    }

    float currentCpu = perfMonitor->getCpuUsage();
    size_t currentMemory = perfMonitor->getMemoryUsage();

    // Audio has lower requirements than video
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
            return false;  // Can't go lower
        default:
            return false;
    }
}

AudioQualityManager::QualityLevel AudioQualityManager::assessOptimalQuality() {
    if (!perfMonitor) {
        return QualityLevel::HIGH;  // Default fallback
    }

    float cpuUsage = perfMonitor->getCpuUsage();
    size_t memoryUsage = perfMonitor->getMemoryUsage();

    // Audio-only has lower requirements than video
    if (devicePerformanceScore >= 95 && cpuUsage < 0.2f && memoryUsage < 10 * 1024 * 1024) {
        return QualityLevel::ULTRA_HIGH;
    } else if (devicePerformanceScore >= 80 && cpuUsage < 0.4f && memoryUsage < 20 * 1024 * 1024) {
        return QualityLevel::HIGH;
    } else if (devicePerformanceScore >= 60 && cpuUsage < 0.6f && memoryUsage < 40 * 1024 * 1024) {
        return QualityLevel::MEDIUM;
    } else {
        return QualityLevel::LOW;
    }
}

void AudioQualityManager::updateQualitySettings(QualityLevel level) {
    switch (level) {
        case QualityLevel::ULTRA_HIGH:
            maxQueuedBuffers = 12;
            sampleRate = 48000;
            highQualityResampling = true;
            bufferSize = 8192;
            break;

        case QualityLevel::HIGH:
            maxQueuedBuffers = 10;
            sampleRate = 44100;
            highQualityResampling = true;
            bufferSize = 4096;
            break;

        case QualityLevel::MEDIUM:
            maxQueuedBuffers = 8;
            sampleRate = 44100;
            highQualityResampling = false;
            bufferSize = 2048;
            break;

        case QualityLevel::LOW:
            maxQueuedBuffers = 6;
            sampleRate = 22050;
            highQualityResampling = false;
            bufferSize = 1024;
            break;
    }

    LOGI("Audio quality settings updated for level %s: buffers=%d, rate=%dHz, hq_resample=%d, buf_size=%zu",
         qualityName(level), maxQueuedBuffers, sampleRate, highQualityResampling, bufferSize);
}

int AudioQualityManager::detectDevicePerformanceScore() const {
    // Basic device detection for audio (lower requirements than video)
    // In a real implementation, this would use Android's Build class and device features

    // Placeholder implementation - returns 70 (high-end device)
    return 70;
}

const char* AudioQualityManager::qualityName(QualityLevel level) const {
    switch (level) {
        case QualityLevel::ULTRA_HIGH: return "ULTRA_HIGH";
        case QualityLevel::HIGH: return "HIGH";
        case QualityLevel::MEDIUM: return "MEDIUM";
        case QualityLevel::LOW: return "LOW";
        default: return "UNKNOWN";
    }
}