#ifndef SYNC_LOGGER_H
#define SYNC_LOGGER_H

#include <string>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <map>
#include <variant>
#include <android/log.h>

/**
 * SyncLogger - Comprehensive Logging System
 * Thread-safe event logging with metadata for A/V synchronization
 */
class SyncLogger {
public:
    // Log levels
    enum class LogLevel {
        TRACE,
        DEBUG,
        INFO,
        WARN,
        ERROR,
        FATAL
    };

    // Log event structure
    struct LogEvent {
        LogLevel level;
        std::string tag;
        std::string message;
        double timestamp;
        std::map<std::string, std::variant<int, double, std::string>> metadata;

        LogEvent(LogLevel lvl, const std::string& t, const std::string& msg)
            : level(lvl), tag(t), message(msg), timestamp(getCurrentTime()) {}
    };

    static SyncLogger& getInstance();

    // Logging methods
    void logSyncEvent(const std::string& eventType, double videoPTS, double audioPTS,
                     double masterClock, double drift);
    void logPerformance(const std::string& operation, long duration_us);
    void logQualityChange(const std::string& oldQuality, const std::string& newQuality);
    void logBufferEvent(const std::string& event, size_t videoBuffers, size_t audioBuffers);
    void logClockEvent(const std::string& event, const std::string& source, double drift);

    // Generic logging
    void log(LogLevel level, const std::string& tag, const std::string& message,
             const std::map<std::string, std::variant<int, double, std::string>>& metadata = {});

    // Configuration
    void setLogLevel(LogLevel level);
    LogLevel getLogLevel() const;

    // Control
    void start();
    void stop();

private:
    SyncLogger();
    ~SyncLogger();
    SyncLogger(const SyncLogger&) = delete;
    SyncLogger& operator=(const SyncLogger&) = delete;

    // Event queue
    std::queue<std::unique_ptr<LogEvent>> eventQueue;
    std::mutex queueMutex;
    std::condition_variable queueCV;

    // Processing thread
    std::thread logThread;
    std::atomic<bool> running;
    std::atomic<LogLevel> currentLevel;

    // Methods
    void processLogQueue();
    void writeToLog(const LogEvent& event);
    std::string formatEvent(const LogEvent& event) const;
    std::string levelName(LogLevel level) const;
    static double getCurrentTime();

    // Performance tracking
    std::map<std::string, double> operationAvgDuration;
    std::mutex perfMutex;
    double getAverageDuration(const std::string& operation);
};

#endif // SYNC_LOGGER_H