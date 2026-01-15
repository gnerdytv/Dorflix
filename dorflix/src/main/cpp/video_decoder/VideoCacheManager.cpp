#include "VideoCacheManager.h"
#include <jni.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <sys/stat.h>
#include <dirent.h>
#include <algorithm>
#include <vector>
#include <fstream>

VideoCacheManager::VideoCacheManager() {
    CACHE_LOGI("VideoCacheManager created");
}

VideoCacheManager::~VideoCacheManager() {
    CACHE_LOGI("VideoCacheManager destroyed");
}

bool VideoCacheManager::initialize(const std::string& cacheDir) {
    m_cacheDir = cacheDir;

    // Create cache directory if it doesn't exist
    struct stat st;
    if (stat(cacheDir.c_str(), &st) != 0) {
        if (mkdir(cacheDir.c_str(), 0755) != 0) {
            CACHE_LOGE("Failed to create cache directory: %s", cacheDir.c_str());
            return false;
        }
    }

    CACHE_LOGI("VideoCacheManager initialized with cache dir: %s", cacheDir.c_str());
    return true;
}

std::string VideoCacheManager::downloadVideo(const std::string& url) {
    CACHE_LOGI("Requesting download for: %s", url.c_str());

    // Check if already cached
    if (isCached(url)) {
        std::string cachedPath = getCachedPath(url);
        CACHE_LOGI("Video already cached: %s", cachedPath.c_str());
        return cachedPath;
    }

    // Generate cache filename and full path
    std::string cacheFilename = generateCacheFilename(url);
    std::string cachePath = m_cacheDir + "/" + cacheFilename;
    CACHE_LOGI("Cache filename: %s, full path: %s", cacheFilename.c_str(), cachePath.c_str());

    // Check if we have a global video downloader instance
    if (!g_videoDownloader) {
        CACHE_LOGE("No global video downloader available");
        return "";
    }

    // Use the global video downloader to download the video
    CACHE_LOGI("Starting download via global video downloader...");
    std::string downloadedPath = g_videoDownloader->downloadVideo(url);

    if (downloadedPath.empty()) {
        CACHE_LOGE("Download failed - empty path returned");
        return "";
    }

    // Check if the downloaded file exists and copy it to cache if needed
    struct stat st;
    if (stat(downloadedPath.c_str(), &st) != 0) {
        CACHE_LOGE("Downloaded file does not exist: %s", downloadedPath.c_str());
        return "";
    }

    // If the downloaded file is already in the cache directory, we're done
    if (downloadedPath.find(m_cacheDir) == 0) {
        CACHE_LOGI("Video downloaded directly to cache: %s", downloadedPath.c_str());
        return downloadedPath;
    }

    // Copy the downloaded file to cache directory
    CACHE_LOGI("Copying downloaded file to cache directory...");
    if (!copyFileToCache(downloadedPath, cachePath)) {
        CACHE_LOGE("Failed to copy downloaded file to cache");
        return "";
    }

    CACHE_LOGI("Video successfully downloaded and cached: %s", cachePath.c_str());
    return cachePath;
}

bool VideoCacheManager::isCached(const std::string& url) {
    std::string cachePath = getCachedPath(url);
    return isFileComplete(cachePath);
}

std::string VideoCacheManager::getCachedPath(const std::string& url) {
    std::string filename = generateCacheFilename(url);
    return m_cacheDir + "/" + filename;
}

void VideoCacheManager::setDownloadProgressCallback(DownloadProgressCallback callback) {
    m_progressCallback = callback;
}

void VideoCacheManager::setDownloadCompleteCallback(DownloadCompleteCallback callback) {
    m_completeCallback = callback;
}

void VideoCacheManager::setDownloadErrorCallback(DownloadErrorCallback callback) {
    m_errorCallback = callback;
}

