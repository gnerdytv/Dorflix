package com.dorflix.app.video

import android.graphics.SurfaceTexture
import android.view.Surface
import java.lang.ref.WeakReference

/**
 * Video player controller that interfaces with C++ native implementation
 */
class VideoPlayerController(private val listener: VideoPlayerListener) {
    
    companion object {
        // Load the native library
        init {
            System.loadLibrary("dorflix-native")
        }
    }
    
    // Native pointer to C++ VideoDecoder instance
    private var nativePlayerPtr: Long = 0
    
    // Weak reference to prevent memory leaks
    private val listenerRef = WeakReference(listener)
    
    init {
        // Create native player instance
        nativePlayerPtr = nativeCreatePlayer()
    }
    
    /**
     * Set the surface for video rendering
     * @param surface Android surface for rendering
     */
    fun setSurface(surface: Surface?) {
        if (nativePlayerPtr != 0L) {
            nativeSetSurface(nativePlayerPtr, surface)
        }
    }
    
    /**
     * Load a video file for playback
     * @param videoUrl URL or path to the video file
     */
    fun loadVideo(videoUrl: String) {
        if (nativePlayerPtr != 0L) {
            nativeLoadVideo(nativePlayerPtr, videoUrl)
        }
    }
    
    /**
     * Start video playback
     */
    fun play() {
        if (nativePlayerPtr != 0L) {
            nativePlay(nativePlayerPtr)
        }
    }
    
    /**
     * Pause video playback
     */
    fun pause() {
        if (nativePlayerPtr != 0L) {
            nativePause(nativePlayerPtr)
        }
    }
    
    /**
     * Stop video playback
     */
    fun stop() {
        if (nativePlayerPtr != 0L) {
            nativeStop(nativePlayerPtr)
        }
    }
    
    /**
     * Seek to a specific position in the video
     * @param position Position in milliseconds
     */
    fun seekTo(position: Int) {
        if (nativePlayerPtr != 0L) {
            nativeSeekTo(nativePlayerPtr, position)
        }
    }
    
    /**
     * Get the current playback position
     * @return Current position in milliseconds
     */
    fun getCurrentPosition(): Int {
        return if (nativePlayerPtr != 0L) {
            nativeGetCurrentPosition(nativePlayerPtr)
        } else 0
    }
    
    /**
     * Get the total duration of the video
     * @return Duration in milliseconds
     */
    fun getDuration(): Int {
        return if (nativePlayerPtr != 0L) {
            nativeGetDuration(nativePlayerPtr)
        } else 0
    }
    
    /**
     * Check if the video is currently playing
     * @return True if playing, false otherwise
     */
    fun isPlaying(): Boolean {
        return if (nativePlayerPtr != 0L) {
            nativeIsPlaying(nativePlayerPtr)
        } else false
    }
    
    /**
     * Set the volume level
     * @param volume Volume level (0.0 to 1.0)
     */
    fun setVolume(volume: Float) {
        if (nativePlayerPtr != 0L) {
            nativeSetVolume(nativePlayerPtr, volume.coerceIn(0f, 1f))
        }
    }
    
    /**
     * Set mute state
     * @param mute True to mute, false to unmute
     */
    fun setMute(mute: Boolean) {
        if (nativePlayerPtr != 0L) {
            nativeSetMute(nativePlayerPtr, mute)
        }
    }
    
    /**
     * Release resources and clean up
     */
    fun release() {
        if (nativePlayerPtr != 0L) {
            nativeDestroyPlayer(nativePlayerPtr)
            nativePlayerPtr = 0
        }
    }
    
    // Native method declarations
    private external fun nativeCreatePlayer(): Long
    private external fun nativeDestroyPlayer(playerPtr: Long)
    private external fun nativeSetSurface(playerPtr: Long, surface: Surface?)
    private external fun nativeLoadVideo(playerPtr: Long, videoPath: String)
    private external fun nativePlay(playerPtr: Long)
    private external fun nativePause(playerPtr: Long)
    private external fun nativeStop(playerPtr: Long)
    private external fun nativeSeekTo(playerPtr: Long, position: Int)
    private external fun nativeGetCurrentPosition(playerPtr: Long): Int
    private external fun nativeGetDuration(playerPtr: Long): Int
    private external fun nativeIsPlaying(playerPtr: Long): Boolean
    private external fun nativeSetVolume(playerPtr: Long, volume: Float)
    private external fun nativeSetMute(playerPtr: Long, mute: Boolean)
}
