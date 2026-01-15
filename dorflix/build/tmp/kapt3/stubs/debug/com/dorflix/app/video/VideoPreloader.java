package com.dorflix.app.video;

/**
 * Preloader for video playback using C++ backend
 * Provides video preloading functionality for smooth playback with minimal buffering
 */
@kotlin.Metadata(mv = {2, 3, 0}, k = 1, xi = 48, d1 = {"\u0000\"\n\u0002\u0018\u0002\n\u0002\u0010\u0000\n\u0002\b\u0003\n\u0002\u0010\t\n\u0000\n\u0002\u0010\u0002\n\u0002\b\u0003\n\u0002\u0010\u000e\n\u0002\b\u0005\u0018\u0000 \u000f2\u00020\u0001:\u0001\u000fB\u0007\u00a2\u0006\u0004\b\u0002\u0010\u0003J\t\u0010\u0004\u001a\u00020\u0005H\u0082 J\u0011\u0010\u0006\u001a\u00020\u00072\u0006\u0010\b\u001a\u00020\u0005H\u0082 J\u0019\u0010\t\u001a\u00020\u00072\u0006\u0010\b\u001a\u00020\u00052\u0006\u0010\n\u001a\u00020\u000bH\u0082 J\u000e\u0010\f\u001a\u00020\u00072\u0006\u0010\n\u001a\u00020\u000bJ\u0006\u0010\r\u001a\u00020\u0007J\b\u0010\u000e\u001a\u00020\u0007H\u0004R\u000e\u0010\b\u001a\u00020\u0005X\u0082\u000e\u00a2\u0006\u0002\n\u0000\u00a8\u0006\u0010"}, d2 = {"Lcom/dorflix/app/video/VideoPreloader;", "", "<init>", "()V", "nativeCreatePreloader", "", "nativeDestroyPreloader", "", "preloaderHandle", "nativePreloadVideo", "videoPath", "", "preloadVideo", "release", "finalize", "Companion", "DorflixNative_debug"})
public final class VideoPreloader {
    private long preloaderHandle = 0L;
    @org.jetbrains.annotations.NotNull()
    public static final com.dorflix.app.video.VideoPreloader.Companion Companion = null;
    
    public VideoPreloader() {
        super();
    }
    
    private final native long nativeCreatePreloader() {
        return 0L;
    }
    
    private final native void nativeDestroyPreloader(long preloaderHandle) {
    }
    
    private final native void nativePreloadVideo(long preloaderHandle, java.lang.String videoPath) {
    }
    
    /**
     * Preload a video from the given path/URL
     * @param videoPath Path or URL to the video file
     */
    public final void preloadVideo(@org.jetbrains.annotations.NotNull()
    java.lang.String videoPath) {
    }
    
    /**
     * Release the preloader resources
     */
    public final void release() {
    }
    
    /**
     * Clean up when the object is garbage collected
     */
    protected final void finalize() {
    }
    
    @kotlin.Metadata(mv = {2, 3, 0}, k = 1, xi = 48, d1 = {"\u0000\f\n\u0002\u0018\u0002\n\u0002\u0010\u0000\n\u0002\b\u0003\b\u0086\u0003\u0018\u00002\u00020\u0001B\t\b\u0002\u00a2\u0006\u0004\b\u0002\u0010\u0003\u00a8\u0006\u0004"}, d2 = {"Lcom/dorflix/app/video/VideoPreloader$Companion;", "", "<init>", "()V", "DorflixNative_debug"})
    public static final class Companion {
        
        private Companion() {
            super();
        }
    }
}