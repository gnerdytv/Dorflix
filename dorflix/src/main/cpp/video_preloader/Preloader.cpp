#include "Preloader.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <thread>
#include <algorithm>
#include <functional>
#include <ctime>

Preloader::Preloader() {
    m_running = true;
    
    // Start preloader workers
    for (int i = 0; i < m_maxWorkers.load(); ++i) {
        auto worker = std::make_unique<PreloaderWorker>(this);
        worker->start();
        m_preloaders.push_back(std::move(worker));
    }
}

Preloader::~Preloader() {
    stopPreloading();
}

void Preloader::preloadVideo(const std::string& videoPath) {
    if (!shouldPreloadVideo(videoPath)) {
        return;
    }
    
    std::string cacheKey = generateCacheKey(videoPath);
    
    // Check if already cached
    {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        for (const auto& cached : m_cachedVideos) {
            if (cached->cacheKey == cacheKey) {
                cached->lastAccessed = 0; // Simple timestamp
                return;
            }
        }
    }
    
    // Create preload task
    auto task = std::make_shared<PreloadTask>(videoPath, cacheKey);
    
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        if (m_preloadQueue.size() < static_cast<size_t>(m_maxQueueSize.load())) {
            m_preloadQueue.push(task);
        }
    }
    
    m_queueCondition.notify_one();
}

void Preloader::preloadVideos(const std::vector<std::string>& videoPaths) {
    for (const auto& path : videoPaths) {
        preloadVideo(path);
    }
}

void Preloader::stopPreloading() {
    m_running = false;
    
    // Stop all workers
    {
        std::lock_guard<std::mutex> lock(m_preloadersMutex);
        for (auto& worker : m_preloaders) {
            if (worker) {
                worker->stop();
            }
        }
    }
    
    // Clear queue
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        std::queue<std::shared_ptr<PreloadTask>> empty;
        std::swap(m_preloadQueue, empty);
    }
    
    m_queueCondition.notify_all();
}

void Preloader::clearCache() {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    m_cachedVideos.clear();
    m_currentCacheSize.store(0);
    m_totalCached.store(0);
}

bool Preloader::isVideoCached(const std::string& videoPath) const {
    std::string cacheKey = const_cast<Preloader*>(this)->generateCacheKey(videoPath);
    
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    for (const auto& cached : m_cachedVideos) {
        if (cached->cacheKey == cacheKey) {
            return true;
        }
    }
    return false;
}

void Preloader::setMaxCacheSize(size_t maxBytes) {
    m_maxCacheSize.store(maxBytes);
    cleanupOldCache();
}

size_t Preloader::getCurrentCacheSize() const {
    return m_currentCacheSize.load();
}

void Preloader::setPreloadQueueSize(int maxQueueSize) {
    m_maxQueueSize.store(maxQueueSize);
}

int Preloader::getCurrentQueueSize() const {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    return static_cast<int>(m_preloadQueue.size());
}

Preloader::Statistics Preloader::getStatistics() const {
    Statistics stats;
    
    {
        std::lock_guard<std::mutex> lock(m_statsMutex);
        stats.totalPreloaded = m_totalPreloaded.load();
        stats.totalCached = m_totalCached.load();
        stats.currentMemoryUsage = m_currentMemoryUsage;
        stats.activePreloaders = m_activePreloaders;
        stats.queueSize = getCurrentQueueSize();
    }
    
    return stats;
}

std::string Preloader::generateCacheKey(const std::string& videoPath) {
    // Simple hash-based cache key generation
    std::hash<std::string> hasher;
    size_t hash = hasher(videoPath);
    
    std::stringstream ss;
    ss << std::hex << hash;
    return "cache_" + ss.str();
}

