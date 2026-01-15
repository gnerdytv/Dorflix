package com.dorflix.app

import android.app.Application
import android.content.Context
import android.util.Log
import com.dorflix.app.video.VideoDownloader
import java.io.File
import java.io.FileWriter
import java.io.IOException
import java.text.SimpleDateFormat
import java.util.*

/**
 * Application class for Dorflix app
 * Provides global application context, singleton instance access, and crash monitoring
 */
class DorflixApplication : Application() {
    
    companion object {
        private const val TAG = "DorflixApplication"
        private const val CRASH_LOG_FILE = "crash_log.txt"
        
        @Volatile
        private var INSTANCE: DorflixApplication? = null
        
        /**
         * Gets the singleton instance of the application
         * @return The application instance or null if not initialized
         */
        val instance: DorflixApplication?
            get() {
                if (INSTANCE == null) {
                    Log.w(TAG, "Application instance not yet initialized")
                }
                return INSTANCE
            }
    }
    
    override fun onCreate() {
        super.onCreate()
        INSTANCE = this
        Log.d(TAG, "DorflixApplication created")

        // Set up global exception handler for crash monitoring
        setupCrashHandler()

        // Initialize VideoDownloader for JNI calls
        VideoDownloader.initialize(this)

        // Log application start
        logCrashEvent("APPLICATION_START", "Dorflix application started successfully")
    }
    
    override fun onTerminate() {
        super.onTerminate()
        Log.d(TAG, "DorflixApplication terminated")
        INSTANCE = null
    }
    
    /**
     * Sets up global exception handler to catch unhandled crashes
     */
    private fun setupCrashHandler() {
        Thread.setDefaultUncaughtExceptionHandler { thread, exception ->
            handleCrash(thread, exception)
        }
        Log.d(TAG, "Global crash handler installed")
    }
    
    /**
     * Handles application crashes and logs detailed information
     */
    private fun handleCrash(thread: Thread, exception: Throwable) {
        try {
            // Log the crash details
            val crashDetails = buildCrashDetails(thread, exception)
            logCrashEvent("CRASH_DETECTED", crashDetails)
            
            // Log to Android system log
            Log.e(TAG, "Uncaught exception in thread: ${thread.name}", exception)
            
            // Write crash details to file for later analysis
            writeCrashToFile(crashDetails)
            
        } catch (e: Exception) {
            Log.e(TAG, "Failed to handle crash properly", e)
        }
        
        // Let the system handle the crash (default behavior)
        val defaultHandler = Thread.getDefaultUncaughtExceptionHandler()
        defaultHandler?.uncaughtException(thread, exception)
    }
    
    /**
     * Builds detailed crash information string
     */
    private fun buildCrashDetails(thread: Thread, exception: Throwable): String {
        val timestamp = SimpleDateFormat("yyyy-MM-dd HH:mm:ss.SSS", Locale.getDefault()).format(Date())
        val sb = StringBuilder()
        
        sb.appendLine("=== CRASH REPORT ===")
        sb.appendLine("Timestamp: $timestamp")
        sb.appendLine("Thread: ${thread.name} (ID: ${thread.id})")
        sb.appendLine("Priority: ${thread.priority}")
        sb.appendLine("State: ${thread.state}")
        sb.appendLine("Exception: ${exception.javaClass.name}")
        sb.appendLine("Message: ${exception.message}")
        sb.appendLine()
        sb.appendLine("=== STACK TRACE ===")
        
        // Add stack trace
        exception.stackTrace.forEach { element ->
            sb.appendLine("    at ${element.className}.${element.methodName}(${element.fileName}:${element.lineNumber})")
        }
        
        // Add cause if available
        var cause = exception.cause
        var depth = 0
        while (cause != null && depth < 5) {
            sb.appendLine()
            sb.appendLine("Caused by: ${cause.javaClass.name}: ${cause.message}")
            cause.stackTrace.forEach { element ->
                sb.appendLine("    at ${element.className}.${element.methodName}(${element.fileName}:${element.lineNumber})")
            }
            cause = cause.cause
            depth++
        }
        
        sb.appendLine()
        sb.appendLine("=== SYSTEM INFO ===")
        sb.appendLine("Android Version: ${android.os.Build.VERSION.RELEASE}")
        sb.appendLine("API Level: ${android.os.Build.VERSION.SDK_INT}")
        sb.appendLine("Device: ${android.os.Build.MANUFACTURER} ${android.os.Build.MODEL}")
        sb.appendLine("App Version: ${getAppVersion()}")
        
        return sb.toString()
    }
    
    /**
     * Logs crash events with timestamp
     */
    fun logCrashEvent(eventType: String, details: String) {
        val timestamp = SimpleDateFormat("yyyy-MM-dd HH:mm:ss", Locale.getDefault()).format(Date())
        val logEntry = "[$timestamp] [$eventType] $details"
        
        // Write to crash log file
        try {
            val logFile = File(filesDir, CRASH_LOG_FILE)
            FileWriter(logFile, true).use { writer ->
                writer.appendLine(logEntry)
                writer.appendLine("---")
                writer.flush()
            }
        } catch (e: IOException) {
            Log.e(TAG, "Failed to write crash log", e)
        }
    }
    
    /**
     * Writes crash details to a file for persistent storage
     */
    private fun writeCrashToFile(crashDetails: String) {
        try {
            val crashFile = File(filesDir, "last_crash.txt")
            FileWriter(crashFile).use { writer ->
                writer.write(crashDetails)
                writer.flush()
            }
            Log.d(TAG, "Crash details written to ${crashFile.absolutePath}")
        } catch (e: IOException) {
            Log.e(TAG, "Failed to write crash file", e)
        }
    }
    
    /**
     * Gets the application version name
     */
    private fun getAppVersion(): String {
        return try {
            val packageInfo = packageManager.getPackageInfo(packageName, 0)
            packageInfo.versionName ?: "unknown"
        } catch (e: Exception) {
            "unknown"
        }
    }
    
    /**
     * Gets the crash log file for reading
     */
    fun getCrashLogFile(): File? {
        return try {
            File(filesDir, CRASH_LOG_FILE)
        } catch (e: Exception) {
            Log.e(TAG, "Failed to get crash log file", e)
            null
        }
    }
    
    /**
     * Gets the last crash details file
     */
    fun getLastCrashFile(): File? {
        return try {
            File(filesDir, "last_crash.txt")
        } catch (e: Exception) {
            Log.e(TAG, "Failed to get last crash file", e)
            null
        }
    }

    /**
     * Checks if the JNI library has been loaded and is ready
     * @return true if JNI_OnLoad completed successfully
     */
    external fun isJNILibraryReady(): Boolean
}