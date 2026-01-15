package com.dorflix.app.util

import android.util.Log
import java.io.File
import java.io.FileWriter
import java.io.IOException
import java.text.SimpleDateFormat
import java.util.*
import java.util.concurrent.ConcurrentLinkedQueue
import java.util.concurrent.atomic.AtomicBoolean
import java.util.concurrent.locks.ReentrantLock
import kotlin.concurrent.withLock

/**
 * Comprehensive crash logging utility for Dorflix app
 * Provides both Android logcat logging and persistent file logging
 * Thread-safe implementation with proper handler management
 */
object CrashLogger {
    
    private const val TAG = "DorflixCrash"
    private const val LOG_FILE_NAME = "crash_log.txt"
    private const val MAX_LOG_SIZE = 1024 * 1024 // 1MB
    private const val MAX_QUEUE_SIZE = 1000
    
    private var logFile: File? = null
    private val dateFormat = SimpleDateFormat("yyyy-MM-dd HH:mm:ss.SSS", Locale.getDefault())
    
    // Thread-safe logging queue
    private val logQueue = ConcurrentLinkedQueue<LogEntry>()
    private val isLogging = AtomicBoolean(false)
    private val fileLock = ReentrantLock()
    
    // Handler safety
    private var isMainThreadValid = AtomicBoolean(true)
    private var mainThread: Thread? = null
    
    /**
     * Log entry data class for thread-safe operations
     */
    private data class LogEntry(
        val level: String,
        val message: String,
        val throwable: Throwable?
    )
    
    /**
     * Initialize crash logger with application context
     */
    fun init(logDir: File) {
        logFile = File(logDir, LOG_FILE_NAME)
        setupUncaughtExceptionHandler()
        setupMainThreadWatcher()
    }
    
    /**
     * Setup main thread watcher to detect thread death
     */
    private fun setupMainThreadWatcher() {
        mainThread = Thread.currentThread()
        isMainThreadValid.set(true)
        
        // Monitor main thread for death
        Thread {
            try {
                mainThread?.join()
                isMainThreadValid.set(false)
                Log.w(TAG, "Main thread has died, switching to background logging")
            } catch (e: InterruptedException) {
                // Thread was interrupted, still valid
                Thread.currentThread().interrupt()
            }
        }.start()
    }
    
    /**
     * Log verbose message
     */
    fun v(message: String, throwable: Throwable? = null) {
        Log.v(TAG, message, throwable)
        writeToFile("VERBOSE", message, throwable)
    }
    
    /**
     * Log debug message
     */
    fun d(message: String, throwable: Throwable? = null) {
        Log.d(TAG, message, throwable)
        writeToFile("DEBUG", message, throwable)
    }
    
    /**
     * Log info message
     */
    fun i(message: String, throwable: Throwable? = null) {
        Log.i(TAG, message, throwable)
        writeToFile("INFO", message, throwable)
    }
    
    /**
     * Log warning message
     */
    fun w(message: String, throwable: Throwable? = null) {
        Log.w(TAG, message, throwable)
        writeToFile("WARN", message, throwable)
    }
    
    /**
     * Log error message
     */
    fun e(message: String, throwable: Throwable? = null) {
        Log.e(TAG, message, throwable)
        writeToFile("ERROR", message, throwable)
    }
    
    /**
     * Log fatal crash
     */
    fun fatal(message: String, throwable: Throwable) {
        Log.e(TAG, "FATAL CRASH: $message", throwable)
        writeToFile("FATAL", "FATAL CRASH: $message", throwable)
        
        // Log stack trace details
        val stackTrace = getStackTraceString(throwable)
        writeToFile("STACKTRACE", stackTrace, null)
    }
    
    /**
     * Log video player specific events
     */
    fun videoEvent(event: String, details: String = "") {
        val message = "VIDEO_EVENT: $event${if (details.isNotEmpty()) " - $details" else ""}"
        Log.d(TAG, message)
        writeToFile("VIDEO", message, null)
    }
    
    /**
     * Log memory usage statistics
     */
    fun memoryStats(used: Long, free: Long, total: Long) {
        val message = "MEMORY: Used=${formatBytes(used)}, Free=${formatBytes(free)}, Total=${formatBytes(total)}"
        Log.d(TAG, message)
        writeToFile("MEMORY", message, null)
    }
    
