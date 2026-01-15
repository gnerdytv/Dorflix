package com.dorflix.app.video

/**
 * Listener interface for video player events
 */
interface VideoPlayerListener {
    
    /**
     * Called when video is prepared and ready to play
     * @param duration Video duration in milliseconds
     */
    fun onVideoPrepared(duration: Int)
    
    /**
     * Called when video starts playing
     */
    fun onVideoStarted()
    
    /**
     * Called when video is paused
     */
    fun onVideoPaused()
    
    /**
     * Called when video is stopped
     */
    fun onVideoStopped()
    
    /**
     * Called when video playback is completed
     */
    fun onVideoCompleted()
    
    /**
     * Called when an error occurs during playback
     * @param errorCode Error code
     * @param errorMessage Error message
     */
    fun onVideoError(errorCode: Int, errorMessage: String)
    
    /**
     * Called when video progress changes
     * @param position Current position in milliseconds
     */
    fun onVideoProgressChanged(position: Int)
    
    /**
     * Called when video buffering starts
     */
    fun onVideoBufferingStarted()
    
    /**
     * Called when video buffering ends
     */
    fun onVideoBufferingEnded()
    
    /**
     * Called when seek operation is completed
     */
    fun onVideoSeekComplete()
    
    /**
     * Called when video size changes
     * @param width Video width in pixels
     * @param height Video height in pixels
     */
    fun onVideoSizeChanged(width: Int, height: Int)
}
