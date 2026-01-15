package com.dorflix.app.video

import android.graphics.SurfaceTexture
import android.view.Surface

/**
 * Controller for video playback using C++ backend
 */
class VideoPlayerController(private val listener: VideoPlayerListener) {

    // Store the listener as a public field accessible to JNI
    // JNI requires public fields for reflection access
    @JvmField
    val callback: VideoPlayerListener = listener

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
    private external fun nativeCreatePlayer(): Long
    private external fun nativeSetupCallbacks(playerHandle: Long)
    private external fun nativeLoadVideo(playerHandle: Long, videoPath: String): Boolean
    private external fun nativePlay(playerHandle: Long)
    private external fun nativePause(playerHandle: Long)
    private external fun nativeStop(playerHandle: Long)
    private external fun nativeSeekTo(playerHandle: Long, positionMs: Int)
    private external fun nativeSetSurface(playerHandle: Long, surface: Surface?): Boolean
    private external fun nativeGetCurrentPosition(playerHandle: Long): Int
    private external fun nativeGetDuration(playerHandle: Long): Int
    private external fun nativeIsPlaying(playerHandle: Long): Boolean
    private external fun nativeSetVolume(playerHandle: Long, volume: Float)
    private external fun nativeSetMute(playerHandle: Long, mute: Boolean)
    private external fun nativeDestroyPlayer(playerHandle: Long)

    private var playerHandle: Long = 0
    
    init {
        playerHandle = nativeCreatePlayer()
        // Set up C++ to Java callbacks immediately after creation
        nativeSetupCallbacks(playerHandle)
    }
    
    fun loadVideo(videoPath: String): Boolean {
        if (playerHandle == 0L) {
            android.util.Log.e("VideoPlayerController", "CRASH PREVENTION: loadVideo() called on released controller!")
            return false
        }
        return nativeLoadVideo(playerHandle, videoPath)
    }

    fun play() {
        if (playerHandle == 0L) {
            android.util.Log.e("VideoPlayerController", "CRASH PREVENTION: play() called on released controller!")
            return
        }
        nativePlay(playerHandle)
    }

    fun pause() {
        if (playerHandle == 0L) {
            android.util.Log.e("VideoPlayerController", "CRASH PREVENTION: pause() called on released controller!")
            return
        }
        nativePause(playerHandle)
    }

    fun resume() {
        if (playerHandle == 0L) {
            android.util.Log.e("VideoPlayerController", "CRASH PREVENTION: resume() called on released controller!")
            return
        }
        nativePlay(playerHandle)
    }

    fun stop() {
        if (playerHandle == 0L) {
            android.util.Log.e("VideoPlayerController", "CRASH PREVENTION: stop() called on released controller!")
            return
        }
        nativeStop(playerHandle)
    }

    fun seekTo(positionMs: Int) {
        if (playerHandle == 0L) {
            android.util.Log.e("VideoPlayerController", "CRASH PREVENTION: seekTo() called on released controller!")
            return
        }
        nativeSeekTo(playerHandle, positionMs)
    }

    fun setSurface(surface: Surface?): Boolean {
        if (playerHandle == 0L) {
            android.util.Log.e("VideoPlayerController", "CRASH PREVENTION: setSurface() called on released controller!")
            return false
        }
        return nativeSetSurface(playerHandle, surface)
    }

    fun getCurrentPosition(): Int {
        if (playerHandle == 0L) {
            android.util.Log.e("VideoPlayerController", "CRASH PREVENTION: getCurrentPosition() called on released controller!")
            return 0
        }
        return nativeGetCurrentPosition(playerHandle)
    }

    fun getDuration(): Int {
        if (playerHandle == 0L) {
            android.util.Log.e("VideoPlayerController", "CRASH PREVENTION: getDuration() called on released controller!")
            return 0
        }
        return nativeGetDuration(playerHandle)
    }

    fun isPlaying(): Boolean {
        if (playerHandle == 0L) {
            android.util.Log.e("VideoPlayerController", "CRASH PREVENTION: isPlaying() called on released controller!")
            return false
        }
        return nativeIsPlaying(playerHandle)
    }
    
    fun release() {
        try {
            if (playerHandle != 0L) {
                nativeDestroyPlayer(playerHandle)
                playerHandle = 0
            }
        } catch (e: UnsatisfiedLinkError) {
            // Native library not loaded, but we can still clean up
            playerHandle = 0
        } catch (e: Exception) {
            // Log the error but don't crash
            playerHandle = 0
        }
    }
    
    /**
     * Check if the native player is properly initialized
     */
    fun isInitialized(): Boolean {
        return playerHandle != 0L
    }
    
    /**
     * Safely perform an operation that requires the native player
     */
    private fun <T> withPlayer(operation: (Long) -> T): T? {
        return try {
            if (playerHandle != 0L) {
                operation(playerHandle)
            } else {
                null
            }
        } catch (e: UnsatisfiedLinkError) {
            null
        } catch (e: Exception) {
            null
        }
    }
    
    // Native callback methods (called from C++)
    @Suppress("unused")
    private fun onVideoPrepared(duration: Int) {
        listener.onVideoPrepared(duration)
    }
    
    @Suppress("unused")
    private fun onVideoStarted() {
        listener.onVideoStarted()
    }
    
    @Suppress("unused")
    private fun onVideoPaused() {
        listener.onVideoPaused()
    }
    
    @Suppress("unused")
    private fun onVideoStopped() {
        try {
            listener.onVideoStopped()
        } catch (e: Exception) {
            android.util.Log.e("VideoPlayerController", "CRASH in onVideoStopped callback", e)
            throw e
        }
    }
    
    @Suppress("unused")
    private fun onVideoCompleted() {
        listener.onVideoCompleted()
    }
    
    @Suppress("unused")
    private fun onVideoError(errorCode: Int, errorMessage: String) {
        listener.onVideoError(errorCode, errorMessage)
    }
    
    @Suppress("unused")
    private fun onVideoProgressChanged(position: Int) {
        listener.onVideoProgressChanged(position)
    }
    
    @Suppress("unused")
    private fun onVideoBufferingStarted() {
        listener.onVideoBufferingStarted()
    }
    
    @Suppress("unused")
    private fun onVideoBufferingEnded() {
        listener.onVideoBufferingEnded()
    }
    
    @Suppress("unused")
    private fun onVideoSeekComplete() {
        listener.onVideoSeekComplete()
    }
    
    @Suppress("unused")
    private fun onVideoSizeChanged(width: Int, height: Int) {
        listener.onVideoSizeChanged(width, height)
    }
}