void Preloader::preloadTask(const std::shared_ptr<PreloadTask>& task) {
    if (!task || !m_running.load()) {
        return;
    }
    
    try {
        // Read video file
        std::ifstream file(task->videoPath, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            task->failed = true;
            return;
        }
        
        size_t fileSize = file.tellg();
        file.seekg(0, std::ios::beg);
        
        std::vector<uint8_t> buffer(fileSize);
        file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
        file.close();
        
        if (file.gcount() != static_cast<std::streamsize>(fileSize)) {
            task->failed = true;
            return;
        }
        
        // Cache the video data
        cacheVideoData(task->cacheKey, buffer);
        task->completed = true;
        m_totalPreloaded.fetch_add(1);
        
    } catch (const std::exception& e) {
        task->failed = true;
    }
}

void Preloader::cacheVideoData(const std::string& cacheKey, const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    
    // Check if already cached
    for (const auto& cached : m_cachedVideos) {
        if (cached->cacheKey == cacheKey) {
            cached->videoData = data;
            cached->dataSize = data.size();
            cached->lastAccessed = 0; // Simple timestamp
            m_currentCacheSize.fetch_add(data.size());
            return;
        }
    }
    
    // Add new cached video
    auto cached = std::make_shared<CachedVideo>("", cacheKey); // videoPath will be set when we have it
    cached->videoData = data;
    cached->dataSize = data.size();
    cached->lastAccessed = 0; // Simple timestamp
    
    m_cachedVideos.push_back(cached);
    m_currentCacheSize.fetch_add(data.size());
    m_totalCached.fetch_add(1);
    
    // Cleanup old cache if needed
    cleanupOldCache();
}

void Preloader::cleanupOldCache() {
    size_t maxCache = m_maxCacheSize.load();
    size_t currentCache = m_currentCacheSize.load();
    
    if (currentCache <= maxCache) {
        return;
    }
    
    // Sort by last accessed time
    std::sort(m_cachedVideos.begin(), m_cachedVideos.end(),
              [](const std::shared_ptr<CachedVideo>& a, const std::shared_ptr<CachedVideo>& b) {
                  return a->lastAccessed < b->lastAccessed;
              });
    
    // Remove oldest videos until under limit
    while (!m_cachedVideos.empty() && currentCache > maxCache) {
        auto oldest = m_cachedVideos.front();
        m_cachedVideos.erase(m_cachedVideos.begin());
        
        currentCache -= oldest->dataSize;
        m_currentCacheSize.store(currentCache);
    }
}

bool Preloader::shouldPreloadVideo(const std::string& videoPath) {
    // Check if already cached
    if (isVideoCached(videoPath)) {
        return false;
    }
    
    // Check cache space
    size_t currentCache = m_currentCacheSize.load();
    size_t maxCache = m_maxCacheSize.load();
    
    if (currentCache >= maxCache) {
        return false;
    }
    
    return true;
}

// PreloaderWorker implementation
Preloader::PreloaderWorker::PreloaderWorker(Preloader* preloader)
    : m_preloader(preloader), m_running(false) {}

Preloader::PreloaderWorker::~PreloaderWorker() {
    stop();
}

void Preloader::PreloaderWorker::start() {
    if (m_running.load()) {
        return;
    }
    
    m_running = true;
    m_workerThread = std::thread(&PreloaderWorker::workerThread, this);
}

void Preloader::PreloaderWorker::stop() {
    if (!m_running.load()) {
        return;
    }
    
    m_running = false;
    m_taskCondition.notify_one();
    
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
}

void Preloader::PreloaderWorker::addTask(const std::shared_ptr<PreloadTask>& task) {
    if (!task || !m_running.load()) {
        return;
    }
    
    {
        std::lock_guard<std::mutex> lock(m_taskMutex);
        m_taskQueue.push(task);
    }
    
    m_taskCondition.notify_one();
}

void Preloader::PreloaderWorker::workerThread() {
    while (m_running.load()) {
        std::shared_ptr<PreloadTask> task;
        
    // Get task from queue
    {
        std::unique_lock<std::mutex> lock(m_taskMutex);
        if (!m_running.load()) {
            break;
        }
        
        if (!m_taskQueue.empty()) {
            task = m_taskQueue.front();
            m_taskQueue.pop();
        }
    }
        
        // Process task
        if (task) {
            m_preloader->preloadTask(task);
        }
        
        // Small delay to prevent busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}
