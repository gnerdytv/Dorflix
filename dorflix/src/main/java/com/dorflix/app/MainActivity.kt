package com.dorflix.app

import android.os.Bundle
import android.util.Log
import android.view.View
import androidx.appcompat.app.AppCompatActivity
import com.dorflix.app.ui.FeedFragment
import com.dorflix.app.ui.VideoPlayerFragment

/**
 * TikTok-style full-screen MainActivity for Dorflix
 * Shows only the video feed, no navigation
 */
class MainActivity : AppCompatActivity() {

    companion object {
        private const val TAG = "MainActivity"
    }

    private lateinit var binding: com.dorflix.app.databinding.ActivityMainBinding
    private var isInitialized = false
    
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // Enable TikTok-style immersive mode using modern WindowInsetsController
        val windowInsetsController = androidx.core.view.WindowCompat.getInsetsController(window, window.decorView)

        // Hide system bars with sticky immersive behavior
        windowInsetsController.apply {
            hide(androidx.core.view.WindowInsetsCompat.Type.systemBars())
            systemBarsBehavior = androidx.core.view.WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
        }

        // Make status bar translucent so video shows behind it
        window.addFlags(android.view.WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS)
        @Suppress("DEPRECATION")
        window.statusBarColor = android.graphics.Color.TRANSPARENT

        // Log main activity creation
        logMainEvent("ACTIVITY_CREATE", "TikTok-style full-screen MainActivity onCreate called")

        try {
            // Set up theme before setContentView
            logMainEvent("SET_THEME", "Setting theme to Theme_DorflixNative")
            setTheme(com.dorflix.app.R.style.Theme_DorflixNative)

            // Inflate layout
            logMainEvent("INFLATE_LAYOUT", "Inflating ActivityMainBinding")
            binding = com.dorflix.app.databinding.ActivityMainBinding.inflate(layoutInflater)

            // Set content view
            logMainEvent("SET_CONTENT_VIEW", "Setting content view")
            setContentView(binding.root)

            // Show only the feed fragment (TikTok-style)
            logMainEvent("SETUP_FEED", "Setting up full-screen feed fragment")
            setupFeedFragment()

            isInitialized = true
            logMainEvent("ON_CREATE_COMPLETE", "TikTok-style MainActivity onCreate completed successfully")

        } catch (e: Exception) {
            logMainEvent("ON_CREATE_ERROR", "Error in onCreate: ${e.message}")
            Log.e(TAG, "MainActivity onCreate failed", e)

            // Try to recover by finishing activity
            try {
                logMainEvent("RECOVERY_FINISH", "Attempting to finish activity due to error")
                finish()
            } catch (recoveryError: Exception) {
                logMainEvent("RECOVERY_FINISH_ERROR", "Failed to finish activity: ${recoveryError.message}")
                Log.e(TAG, "Failed to finish activity", recoveryError)
            }
        }
    }
    
    /**
     * Sets up the full-screen feed fragment (TikTok-style)
     */
    private fun setupFeedFragment() {
        try {
            logMainEvent("SETUP_FEED_FRAGMENT", "Setting up full-screen feed fragment")

            supportFragmentManager
                .beginTransaction()
                .replace(com.dorflix.app.R.id.fragment_container, FeedFragment())
                .commit()

            logMainEvent("FEED_FRAGMENT_COMPLETE", "Full-screen feed fragment setup completed")
        } catch (e: Exception) {
            logMainEvent("FEED_FRAGMENT_ERROR", "Error setting up feed fragment: ${e.message}")
            Log.e(TAG, "Error setting up feed fragment", e)
        }
    }

    /**
     * Navigate to video player with error handling (for compatibility with other fragments)
     */
    fun openVideoPlayer(videoId: String) {
        try {
            logMainEvent("OPEN_VIDEO_PLAYER", "Opening video player for video ID: $videoId")

            val fragment = VideoPlayerFragment.newInstance(videoId)
            supportFragmentManager
                .beginTransaction()
                .replace(com.dorflix.app.R.id.fragment_container, fragment)
                .addToBackStack("video_player")
                .commit()

            logMainEvent("VIDEO_PLAYER_OPENED", "Video player opened successfully for video ID: $videoId")

        } catch (e: Exception) {
            logMainEvent("OPEN_VIDEO_PLAYER_ERROR", "Error opening video player: ${e.message}")
            Log.e(TAG, "Error opening video player", e)
        }
    }
    
    init {
        // Set up back pressed dispatcher for modern Android
        onBackPressedDispatcher.addCallback(this, object : androidx.activity.OnBackPressedCallback(true) {
            override fun handleOnBackPressed() {
                try {
                    logMainEvent("BACK_PRESSED", "Back button pressed, back stack count: ${supportFragmentManager.backStackEntryCount}")

                    if (supportFragmentManager.backStackEntryCount > 0) {
                        logMainEvent("POP_BACK_STACK", "Popping back stack")
                        supportFragmentManager.popBackStack()
                    } else {
                        logMainEvent("FINISH_ACTIVITY", "Finishing activity")
                        finish()
                    }

                } catch (e: Exception) {
                    logMainEvent("BACK_PRESSED_ERROR", "Error handling back press: ${e.message}")
                    Log.e(TAG, "Error handling back press", e)

                    // Try to finish as fallback
                    try {
                        finish()
                    } catch (finishError: Exception) {
                        logMainEvent("FINISH_ERROR", "Failed to finish activity: ${finishError.message}")
                        Log.e(TAG, "Failed to finish activity", finishError)
                    }
                }
            }
        })
    }
    
    override fun onDestroy() {
        super.onDestroy()
        logMainEvent("ACTIVITY_DESTROY", "MainActivity onDestroy called")
        
        // Clean up resources
        try {
            if (isInitialized) {
                logMainEvent("CLEANUP_RESOURCES", "Cleaning up MainActivity resources")
                // Any additional cleanup can be added here
            }
        } catch (e: Exception) {
            logMainEvent("CLEANUP_ERROR", "Error during cleanup: ${e.message}")
            Log.w(TAG, "Error during MainActivity cleanup", e)
        }
    }
    
    override fun onPause() {
        super.onPause()
        logMainEvent("ACTIVITY_PAUSE", "MainActivity onPause called")
    }
    
    override fun onResume() {
        super.onResume()
        logMainEvent("ACTIVITY_RESUME", "MainActivity onResume called")
    }
    
    override fun onStart() {
        super.onStart()
        logMainEvent("ACTIVITY_START", "MainActivity onStart called")
    }
    
    override fun onStop() {
        super.onStop()
        logMainEvent("ACTIVITY_STOP", "MainActivity onStop called")
    }
    
    /**
     * Logs main activity events with timestamp
     */
    private fun logMainEvent(eventType: String, details: String) {
        val timestamp = android.text.format.DateFormat.format("HH:mm:ss.SSS", System.currentTimeMillis())
        val logMessage = "[$timestamp] [$eventType] $details"
        
        // Log to Android system log
        Log.d(TAG, logMessage)
        
        // Also log to application crash monitoring system
        try {
            DorflixApplication.instance?.let { app ->
                app.javaClass.getDeclaredMethod("logCrashEvent", String::class.java, String::class.java)
                    .invoke(app, "MAIN_$eventType", details)
            }
        } catch (e: Exception) {
            Log.w(TAG, "Failed to log to crash monitoring system", e)
        }
    }
}