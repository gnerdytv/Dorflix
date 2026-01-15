package com.dorflix.app.video

/**
 * Video preloader for caching video data in advance
 */
class VideoPreloader {
    
    companion object {
        // Load the native library
        init {
            System.loadLibrary("dorflix-native")
        }
    }
    
    // Native pointer to C++ Preloader instance
    private var nativePreloaderPtr: Long = 0
    
    init {
        // Create native preloader instance
        nativePreloaderPtr = nativeCreatePreloader()
    }
    
    /**
     * Preload a video file for faster playback
     * @param videoUrl URL or path to the video file
     */
    fun preloadVideo(videoUrl: String) {
        if (nativePreloaderPtr != 0L) {
            nativePreloadVideo(nativePreloaderPtr, videoUrl)
        }
    }
    
    /**
     * Release resources and clean up
     */
    fun release() {
        if (nativePreloaderPtr != 0L) {
            nativeDestroyPreloader(nativePreloaderPtr)
            nativePreloaderPtr = 0
        }
    }
    
    // Native method declarations
    private external fun nativeCreatePreloader(): Long
    private external fun nativeDestroyPreloader(preloaderPtr: Long)
    private external fun nativePreloadVideo(preloaderPtr: Long, videoPath: String)
}
