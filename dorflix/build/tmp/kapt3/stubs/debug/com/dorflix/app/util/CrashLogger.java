package com.dorflix.app.util;

/**
 * Comprehensive crash logging utility for Dorflix app
 * Provides both Android logcat logging and persistent file logging
 * Thread-safe implementation with proper handler management
 */
@kotlin.Metadata(mv = {2, 3, 0}, k = 1, xi = 48, d1 = {"\u0000n\n\u0002\u0018\u0002\n\u0002\u0010\u0000\n\u0002\b\u0003\n\u0002\u0010\u000e\n\u0002\b\u0002\n\u0002\u0010\b\n\u0002\b\u0002\n\u0002\u0018\u0002\n\u0000\n\u0002\u0018\u0002\n\u0000\n\u0002\u0018\u0002\n\u0002\u0018\u0002\n\u0000\n\u0002\u0018\u0002\n\u0000\n\u0002\u0018\u0002\n\u0002\b\u0002\n\u0002\u0018\u0002\n\u0000\n\u0002\u0010\u0002\n\u0002\b\u0005\n\u0002\u0010\u0003\n\u0002\b\n\n\u0002\u0010\t\n\u0002\b\u0005\n\u0002\u0010\u000b\n\u0002\b\u0003\n\u0002\u0010\u0006\n\u0002\b\r\b\u00c6\u0002\u0018\u00002\u00020\u0001:\u0001@B\t\b\u0002\u00a2\u0006\u0004\b\u0002\u0010\u0003J\u000e\u0010\u0018\u001a\u00020\u00192\u0006\u0010\u001a\u001a\u00020\u000bJ\b\u0010\u001b\u001a\u00020\u0019H\u0002J\u001a\u0010\u001c\u001a\u00020\u00192\u0006\u0010\u001d\u001a\u00020\u00052\n\b\u0002\u0010\u001e\u001a\u0004\u0018\u00010\u001fJ\u001a\u0010 \u001a\u00020\u00192\u0006\u0010\u001d\u001a\u00020\u00052\n\b\u0002\u0010\u001e\u001a\u0004\u0018\u00010\u001fJ\u001a\u0010!\u001a\u00020\u00192\u0006\u0010\u001d\u001a\u00020\u00052\n\b\u0002\u0010\u001e\u001a\u0004\u0018\u00010\u001fJ\u001a\u0010\"\u001a\u00020\u00192\u0006\u0010\u001d\u001a\u00020\u00052\n\b\u0002\u0010\u001e\u001a\u0004\u0018\u00010\u001fJ\u001a\u0010#\u001a\u00020\u00192\u0006\u0010\u001d\u001a\u00020\u00052\n\b\u0002\u0010\u001e\u001a\u0004\u0018\u00010\u001fJ\u0016\u0010$\u001a\u00020\u00192\u0006\u0010\u001d\u001a\u00020\u00052\u0006\u0010\u001e\u001a\u00020\u001fJ\u0018\u0010%\u001a\u00020\u00192\u0006\u0010&\u001a\u00020\u00052\b\b\u0002\u0010\'\u001a\u00020\u0005J\u001e\u0010(\u001a\u00020\u00192\u0006\u0010)\u001a\u00020*2\u0006\u0010+\u001a\u00020*2\u0006\u0010,\u001a\u00020*J \u0010-\u001a\u00020\u00192\u0006\u0010.\u001a\u00020\u00052\u0006\u0010/\u001a\u0002002\b\b\u0002\u0010\'\u001a\u00020\u0005J\u001e\u00101\u001a\u00020\u00192\u0006\u00102\u001a\u00020\b2\u0006\u00103\u001a\u0002042\u0006\u00105\u001a\u00020*J\b\u00106\u001a\u00020\u0019H\u0002J\"\u00107\u001a\u00020\u00192\u0006\u00108\u001a\u00020\u00052\u0006\u0010\u001d\u001a\u00020\u00052\b\u0010\u001e\u001a\u0004\u0018\u00010\u001fH\u0002J\"\u00109\u001a\u00020\u00192\u0006\u00108\u001a\u00020\u00052\u0006\u0010\u001d\u001a\u00020\u00052\b\u0010\u001e\u001a\u0004\u0018\u00010\u001fH\u0002J\b\u0010:\u001a\u00020\u0019H\u0002J\u0010\u0010;\u001a\u00020\u00192\u0006\u0010<\u001a\u00020\u000bH\u0002J\u0010\u0010=\u001a\u00020\u00052\u0006\u0010\u001e\u001a\u00020\u001fH\u0002J\u0010\u0010>\u001a\u00020\u00052\u0006\u0010?\u001a\u00020*H\u0002R\u000e\u0010\u0004\u001a\u00020\u0005X\u0082T\u00a2\u0006\u0002\n\u0000R\u000e\u0010\u0006\u001a\u00020\u0005X\u0082T\u00a2\u0006\u0002\n\u0000R\u000e\u0010\u0007\u001a\u00020\bX\u0082T\u00a2\u0006\u0002\n\u0000R\u000e\u0010\t\u001a\u00020\bX\u0082T\u00a2\u0006\u0002\n\u0000R\u0010\u0010\n\u001a\u0004\u0018\u00010\u000bX\u0082\u000e\u00a2\u0006\u0002\n\u0000R\u000e\u0010\f\u001a\u00020\rX\u0082\u0004\u00a2\u0006\u0002\n\u0000R\u0014\u0010\u000e\u001a\b\u0012\u0004\u0012\u00020\u00100\u000fX\u0082\u0004\u00a2\u0006\u0002\n\u0000R\u000e\u0010\u0011\u001a\u00020\u0012X\u0082\u0004\u00a2\u0006\u0002\n\u0000R\u000e\u0010\u0013\u001a\u00020\u0014X\u0082\u0004\u00a2\u0006\u0002\n\u0000R\u000e\u0010\u0015\u001a\u00020\u0012X\u0082\u000e\u00a2\u0006\u0002\n\u0000R\u0010\u0010\u0016\u001a\u0004\u0018\u00010\u0017X\u0082\u000e\u00a2\u0006\u0002\n\u0000\u00a8\u0006A"}, d2 = {"Lcom/dorflix/app/util/CrashLogger;", "", "<init>", "()V", "TAG", "", "LOG_FILE_NAME", "MAX_LOG_SIZE", "", "MAX_QUEUE_SIZE", "logFile", "Ljava/io/File;", "dateFormat", "Ljava/text/SimpleDateFormat;", "logQueue", "Ljava/util/concurrent/ConcurrentLinkedQueue;", "Lcom/dorflix/app/util/CrashLogger$LogEntry;", "isLogging", "Ljava/util/concurrent/atomic/AtomicBoolean;", "fileLock", "Ljava/util/concurrent/locks/ReentrantLock;", "isMainThreadValid", "mainThread", "Ljava/lang/Thread;", "init", "", "logDir", "setupMainThreadWatcher", "v", "message", "throwable", "", "d", "i", "w", "e", "fatal", "videoEvent", "event", "details", "memoryStats", "used", "", "free", "total", "jniCall", "method", "result", "", "videoPerformance", "frameCount", "fps", "", "avgFrameTime", "setupUncaughtExceptionHandler", "writeToFile", "level", "safeLogToFile", "processLogQueue", "rotateLogFile", "file", "getStackTraceString", "formatBytes", "bytes", "LogEntry", "DorflixNative_debug"})
public final class CrashLogger {
    @org.jetbrains.annotations.NotNull()
    private static final java.lang.String TAG = "DorflixCrash";
    @org.jetbrains.annotations.NotNull()
    private static final java.lang.String LOG_FILE_NAME = "crash_log.txt";
    private static final int MAX_LOG_SIZE = 1048576;
    private static final int MAX_QUEUE_SIZE = 1000;
    @org.jetbrains.annotations.Nullable()
    private static java.io.File logFile;
    @org.jetbrains.annotations.NotNull()
    private static final java.text.SimpleDateFormat dateFormat = null;
    @org.jetbrains.annotations.NotNull()
    private static final java.util.concurrent.ConcurrentLinkedQueue<com.dorflix.app.util.CrashLogger.LogEntry> logQueue = null;
    @org.jetbrains.annotations.NotNull()
    private static final java.util.concurrent.atomic.AtomicBoolean isLogging = null;
    @org.jetbrains.annotations.NotNull()
    private static final java.util.concurrent.locks.ReentrantLock fileLock = null;
    @org.jetbrains.annotations.NotNull()
    private static java.util.concurrent.atomic.AtomicBoolean isMainThreadValid;
    @org.jetbrains.annotations.Nullable()
    private static java.lang.Thread mainThread;
    @org.jetbrains.annotations.NotNull()
    public static final com.dorflix.app.util.CrashLogger INSTANCE = null;
    
