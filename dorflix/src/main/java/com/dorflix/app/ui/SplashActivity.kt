package com.dorflix.app.ui

import android.content.Intent
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.os.SystemClock
import android.util.Log
import androidx.appcompat.app.AppCompatActivity
import com.dorflix.app.DorflixApplication
import com.dorflix.app.MainActivity
import com.dorflix.app.R

/**
 * Enhanced Splash screen activity with comprehensive monitoring and error recovery
 * Features timeout detection, performance metrics, and robust error handling
 */
class SplashActivity : AppCompatActivity() {

    companion object {
        private const val TAG = "SplashActivity"
        private const val SPLASH_DELAY: Long = 2000 // 2 seconds
        private const val MAX_ALLOWED_DELAY: Long = 5000 // 5 seconds max before timeout
        private const val HEALTH_CHECK_INTERVAL: Long = 500 // Check every 500ms
    }

    private var handler: Handler? = null
    private var transitionHandler: Handler? = null
    private var isTransitioning = false
    private var activityStartTime: Long = 0
    private var healthCheckRunnable: Runnable? = null
    private var transitionTimeoutRunnable: Runnable? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        activityStartTime = SystemClock.uptimeMillis()

        logSplashEvent("ACTIVITY_CREATE", "SplashActivity onCreate called")
        logPerformanceMetric("ACTIVITY_START_TIME", activityStartTime.toString())