void VideoCacheManager::cleanupCache() {
    const long MAX_CACHE_SIZE = 500 * 1024 * 1024; // 500MB

    if (getCacheSize() <= MAX_CACHE_SIZE) {
        return; // No cleanup needed
    }

    CACHE_LOGI("Cache size exceeds 500MB, starting cleanup");

    // Get all cache files with their modification times
    struct FileInfo {
        std::string path;
        time_t mtime;
        long size;
    };

    std::vector<FileInfo> files;
    DIR* dir = opendir(m_cacheDir.c_str());
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (entry->d_name[0] == '.') continue; // Skip hidden files

            std::string filepath = m_cacheDir + "/" + entry->d_name;
            struct stat st;
            if (stat(filepath.c_str(), &st) == 0) {
                files.push_back({filepath, st.st_mtime, static_cast<long>(st.st_size)});
            }
        }
        closedir(dir);
    }

    // Sort by modification time (oldest first)
    std::sort(files.begin(), files.end(), [](const FileInfo& a, const FileInfo& b) {
        return a.mtime < b.mtime;
    });

    // Remove oldest files until cache is under 400MB (leave some buffer)
    const long TARGET_SIZE = 400 * 1024 * 1024;
    long currentSize = getCacheSize();

    for (const auto& file : files) {
        if (currentSize <= TARGET_SIZE) break;

        if (remove(file.path.c_str()) == 0) {
            CACHE_LOGI("Removed old cache file: %s", file.path.c_str());
            currentSize -= file.size;
        } else {
            CACHE_LOGE("Failed to remove cache file: %s", file.path.c_str());
        }
    }

    CACHE_LOGI("Cache cleanup completed, new size: %ld MB", currentSize / (1024 * 1024));
}

long VideoCacheManager::getCacheSize() {
    long totalSize = 0;
    DIR* dir = opendir(m_cacheDir.c_str());
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (entry->d_name[0] == '.') continue;

            std::string filepath = m_cacheDir + "/" + entry->d_name;
            struct stat st;
            if (stat(filepath.c_str(), &st) == 0) {
                totalSize += st.st_size;
            }
        }
        closedir(dir);
    }
    return totalSize;
}

void VideoCacheManager::preloadVideo(const std::string& url) {
    CACHE_LOGI("Starting preload for: %s", url.c_str());
    downloadVideo(url); // Same logic, just called preemptively
}

std::string VideoCacheManager::generateCacheFilename(const std::string& url) {
    // Create a hash of the URL for the filename
    // Simple hash for now - in production, use proper hash function
    size_t hash = std::hash<std::string>{}(url);

    std::stringstream ss;
    ss << "video_" << std::hex << hash << ".mp4";
    return ss.str();
}

bool VideoCacheManager::isFileComplete(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) != 0) {
        return false; // File doesn't exist
    }

    // For now, assume file is complete if it exists and is > 0 bytes
    // In a more robust implementation, we could check for MP4 moov atom
    // or store completion markers
    return st.st_size > 0;
}

// Helper method to copy downloaded file to cache directory
bool VideoCacheManager::copyFileToCache(const std::string& sourcePath, const std::string& destPath) {
    CACHE_LOGI("Copying file from %s to %s", sourcePath.c_str(), destPath.c_str());

    std::ifstream sourceFile(sourcePath, std::ios::binary);
    if (!sourceFile.is_open()) {
        CACHE_LOGE("Failed to open source file: %s", sourcePath.c_str());
        return false;
    }

    std::ofstream destFile(destPath, std::ios::binary);
    if (!destFile.is_open()) {
        CACHE_LOGE("Failed to open destination file: %s", destPath.c_str());
        sourceFile.close();
        return false;
    }

    // Copy file contents
    destFile << sourceFile.rdbuf();

    sourceFile.close();
    destFile.close();

    if (!destFile.good()) {
        CACHE_LOGE("Error occurred during file copy");
        remove(destPath.c_str()); // Clean up partial file
        return false;
    }

    CACHE_LOGI("File copy completed successfully");
    return true;
}

// JNI callback implementations
void VideoCacheManager::onDownloadProgress(JNIEnv* env, jclass clazz, jstring url, jlong downloaded, jlong total) {
    // TODO: Call back to C++ instance
    CACHE_LOGI("Download progress: %lld/%lld", (long long)downloaded, (long long)total);
}

void VideoCacheManager::onDownloadComplete(JNIEnv* env, jclass clazz, jstring url, jstring localPath) {
    // TODO: Call back to C++ instance
    CACHE_LOGI("Download complete");
}

void VideoCacheManager::onDownloadError(JNIEnv* env, jclass clazz, jstring url, jstring error) {
    // TODO: Call back to C++ instance
    CACHE_LOGI("Download error");
}