    private CrashLogger() {
        super();
    }
    
    /**
     * Initialize crash logger with application context
     */
    public final void init(@org.jetbrains.annotations.NotNull()
    java.io.File logDir) {
    }
    
    /**
     * Setup main thread watcher to detect thread death
     */
    private final void setupMainThreadWatcher() {
    }
    
    /**
     * Log verbose message
     */
    public final void v(@org.jetbrains.annotations.NotNull()
    java.lang.String message, @org.jetbrains.annotations.Nullable()
    java.lang.Throwable throwable) {
    }
    
    /**
     * Log debug message
     */
    public final void d(@org.jetbrains.annotations.NotNull()
    java.lang.String message, @org.jetbrains.annotations.Nullable()
    java.lang.Throwable throwable) {
    }
    
    /**
     * Log info message
     */
    public final void i(@org.jetbrains.annotations.NotNull()
    java.lang.String message, @org.jetbrains.annotations.Nullable()
    java.lang.Throwable throwable) {
    }
    
    /**
     * Log warning message
     */
    public final void w(@org.jetbrains.annotations.NotNull()
    java.lang.String message, @org.jetbrains.annotations.Nullable()
    java.lang.Throwable throwable) {
    }
    
    /**
     * Log error message
     */
    public final void e(@org.jetbrains.annotations.NotNull()
    java.lang.String message, @org.jetbrains.annotations.Nullable()
    java.lang.Throwable throwable) {
    }
    
