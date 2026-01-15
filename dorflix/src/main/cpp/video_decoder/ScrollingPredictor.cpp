#include "ScrollingPredictor.h"
#include <chrono>
#include <algorithm>
#include <android/log.h>

// Logging macros
#define LOG_TAG "ScrollingPredictor"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

ScrollingPredictor::ScrollingPredictor()
    : predictedDirection(ScrollDirection::STOPPED)
    , confidence(0.0)
{
    LOGI("ScrollingPredictor initialized");
}

ScrollingPredictor::~ScrollingPredictor() {
    LOGI("ScrollingPredictor destroyed");
}

void ScrollingPredictor::recordScroll(float velocity, ScrollDirection direction) {
    std::lock_guard<std::mutex> lock(predictorMutex);

    ScrollEvent event = {getCurrentTime(), velocity, direction};
    recentScrolls.push_back(event);

    // Keep only recent events
    double cutoffTime = getCurrentTime() - SCROLL_TIMEOUT;
    while (!recentScrolls.empty() && recentScrolls.front().timestamp < cutoffTime) {
        recentScrolls.pop_front();
    }

    // Limit queue size
    while (recentScrolls.size() > MAX_SCROLL_EVENTS) {
        recentScrolls.pop_front();
    }

    // Update prediction
    updatePrediction();

    LOGD("Recorded scroll: velocity=%.2f, direction=%d, events=%zu",
         velocity, static_cast<int>(direction), recentScrolls.size());
}

ScrollingPredictor::ScrollDirection ScrollingPredictor::analyzeRecentScrolls() const {
    std::lock_guard<std::mutex> lock(predictorMutex);
    return predictedDirection;
}

double ScrollingPredictor::getConfidence() const {
    std::lock_guard<std::mutex> lock(predictorMutex);
    return confidence;
}

bool ScrollingPredictor::shouldPreload(ScrollDirection direction) const {
    std::lock_guard<std::mutex> lock(predictorMutex);
    return confidence > 0.7 && predictedDirection == direction;
}

void ScrollingPredictor::updatePrediction() {
    if (recentScrolls.size() < 3) {
        predictedDirection = ScrollDirection::STOPPED;
        confidence = 0.0;
        return;
    }

    // Analyze velocity and direction consistency
    int upCount = 0;
    int downCount = 0;
    double avgVelocity = 0.0;

    for (const auto& event : recentScrolls) {
        avgVelocity += std::abs(event.velocity);

        switch (event.direction) {
            case ScrollDirection::UP: upCount++; break;
            case ScrollDirection::DOWN: downCount++; break;
            case ScrollDirection::STOPPED: break;
        }
    }

    avgVelocity /= recentScrolls.size();

    // Determine direction with majority vote
    if (upCount > downCount) {
        predictedDirection = ScrollDirection::UP;
        confidence = static_cast<double>(upCount) / recentScrolls.size();
    } else if (downCount > upCount) {
        predictedDirection = ScrollDirection::DOWN;
        confidence = static_cast<double>(downCount) / recentScrolls.size();
    } else {
        predictedDirection = ScrollDirection::STOPPED;
        confidence = 0.5;
    }

    // Boost confidence if velocity is consistent and high
    if (avgVelocity > 500) {  // Fast scrolling
        confidence = std::min(1.0, confidence + 0.2);
    }

    LOGD("Prediction updated: direction=%d, confidence=%.2f, avg_velocity=%.2f",
         static_cast<int>(predictedDirection), confidence, avgVelocity);
}

double ScrollingPredictor::getCurrentTime() const {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() / 1000.0;
}