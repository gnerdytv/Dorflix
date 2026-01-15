#ifndef SCROLLING_PREDICTOR_H
#define SCROLLING_PREDICTOR_H

#include <deque>
#include <mutex>
#include <atomic>

class ScrollingPredictor {
public:
    enum class ScrollDirection {
        UP,
        DOWN,
        STOPPED
    };

    struct ScrollEvent {
        double timestamp;
        float velocity;
        ScrollDirection direction;
    };

    ScrollingPredictor();
    ~ScrollingPredictor();

    // Record scroll events
    void recordScroll(float velocity, ScrollDirection direction);

    // Get predictions
    ScrollDirection analyzeRecentScrolls() const;
    double getConfidence() const;
    bool shouldPreload(ScrollDirection direction) const;

private:
    mutable std::mutex predictorMutex;
    std::deque<ScrollEvent> recentScrolls;
    ScrollDirection predictedDirection;
    double confidence;

    static constexpr size_t MAX_SCROLL_EVENTS = 10;
    static constexpr double SCROLL_TIMEOUT = 2.0; // seconds

    void updatePrediction();
    double getCurrentTime() const;
};

#endif // SCROLLING_PREDICTOR_H