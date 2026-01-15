#pragma once

#include <android/native_window.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>
}

class VideoRenderer {
public:
    VideoRenderer() = default;
    virtual ~VideoRenderer() = default;

    /**
     * Initialize the renderer with surface and dimensions
     */
    virtual bool initialize(ANativeWindow* window, int width, int height) = 0;

    /**
     * Render a video frame
     */
    virtual bool renderFrame(AVFrame* frame) = 0;

    /**
     * Set viewport dimensions
     */
    virtual void setViewport(int width, int height) = 0;

    /**
     * Check if renderer is ready
     */
    virtual bool isInitialized() const = 0;

    /**
     * Clean up resources
     */
    virtual void cleanup() = 0;

    // Prevent copying
    VideoRenderer(const VideoRenderer&) = delete;
    VideoRenderer& operator=(const VideoRenderer&) = delete;
};