    /**
     * Log fatal crash
     */
    public final void fatal(@org.jetbrains.annotations.NotNull()
    java.lang.String message, @org.jetbrains.annotations.NotNull()
    java.lang.Throwable throwable) {
    }
    
    /**
     * Log video player specific events
     */
    public final void videoEvent(@org.jetbrains.annotations.NotNull()
    java.lang.String event, @org.jetbrains.annotations.NotNull()
    java.lang.String details) {
    }
    
    /**
     * Log memory usage statistics
     */
    public final void memoryStats(long used, long free, long total) {
    }
    
    /**
     * Log JNI/native call results
     */
    public final void jniCall(@org.jetbrains.annotations.NotNull()
    java.lang.String method, boolean result, @org.jetbrains.annotations.NotNull()
    java.lang.String details) {
    }
    
    /**
     * Log video decoding performance
     */
    public final void videoPerformance(int frameCount, double fps, long avgFrameTime) {
    }
    
    private final void setupUncaughtExceptionHandler() {
    }
    
    /**
     * Thread-safe file writing with queue management
     */
    private final void writeToFile(java.lang.String level, java.lang.String message, java.lang.Throwable throwable) {
    }
    
    /**
     * Safe file writing for critical operations (like uncaught exceptions)
     */
    private final void safeLogToFile(java.lang.String level, java.lang.String message, java.lang.Throwable throwable) {
    }
    
    /**
     * Process the log queue in background
     */
    private final void processLogQueue() {
    }
    
    private final void rotateLogFile(java.io.File file) {
    }
    
    private final java.lang.String getStackTraceString(java.lang.Throwable throwable) {
        return null;
    }
    
    private final java.lang.String formatBytes(long bytes) {
        return null;
    }
    
