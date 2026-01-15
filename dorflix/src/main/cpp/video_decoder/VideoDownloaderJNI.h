#pragma once

#include <string>

/**
 * Simple interface for video downloading
 * Implemented by the JNI layer, called from VideoDecoder
 */
class VideoDownloaderInterface {
public:
    virtual ~VideoDownloaderInterface() = default;
    virtual std::string downloadVideo(const std::string& url) = 0;
};

// Global downloader instance - set by JNI layer
extern VideoDownloaderInterface* g_videoDownloader;

/**
 * Simple function to download video - delegates to global downloader
 */
inline std::string downloadVideoViaJNI(const std::string& url) {
    if (g_videoDownloader) {
        return g_videoDownloader->downloadVideo(url);
    }
    return "";
}

