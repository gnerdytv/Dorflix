package com.dorflix.app.video

/**
 * Preloader for video playback using C++ backend
 * Provides video preloading functionality for smooth playback with minimal buffering
 */
class VideoPreloader {
    
    // Load the native library
    companion object {
        init {
            try {
                System.loadLibrary("dorflix-native")
            } catch (e: UnsatisfiedLinkError) {
                // Handle native library loading failure
                throw RuntimeException("Failed to load native library", e)
            }
        }
    }
    
    // Native methods
    private external fun nativeCreatePreloader(): Long
    private external fun nativeDestroyPreloader(preloaderHandle: Long)
    private external fun nativePreloadVideo(preloaderHandle: Long, videoPath: String)
    
    private var preloaderHandle: Long = 0
    
    init {
        preloaderHandle = nativeCreatePreloader()
    }
    
    /**
     * Preload a video from the given path/URL
     * @param videoPath Path or URL to the video file
     */
    fun preloadVideo(videoPath: String) {
        nativePreloadVideo(preloaderHandle, videoPath)
    }
    
    /**
     * Release the preloader resources
     */
    fun release() {
        nativeDestroyPreloader(preloaderHandle)
        preloaderHandle = 0
    }
    
    /**
     * Clean up when the object is garbage collected
     */
    protected fun finalize() {
        if (preloaderHandle != 0L) {
            release()
        }
    }
}