    /**
     * Log entry data class for thread-safe operations
     */
    @kotlin.Metadata(mv = {2, 3, 0}, k = 1, xi = 48, d1 = {"\u0000*\n\u0002\u0018\u0002\n\u0002\u0010\u0000\n\u0000\n\u0002\u0010\u000e\n\u0002\b\u0002\n\u0002\u0010\u0003\n\u0002\b\f\n\u0002\u0010\u000b\n\u0002\b\u0002\n\u0002\u0010\b\n\u0002\b\u0002\b\u0082\b\u0018\u00002\u00020\u0001B!\u0012\u0006\u0010\u0002\u001a\u00020\u0003\u0012\u0006\u0010\u0004\u001a\u00020\u0003\u0012\b\u0010\u0005\u001a\u0004\u0018\u00010\u0006\u00a2\u0006\u0004\b\u0007\u0010\bJ\t\u0010\u000e\u001a\u00020\u0003H\u00c6\u0003J\t\u0010\u000f\u001a\u00020\u0003H\u00c6\u0003J\u000b\u0010\u0010\u001a\u0004\u0018\u00010\u0006H\u00c6\u0003J)\u0010\u0011\u001a\u00020\u00002\b\b\u0002\u0010\u0002\u001a\u00020\u00032\b\b\u0002\u0010\u0004\u001a\u00020\u00032\n\b\u0002\u0010\u0005\u001a\u0004\u0018\u00010\u0006H\u00c6\u0001J\u0014\u0010\u0012\u001a\u00020\u00132\b\u0010\u0014\u001a\u0004\u0018\u00010\u0001H\u00d6\u0083\u0004J\n\u0010\u0015\u001a\u00020\u0016H\u00d6\u0081\u0004J\n\u0010\u0017\u001a\u00020\u0003H\u00d6\u0081\u0004R\u0011\u0010\u0002\u001a\u00020\u0003\u00a2\u0006\b\n\u0000\u001a\u0004\b\t\u0010\nR\u0011\u0010\u0004\u001a\u00020\u0003\u00a2\u0006\b\n\u0000\u001a\u0004\b\u000b\u0010\nR\u0013\u0010\u0005\u001a\u0004\u0018\u00010\u0006\u00a2\u0006\b\n\u0000\u001a\u0004\b\f\u0010\r\u00a8\u0006\u0018"}, d2 = {"Lcom/dorflix/app/util/CrashLogger$LogEntry;", "", "level", "", "message", "throwable", "", "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/Throwable;)V", "getLevel", "()Ljava/lang/String;", "getMessage", "getThrowable", "()Ljava/lang/Throwable;", "component1", "component2", "component3", "copy", "equals", "", "other", "hashCode", "", "toString", "DorflixNative_debug"})
    static final class LogEntry {
        @org.jetbrains.annotations.NotNull()
        private final java.lang.String level = null;
        @org.jetbrains.annotations.NotNull()
        private final java.lang.String message = null;
        @org.jetbrains.annotations.Nullable()
        private final java.lang.Throwable throwable = null;
        
        public LogEntry(@org.jetbrains.annotations.NotNull()
        java.lang.String level, @org.jetbrains.annotations.NotNull()
        java.lang.String message, @org.jetbrains.annotations.Nullable()
        java.lang.Throwable throwable) {
            super();
        }
        
        @org.jetbrains.annotations.NotNull()
        public final java.lang.String getLevel() {
            return null;
        }
        
        @org.jetbrains.annotations.NotNull()
        public final java.lang.String getMessage() {
            return null;
        }
        
        @org.jetbrains.annotations.Nullable()
        public final java.lang.Throwable getThrowable() {
            return null;
        }
        
        @org.jetbrains.annotations.NotNull()
        public final java.lang.String component1() {
            return null;
        }
        
        @org.jetbrains.annotations.NotNull()
        public final java.lang.String component2() {
            return null;
        }
        
        @org.jetbrains.annotations.Nullable()
        public final java.lang.Throwable component3() {
            return null;
        }
        
        @org.jetbrains.annotations.NotNull()
        public final com.dorflix.app.util.CrashLogger.LogEntry copy(@org.jetbrains.annotations.NotNull()
        java.lang.String level, @org.jetbrains.annotations.NotNull()
        java.lang.String message, @org.jetbrains.annotations.Nullable()
        java.lang.Throwable throwable) {
            return null;
        }
        
        @java.lang.Override()
        public boolean equals(@org.jetbrains.annotations.Nullable()
        java.lang.Object other) {
            return false;
        }
        
        @java.lang.Override()
        public int hashCode() {
            return 0;
        }
        
        @java.lang.Override()
        @org.jetbrains.annotations.NotNull()
        public java.lang.String toString() {
            return null;
        }
    }
}