        try {
            // Set content view with timing
            val contentViewStart = SystemClock.uptimeMillis()
            logSplashEvent("SET_CONTENT_VIEW", "Setting content view to activity_splash.xml")
            setContentView(R.layout.activity_splash)
            val contentViewEnd = SystemClock.uptimeMillis()
            logSplashEvent("CONTENT_VIEW_SET", "Content view set successfully in ${contentViewEnd - contentViewStart}ms")

            // Initialize handlers
            logSplashEvent("INIT_HANDLERS", "Initializing main and transition handlers")
            handler = Handler(Looper.getMainLooper())
            transitionHandler = Handler(Looper.getMainLooper())

            // Start health monitoring
            startHealthMonitoring()

            // Setup splash transition with timeout protection
            logSplashEvent("SETUP_TRANSITION", "Setting up splash to main activity transition")
            setupSplashTransitionWithTimeout()

            logSplashEvent("ON_CREATE_COMPLETE", "SplashActivity onCreate completed successfully")

        } catch (e: Exception) {
            logSplashEvent("ON_CREATE_ERROR", "Error in onCreate: ${e.message}")
            Log.e(TAG, "SplashActivity onCreate failed", e)
            attemptEmergencyRecovery()
        }
    }

    /**
     * Sets up splash transition with timeout protection
     */
    private fun setupSplashTransitionWithTimeout() {
        try {
            // Create timeout runnable first
            transitionTimeoutRunnable = Runnable {
                logSplashEvent("TRANSITION_TIMEOUT", "Transition timeout reached (${MAX_ALLOWED_DELAY}ms)")
                logPerformanceMetric("TRANSITION_TIMEOUT", "Transition did not complete within ${MAX_ALLOWED_DELAY}ms")

                if (!isTransitioning) {
                    logSplashEvent("FORCE_TRANSITION", "Forcing immediate transition due to timeout")
                    performTransitionToMain()
                }
            }

            // Schedule timeout
            transitionTimeoutRunnable?.let { transitionHandler?.postDelayed(it, MAX_ALLOWED_DELAY) }

            // Create main transition runnable
            val transitionRunnable = Runnable {
                val transitionStartTime = SystemClock.uptimeMillis()
                logSplashEvent("TRANSITION_START", "Starting transition to MainActivity")
                logPerformanceMetric("TRANSITION_START_TIME", transitionStartTime.toString())

                // Cancel timeout since we're transitioning
                transitionTimeoutRunnable?.let { transitionHandler?.removeCallbacks(it) }

                performTransitionToMain()
                logPerformanceMetric("TRANSITION_EXECUTION_TIME", "${SystemClock.uptimeMillis() - transitionStartTime}ms")
            }

            // Schedule main transition
            handler?.postDelayed(transitionRunnable, SPLASH_DELAY)

            logSplashEvent("TRANSITION_SETUP_COMPLETE", "Splash transition setup completed with timeout protection")

        } catch (e: Exception) {
            logSplashEvent("TRANSITION_SETUP_ERROR", "Error setting up transition: ${e.message}")
            Log.e(TAG, "Error setting up splash transition", e)
            attemptEmergencyRecovery()
        }
    }

    /**
     * Starts health monitoring to detect hangs
     */
    private fun startHealthMonitoring() {
        healthCheckRunnable = Runnable {
            val currentTime = SystemClock.uptimeMillis()
            val elapsedTime = currentTime - activityStartTime

            if (elapsedTime > MAX_ALLOWED_DELAY && !isTransitioning) {
                logSplashEvent("HEALTH_CHECK_FAILED", "Activity hung detected after ${elapsedTime}ms")
                logPerformanceMetric("ACTIVITY_HANG_DETECTED", "${elapsedTime}ms")

                // Force transition if we're stuck
                if (!isTransitioning) {
                    logSplashEvent("HEALTH_RECOVERY", "Attempting health recovery transition")
                    performTransitionToMain()
                }
            } else {
                logSplashEvent("HEALTH_CHECK_OK", "Health check OK at ${elapsedTime}ms")
                // Schedule next health check if we're still on splash screen
                if (!isTransitioning) {
                    healthCheckRunnable?.let { handler?.postDelayed(it, HEALTH_CHECK_INTERVAL) }
                }
            }
        }

        // Start health checks
        healthCheckRunnable?.let { handler?.postDelayed(it, HEALTH_CHECK_INTERVAL) }
    }

    /**
     * Performs the transition to MainActivity with enhanced error handling
     */
    private fun performTransitionToMain() {
        if (isTransitioning) {
            logSplashEvent("TRANSITION_ALREADY_STARTED", "Transition already in progress, skipping")
            return
        }

        isTransitioning = true
        logSplashEvent("TRANSITION_PERFORM", "Performing transition to MainActivity")

        try {
            // Validate we're on main thread
            if (Looper.myLooper() != Looper.getMainLooper()) {
                logSplashEvent("THREAD_ERROR", "Not on main thread, posting to main looper")
                Handler(Looper.getMainLooper()).post { performTransitionToMain() }
                return
            }

            // Create intent for MainActivity
            logSplashEvent("INTENT_CREATE", "Creating intent for MainActivity")
            val intent = Intent(this, MainActivity::class.java).apply {
                addFlags(Intent.FLAG_ACTIVITY_NEW_TASK or Intent.FLAG_ACTIVITY_CLEAR_TASK)
            }

            // Start MainActivity with timing
            val activityStartTime = SystemClock.uptimeMillis()
            logSplashEvent("ACTIVITY_START", "Starting MainActivity")
            startActivity(intent)
            logPerformanceMetric("MAIN_ACTIVITY_START_TIME", "${SystemClock.uptimeMillis() - activityStartTime}ms")

            // Finish splash activity
            val finishStartTime = SystemClock.uptimeMillis()
            logSplashEvent("ACTIVITY_FINISH", "Finishing SplashActivity")
            finish()
            logPerformanceMetric("SPLASH_FINISH_TIME", "${SystemClock.uptimeMillis() - finishStartTime}ms")

            // Calculate total splash time
            val totalTime = SystemClock.uptimeMillis() - this.activityStartTime
            logSplashEvent("TRANSITION_SUCCESS", "Transition to MainActivity completed successfully in ${totalTime}ms")
            logPerformanceMetric("TOTAL_SPLASH_TIME", "${totalTime}ms")

        } catch (e: Exception) {
            logSplashEvent("TRANSITION_ERROR", "Error during transition: ${e.message}")
            Log.e(TAG, "Error during splash transition", e)
            attemptEmergencyRecovery()
        }
    }

    /**
     * Attempts emergency recovery when all else fails
     */
    private fun attemptEmergencyRecovery() {
        logSplashEvent("EMERGENCY_RECOVERY", "Starting emergency recovery procedure")

        try {
            // Try to create and start MainActivity directly
            logSplashEvent("EMERGENCY_INTENT", "Creating emergency intent")
            val emergencyIntent = Intent(this, MainActivity::class.java).apply {
                addFlags(Intent.FLAG_ACTIVITY_NEW_TASK or Intent.FLAG_ACTIVITY_CLEAR_TASK)
            }

            logSplashEvent("EMERGENCY_START", "Starting emergency MainActivity")
            startActivity(emergencyIntent)

            // Try to finish this activity
            logSplashEvent("EMERGENCY_FINISH", "Finishing SplashActivity in emergency mode")
            finish()

            logSplashEvent("EMERGENCY_SUCCESS", "Emergency recovery completed")

        } catch (e: Exception) {
            logSplashEvent("EMERGENCY_FAILED", "Emergency recovery failed: ${e.message}")
            Log.e(TAG, "Emergency recovery failed", e)

            // Last resort: try to exit gracefully
            try {
                logSplashEvent("FORCE_EXIT", "Attempting to exit application")
                android.os.Process.killProcess(android.os.Process.myPid())
                System.exit(1)
            } catch (finalError: Exception) {
                logSplashEvent("COMPLETE_FAILURE", "All recovery attempts failed: ${finalError.message}")
                Log.e(TAG, "Complete failure - unable to recover", finalError)
            }
        }
    }

    /**
     * Logs splash screen events with timestamp
     */
    private fun logSplashEvent(eventType: String, details: String) {
        val timestamp = android.text.format.DateFormat.format("HH:mm:ss.SSS", System.currentTimeMillis())
        val elapsedTime = SystemClock.uptimeMillis() - activityStartTime
        val logMessage = "[$timestamp] [${elapsedTime}ms] [$eventType] $details"

        // Log to Android system log (this will appear in logcat)
        Log.d(TAG, logMessage)

        // Also log to application crash monitoring system
        try {
            DorflixApplication.instance?.let { app ->
                app.javaClass.getDeclaredMethod("logCrashEvent", String::class.java, String::class.java)
                    .invoke(app, "SPLASH_$eventType", "$logMessage - $details")
            }
        } catch (e: Exception) {
            Log.w(TAG, "Failed to log to crash monitoring system", e)
        }
    }

    /**
     * Logs performance metrics separately for easier analysis
     */
    private fun logPerformanceMetric(metricName: String, value: String) {
        Log.i("${TAG}_PERF", "[$metricName] $value")

        try {
            DorflixApplication.instance?.let { app ->
                app.javaClass.getDeclaredMethod("logCrashEvent", String::class.java, String::class.java)
                    .invoke(app, "SPLASH_PERF_$metricName", value)
            }
        } catch (e: Exception) {
            Log.w(TAG, "Failed to log performance metric", e)
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        logSplashEvent("ACTIVITY_DESTROY", "SplashActivity onDestroy called")

        // Clean up all handlers and runnables
        try {
            handler?.removeCallbacksAndMessages(null)
            transitionHandler?.removeCallbacksAndMessages(null)
            healthCheckRunnable?.let { handler?.removeCallbacks(it) }
            transitionTimeoutRunnable?.let { transitionHandler?.removeCallbacks(it) }

            handler = null
            transitionHandler = null
            healthCheckRunnable = null
            transitionTimeoutRunnable = null

            logSplashEvent("HANDLER_CLEANUP", "All handlers cleaned up successfully")
        } catch (e: Exception) {
            logSplashEvent("HANDLER_CLEANUP_ERROR", "Error cleaning up handlers: ${e.message}")
            Log.w(TAG, "Error cleaning up handlers", e)
        }
    }

    override fun onPause() {
        super.onPause()
        logSplashEvent("ACTIVITY_PAUSE", "SplashActivity onPause called")
    }

    override fun onResume() {
        super.onResume()
        logSplashEvent("ACTIVITY_RESUME", "SplashActivity onResume called")
    }

    override fun onStart() {
        super.onStart()
        logSplashEvent("ACTIVITY_START", "SplashActivity onStart called")
    }

    override fun onStop() {
        super.onStop()
        logSplashEvent("ACTIVITY_STOP", "SplashActivity onStop called")
    }
}
