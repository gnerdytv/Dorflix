package com.dorflix.app.video;

/**
 * Listener interface for video player events
 */
@kotlin.Metadata(mv = {2, 3, 0}, k = 1, xi = 48, d1 = {"\u0000 \n\u0002\u0018\u0002\n\u0002\u0010\u0000\n\u0000\n\u0002\u0010\u0002\n\u0000\n\u0002\u0010\b\n\u0002\b\u0007\n\u0002\u0010\u000e\n\u0002\b\t\bf\u0018\u00002\u00020\u0001J\u0010\u0010\u0002\u001a\u00020\u00032\u0006\u0010\u0004\u001a\u00020\u0005H&J\b\u0010\u0006\u001a\u00020\u0003H&J\b\u0010\u0007\u001a\u00020\u0003H&J\b\u0010\b\u001a\u00020\u0003H&J\b\u0010\t\u001a\u00020\u0003H&J\u0018\u0010\n\u001a\u00020\u00032\u0006\u0010\u000b\u001a\u00020\u00052\u0006\u0010\f\u001a\u00020\rH&J\u0010\u0010\u000e\u001a\u00020\u00032\u0006\u0010\u000f\u001a\u00020\u0005H&J\b\u0010\u0010\u001a\u00020\u0003H&J\b\u0010\u0011\u001a\u00020\u0003H&J\b\u0010\u0012\u001a\u00020\u0003H&J\u0018\u0010\u0013\u001a\u00020\u00032\u0006\u0010\u0014\u001a\u00020\u00052\u0006\u0010\u0015\u001a\u00020\u0005H&\u00a8\u0006\u0016\u00c0\u0006\u0003"}, d2 = {"Lcom/dorflix/app/video/VideoPlayerListener;", "", "onVideoPrepared", "", "duration", "", "onVideoStarted", "onVideoPaused", "onVideoStopped", "onVideoCompleted", "onVideoError", "errorCode", "errorMessage", "", "onVideoProgressChanged", "position", "onVideoBufferingStarted", "onVideoBufferingEnded", "onVideoSeekComplete", "onVideoSizeChanged", "width", "height", "DorflixNative_debug"})
public abstract interface VideoPlayerListener {
    
    /**
     * Called when video is prepared and ready to play
     * @param duration Video duration in milliseconds
     */
    public abstract void onVideoPrepared(int duration);
    
    /**
     * Called when video starts playing
     */
    public abstract void onVideoStarted();
    
    /**
     * Called when video is paused
     */
    public abstract void onVideoPaused();
    
    /**
     * Called when video is stopped
     */
    public abstract void onVideoStopped();
    
    /**
     * Called when video playback is completed
     */
    public abstract void onVideoCompleted();
    
    /**
     * Called when an error occurs during playback
     * @param errorCode Error code
     * @param errorMessage Error message
     */
    public abstract void onVideoError(int errorCode, @org.jetbrains.annotations.NotNull()
    java.lang.String errorMessage);
    
    /**
     * Called when video progress changes
     * @param position Current position in milliseconds
     */
    public abstract void onVideoProgressChanged(int position);
    
    /**
     * Called when video buffering starts
     */
    public abstract void onVideoBufferingStarted();
    
    /**
     * Called when video buffering ends
     */
    public abstract void onVideoBufferingEnded();
    
    /**
     * Called when seek operation is completed
     */
    public abstract void onVideoSeekComplete();
    
    /**
     * Called when video size changes
     * @param width Video width in pixels
     * @param height Video height in pixels
     */
    public abstract void onVideoSizeChanged(int width, int height);
}