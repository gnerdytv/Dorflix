#include "SyncLogger.h"
#include <chrono>
#include <sstream>
#include <iomanip>

// Static instance
SyncLogger& SyncLogger::getInstance() {
    static SyncLogger instance;
    return instance;
}

SyncLogger::SyncLogger()
    : running(false)
    , currentLevel(LogLevel::INFO)
{
    start();
}

SyncLogger::~SyncLogger() {
    stop();
}

void SyncLogger::start() {
    if (running) return;

    running = true;
    logThread = std::thread(&SyncLogger::processLogQueue, this);
}

void SyncLogger::stop() {
    if (!running) return;

    running = false;
    queueCV.notify_all();

    if (logThread.joinable()) {
        logThread.join();
    }
}

void SyncLogger::logSyncEvent(const std::string& eventType, double videoPTS, double audioPTS,
                              double masterClock, double drift) {
    std::map<std::string, std::variant<int, double, std::string>> metadata = {
        {"video_pts", videoPTS},
        {"audio_pts", audioPTS},
        {"master_clock", masterClock},
        {"drift_ms", drift * 1000.0}
    };

    log(LogLevel::INFO, "SYNC", eventType, metadata);
}

void SyncLogger::logPerformance(const std::string& operation, long duration_us) {
    double avgDuration = getAverageDuration(operation);

    std::map<std::string, std::variant<int, double, std::string>> metadata = {
        {"operation", operation},
        {"duration_us", static_cast<double>(duration_us)},
        {"avg_duration", avgDuration}
    };

    log(LogLevel::DEBUG, "PERF", operation + " completed", metadata);
}

void SyncLogger::logQualityChange(const std::string& oldQuality, const std::string& newQuality) {
    std::map<std::string, std::variant<int, double, std::string>> metadata = {
        {"old_quality", oldQuality},
        {"new_quality", newQuality}
    };

    log(LogLevel::WARN, "QUALITY", "Sync quality changed", metadata);
}

void SyncLogger::logBufferEvent(const std::string& event, size_t videoBuffers, size_t audioBuffers) {
    std::map<std::string, std::variant<int, double, std::string>> metadata = {
        {"event", event},
        {"video_buffers", static_cast<double>(videoBuffers)},
        {"audio_buffers", static_cast<double>(audioBuffers)}
    };

    log(LogLevel::DEBUG, "BUFFER", event, metadata);
}

void SyncLogger::logClockEvent(const std::string& event, const std::string& source, double drift) {
    std::map<std::string, std::variant<int, double, std::string>> metadata = {
        {"event", event},
        {"clock_source", source},
        {"drift_ms", drift * 1000.0}
    };

    log(LogLevel::INFO, "CLOCK", event, metadata);
}

void SyncLogger::log(LogLevel level, const std::string& tag, const std::string& message,
                    const std::map<std::string, std::variant<int, double, std::string>>& metadata) {
    auto event = std::make_unique<LogEvent>(level, tag, message);
    event->metadata = metadata;

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        eventQueue.push(std::move(event));
    }

    queueCV.notify_one();
}

void SyncLogger::setLogLevel(LogLevel level) {
    currentLevel = level;
}

SyncLogger::LogLevel SyncLogger::getLogLevel() const {
    return currentLevel;
}

void SyncLogger::processLogQueue() {
    while (running) {
        std::unique_ptr<LogEvent> event;

        {
            std::unique_lock<std::mutex> lock(queueMutex);
            queueCV.wait(lock, [this]() {
                return !eventQueue.empty() || !running;
            });

            if (!running) break;

            if (!eventQueue.empty()) {
                event = std::move(eventQueue.front());
                eventQueue.pop();
            }
        }

        if (event) {
            writeToLog(*event);
        }
    }
}

void SyncLogger::writeToLog(const LogEvent& event) {
    if (event.level >= currentLevel) {
        std::string formattedMessage = formatEvent(event);

        int androidLevel;
        switch (event.level) {
            case LogLevel::TRACE:
            case LogLevel::DEBUG:
                androidLevel = ANDROID_LOG_DEBUG;
                break;
            case LogLevel::INFO:
                androidLevel = ANDROID_LOG_INFO;
                break;
            case LogLevel::WARN:
                androidLevel = ANDROID_LOG_WARN;
                break;
            case LogLevel::ERROR:
            case LogLevel::FATAL:
                androidLevel = ANDROID_LOG_ERROR;
                break;
            default:
                androidLevel = ANDROID_LOG_INFO;
        }

        __android_log_print(androidLevel, event.tag.c_str(), "%s", formattedMessage.c_str());
    }
}

std::string SyncLogger::formatEvent(const LogEvent& event) const {
    std::stringstream ss;

    // Base message (timestamp not formatted due to Android NDK limitations)
    ss << "[" << event.tag << "] " << event.message;

    // Add metadata if present
    if (!event.metadata.empty()) {
        ss << " {";
        bool first = true;
        for (const auto& pair : event.metadata) {
            if (!first) ss << ", ";
            first = false;

            ss << pair.first << "=";

            std::visit([&ss](const auto& value) {
                using T = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<T, int>) {
                    ss << value;
                } else if constexpr (std::is_same_v<T, double>) {
                    ss << std::fixed << std::setprecision(3) << value;
                } else if constexpr (std::is_same_v<T, std::string>) {
                    ss << "\"" << value << "\"";
                }
            }, pair.second);
        }
        ss << "}";
    }

    return ss.str();
}

std::string SyncLogger::levelName(LogLevel level) const {
    switch (level) {
        case LogLevel::TRACE: return "TRACE";
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARN: return "WARN";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}

double SyncLogger::getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::seconds>(duration).count();
}

double SyncLogger::getAverageDuration(const std::string& operation) {
    std::lock_guard<std::mutex> lock(perfMutex);
    auto it = operationAvgDuration.find(operation);
    return (it != operationAvgDuration.end()) ? it->second : 0.0;
}