    /**
     * Log JNI/native call results
     */
    fun jniCall(method: String, result: Boolean, details: String = "") {
        val status = if (result) "SUCCESS" else "FAILED"
        val message = "JNI: $method - $status${if (details.isNotEmpty()) " - $details" else ""}"
        Log.d(TAG, message)
        writeToFile("JNI", message, null)
    }
    
    /**
     * Log video decoding performance
     */
    fun videoPerformance(frameCount: Int, fps: Double, avgFrameTime: Long) {
        val message = "PERF: Frames=$frameCount, FPS=${"%.2f".format(fps)}, AvgFrameTime=${avgFrameTime}ms"
        Log.d(TAG, message)
        writeToFile("PERF", message, null)
    }
    
    private fun setupUncaughtExceptionHandler() {
        val currentHandler = Thread.getDefaultUncaughtExceptionHandler()
        
        Thread.setDefaultUncaughtExceptionHandler { thread, throwable ->
            // Use safe logging for uncaught exceptions
            safeLogToFile("FATAL", "Uncaught exception in thread: ${thread.name}", throwable)
            
            // Log stack trace details
            val stackTrace = getStackTraceString(throwable)
            safeLogToFile("STACKTRACE", stackTrace, null)
            
            // Call the original handler if it exists
            currentHandler?.uncaughtException(thread, throwable)
        }
    }
    
    /**
     * Thread-safe file writing with queue management
     */
    private fun writeToFile(level: String, message: String, throwable: Throwable?) {
        // Add to queue for background processing
        if (logQueue.size < MAX_QUEUE_SIZE) {
            logQueue.offer(LogEntry(level, message, throwable))
        } else {
            Log.w(TAG, "Log queue full, dropping entry: $level - $message")
        }
        
        // Process queue if not already running
        if (isLogging.compareAndSet(false, true)) {
            processLogQueue()
        }
    }
    
    /**
     * Safe file writing for critical operations (like uncaught exceptions)
     */
    private fun safeLogToFile(level: String, message: String, throwable: Throwable?) {
        try {
            logFile?.let { file ->
                fileLock.withLock {
                    // Rotate log file if it gets too large
                    if (file.exists() && file.length() > MAX_LOG_SIZE) {
                        rotateLogFile(file)
                    }
                    
                    val timestamp = dateFormat.format(Date())
                    val logEntry = "$timestamp [$level] $message"
                    
                    FileWriter(file, true).use { writer ->
                        writer.append(logEntry)
                        if (throwable != null) {
                            writer.append("\n")
                            writer.append(getStackTraceString(throwable))
                        }
                        writer.append("\n")
                    }
                }
            }
        } catch (e: IOException) {
            Log.e(TAG, "Failed to write to log file", e)
        }
    }
    
    /**
     * Process the log queue in background
     */
    private fun processLogQueue() {
        Thread {
            try {
                while (!logQueue.isEmpty()) {
                    val entry = logQueue.poll()
                    if (entry != null) {
                        safeLogToFile(entry.level, entry.message, entry.throwable)
                    }
                }
            } finally {
                isLogging.set(false)
                
                // If there are more entries, process them
                if (!logQueue.isEmpty()) {
                    if (isLogging.compareAndSet(false, true)) {
                        processLogQueue()
                    }
                }
            }
        }.start()
    }
    
    private fun rotateLogFile(file: File) {
        try {
            val backupFile = File(file.parent, "${LOG_FILE_NAME}.backup")
            if (backupFile.exists()) {
                backupFile.delete()
            }
            file.renameTo(backupFile)
        } catch (e: Exception) {
            Log.e(TAG, "Failed to rotate log file", e)
        }
    }
    
    private fun getStackTraceString(throwable: Throwable): String {
        return Log.getStackTraceString(throwable)
    }
    
    private fun formatBytes(bytes: Long): String {
        return when {
            bytes < 1024 -> "${bytes}B"
            bytes < 1024 * 1024 -> "${bytes / 1024}KB"
            bytes < 1024 * 1024 * 1024 -> "${bytes / (1024 * 1024)}MB"
            else -> "${bytes / (1024 * 1024 * 1024)}GB"
        }
    }
}
