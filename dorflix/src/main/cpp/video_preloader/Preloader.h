#ifndef VIDEO_PRELOADER_H
#define VIDEO_PRELOADER_H

#include <ctime>
#include <time.h>
#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>

// Forward declarations
class VideoDecoder;
class FrameBufferManager;

/**
 * Video preloader for background video caching
 */
class Preloader {
public:
    Preloader();
    ~Preloader();
    
    // Preloading management
    void preloadVideo(const std::string& videoPath);
    void preloadVideos(const std::vector<std::string>& videoPaths);
    void stopPreloading();
    void clearCache();
    
    // Cache management
    bool isVideoCached(const std::string& videoPath) const;
    void setMaxCacheSize(size_t maxBytes);
    size_t getCurrentCacheSize() const;
    
    // Preloading queue management
    void setPreloadQueueSize(int maxQueueSize);
    int getCurrentQueueSize() const;
    
    // Statistics
    struct Statistics {
        size_t totalPreloaded;
        size_t totalCached;
        size_t currentMemoryUsage;
        int activePreloaders;
        int queueSize;
    };
    
    Statistics getStatistics() const;
    
private:
    // Preloading task
    struct PreloadTask {
        std::string videoPath;
        std::string cacheKey;
        bool completed;
        bool failed;
        
        PreloadTask(const std::string& path, const std::string& key)
            : videoPath(path), cacheKey(key), completed(false), failed(false) {}
    };
    
    // Cached video data
    struct CachedVideo {
        std::string videoPath;
        std::string cacheKey;
        std::vector<uint8_t> videoData;
        size_t dataSize;
        long long lastAccessed;
        
        CachedVideo(const std::string& path, const std::string& key)
            : videoPath(path), cacheKey(key), dataSize(0), lastAccessed(0) {}
    };
    
    // Preloader worker
    class PreloaderWorker {
    public:
        PreloaderWorker(Preloader* preloader);
        ~PreloaderWorker();
        
        void start();
        void stop();
        void addTask(const std::shared_ptr<PreloadTask>& task);
        
    private:
        void workerThread();
        
        Preloader* m_preloader;
        std::thread m_workerThread;
        std::atomic<bool> m_running{false};
        std::mutex m_taskMutex;
        std::condition_variable m_taskCondition;
        std::queue<std::shared_ptr<PreloadTask>> m_taskQueue;
    };
    
    // Internal methods
    std::string generateCacheKey(const std::string& videoPath);
    void preloadTask(const std::shared_ptr<PreloadTask>& task);
    void cacheVideoData(const std::string& cacheKey, const std::vector<uint8_t>& data);
    void cleanupOldCache();
    bool shouldPreloadVideo(const std::string& videoPath);
    
    // Configuration
    std::atomic<size_t> m_maxCacheSize{100 * 1024 * 1024}; // 100MB default
    std::atomic<int> m_maxQueueSize{5};
    std::atomic<int> m_maxWorkers{3};
    
    // State
    std::atomic<bool> m_running{false};
    std::atomic<size_t> m_currentCacheSize{0};
    std::atomic<size_t> m_totalPreloaded{0};
    std::atomic<size_t> m_totalCached{0};
    
    // Data structures
    mutable std::mutex m_cacheMutex;
    std::vector<std::shared_ptr<CachedVideo>> m_cachedVideos;
    
    mutable std::mutex m_preloadersMutex;
    std::vector<std::unique_ptr<PreloaderWorker>> m_preloaders;
    
    // Synchronization
    mutable std::mutex m_queueMutex;
    std::condition_variable m_queueCondition;
    std::queue<std::shared_ptr<PreloadTask>> m_preloadQueue;
    
    // Statistics
    mutable std::mutex m_statsMutex;
    size_t m_currentMemoryUsage{0};
    int m_activePreloaders{0};
};

#endif // VIDEO_PRELOADER_H
