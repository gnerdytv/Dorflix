#pragma once

#include <string>
#include <memory>
#include <functional>
#include <atomic>
#include <thread>
#include <android/log.h>
#include <jni.h>

// Logging macros
#define CACHE_LOG_TAG "VideoCacheManager"
#define CACHE_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, CACHE_LOG_TAG, __VA_ARGS__)
#define CACHE_LOGI(...) __android_log_print(ANDROID_LOG_INFO, CACHE_LOG_TAG, __VA_ARGS__)
#define CACHE_LOGW(...) __android_log_print(ANDROID_LOG_WARN, CACHE_LOG_TAG, __VA_ARGS__)

class VideoCacheManager {
public:
    // Callback types
    using DownloadProgressCallback = std::function<void(const std::string& url, long downloaded, long total)>;
    using DownloadCompleteCallback = std::function<void(const std::string& url, const std::string& localPath)>;
    using DownloadErrorCallback = std::function<void(const std::string& url, const std::string& error)>;

    VideoCacheManager();
    ~VideoCacheManager();

    // Initialize with cache directory
    bool initialize(const std::string& cacheDir);

    // Download video to cache (returns local path when complete)
    std::string downloadVideo(const std::string& url);

    // Check if video is cached
    bool isCached(const std::string& url);

    // Get cached file path
    std::string getCachedPath(const std::string& url);

    // Set callbacks (called from JNI)
    void setDownloadProgressCallback(DownloadProgressCallback callback);
    void setDownloadCompleteCallback(DownloadCompleteCallback callback);
    void setDownloadErrorCallback(DownloadErrorCallback callback);

    // Cache management
    void cleanupCache(); // Remove old files when cache > 500MB
    long getCacheSize(); // Current cache size in bytes

    // Start preload for next video
    void preloadVideo(const std::string& url);

private:
    std::string m_cacheDir;
    DownloadProgressCallback m_progressCallback;
    DownloadCompleteCallback m_completeCallback;
    DownloadErrorCallback m_errorCallback;

    // Generate cache filename from URL
    std::string generateCacheFilename(const std::string& url);

    // Check if file exists and is complete
    bool isFileComplete(const std::string& path);

    // Copy downloaded file to cache directory
    bool copyFileToCache(const std::string& sourcePath, const std::string& destPath);

    // JNI bridge functions (called from Java)
    static void onDownloadProgress(JNIEnv* env, jclass clazz, jstring url, jlong downloaded, jlong total);
    static void onDownloadComplete(JNIEnv* env, jclass clazz, jstring url, jstring localPath);
    static void onDownloadError(JNIEnv* env, jclass clazz, jstring url, jstring error);
};