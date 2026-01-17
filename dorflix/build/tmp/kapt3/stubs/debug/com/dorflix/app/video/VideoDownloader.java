package com.dorflix.app.video;

@kotlin.Metadata(mv = {2, 3, 0}, k = 1, xi = 48, d1 = {"\u0000Z\n\u0002\u0018\u0002\n\u0002\u0010\u0000\n\u0000\n\u0002\u0018\u0002\n\u0002\b\u0003\n\u0002\u0010\u000e\n\u0000\n\u0002\u0018\u0002\n\u0000\n\u0002\u0018\u0002\n\u0000\n\u0002\u0010%\n\u0002\u0018\u0002\n\u0000\n\u0002\u0010#\n\u0002\b\u0003\n\u0002\u0010!\n\u0002\u0018\u0002\n\u0002\b\u0004\n\u0002\u0010\u000b\n\u0002\b\u0002\n\u0002\u0010\u0002\n\u0002\b\b\n\u0002\u0010\t\n\u0002\b\u0002\u0018\u0000 \'2\u00020\u0001:\u0001\'B\u000f\u0012\u0006\u0010\u0002\u001a\u00020\u0003\u00a2\u0006\u0004\b\u0004\u0010\u0005J\u000e\u0010\u0016\u001a\u00020\u00072\u0006\u0010\u0017\u001a\u00020\u0007J\u000e\u0010\u0018\u001a\u00020\u00072\u0006\u0010\u0017\u001a\u00020\u0007J\u000e\u0010\u0019\u001a\u00020\u001a2\u0006\u0010\u0017\u001a\u00020\u0007J\u000e\u0010\u001b\u001a\u00020\u00072\u0006\u0010\u0017\u001a\u00020\u0007J\u000e\u0010\u001c\u001a\u00020\u001d2\u0006\u0010\u0017\u001a\u00020\u0007J\u000e\u0010\u001e\u001a\u00020\u001d2\u0006\u0010\u0017\u001a\u00020\u0007J\u0018\u0010\u001f\u001a\u00020\u001d2\u0006\u0010\u0017\u001a\u00020\u00072\u0006\u0010 \u001a\u00020\u000bH\u0002J\u001e\u0010!\u001a\u00020\u001d2\u0006\u0010\u0017\u001a\u00020\u00072\u0006\u0010 \u001a\u00020\u000bH\u0082@\u00a2\u0006\u0002\u0010\"J\u0010\u0010#\u001a\u00020\u000b2\u0006\u0010\u0017\u001a\u00020\u0007H\u0002J\u0006\u0010$\u001a\u00020\u001dJ\u0006\u0010%\u001a\u00020&R\u000e\u0010\u0002\u001a\u00020\u0003X\u0082\u0004\u00a2\u0006\u0002\n\u0000R\u000e\u0010\u0006\u001a\u00020\u0007X\u0082D\u00a2\u0006\u0002\n\u0000R\u000e\u0010\b\u001a\u00020\tX\u0082\u0004\u00a2\u0006\u0002\n\u0000R\u000e\u0010\n\u001a\u00020\u000bX\u0082\u0004\u00a2\u0006\u0002\n\u0000R\u001a\u0010\f\u001a\u000e\u0012\u0004\u0012\u00020\u0007\u0012\u0004\u0012\u00020\u000e0\rX\u0082\u0004\u00a2\u0006\u0002\n\u0000R\u0014\u0010\u000f\u001a\b\u0012\u0004\u0012\u00020\u00070\u0010X\u0082\u0004\u00a2\u0006\u0002\n\u0000R\u001a\u0010\u0011\u001a\u000e\u0012\u0004\u0012\u00020\u0007\u0012\u0004\u0012\u00020\u000e0\rX\u0082\u0004\u00a2\u0006\u0002\n\u0000R\u001a\u0010\u0012\u001a\u000e\u0012\u0004\u0012\u00020\u0007\u0012\u0004\u0012\u00020\u000b0\rX\u0082\u0004\u00a2\u0006\u0002\n\u0000R\u0014\u0010\u0013\u001a\b\u0012\u0004\u0012\u00020\u00150\u0014X\u0082\u0004\u00a2\u0006\u0002\n\u0000\u00a8\u0006("}, d2 = {"Lcom/dorflix/app/video/VideoDownloader;", "", "context", "Landroid/content/Context;", "<init>", "(Landroid/content/Context;)V", "TAG", "", "client", "Lokhttp3/OkHttpClient;", "cacheDir", "Ljava/io/File;", "downloadJobs", "", "Lkotlinx/coroutines/Job;", "corruptedSegments", "", "segmentRedownloadJobs", "segmentCache", "downloadListeners", "", "Lcom/dorflix/app/video/DownloadListener;", "downloadVideo", "url", "downloadVideoSync", "isVideoCached", "", "getCachedPath", "preloadVideo", "", "cancelDownload", "downloadFileSync", "outputFile", "downloadFile", "(Ljava/lang/String;Ljava/io/File;Lkotlin/coroutines/Continuation;)Ljava/lang/Object;", "getCachedFile", "cleanupCache", "getCacheSize", "", "Companion", "DorflixNative_debug"})
public final class VideoDownloader {
    @org.jetbrains.annotations.NotNull()
    private final android.content.Context context = null;
    @org.jetbrains.annotations.NotNull()
    private final java.lang.String TAG = "VideoDownloader";
    @org.jetbrains.annotations.NotNull()
    private final okhttp3.OkHttpClient client = null;
    @org.jetbrains.annotations.NotNull()
    private final java.io.File cacheDir = null;
    @org.jetbrains.annotations.NotNull()
    private final java.util.Map<java.lang.String, kotlinx.coroutines.Job> downloadJobs = null;
    @org.jetbrains.annotations.NotNull()
    private final java.util.Set<java.lang.String> corruptedSegments = null;
    @org.jetbrains.annotations.NotNull()
    private final java.util.Map<java.lang.String, kotlinx.coroutines.Job> segmentRedownloadJobs = null;
    @org.jetbrains.annotations.NotNull()
    private final java.util.Map<java.lang.String, java.io.File> segmentCache = null;
    @org.jetbrains.annotations.NotNull()
    private final java.util.List<com.dorflix.app.video.DownloadListener> downloadListeners = null;
    private static com.dorflix.app.video.VideoDownloader instance;
    @org.jetbrains.annotations.NotNull()
    public static final com.dorflix.app.video.VideoDownloader.Companion Companion = null;
    
    public VideoDownloader(@org.jetbrains.annotations.NotNull()
    android.content.Context context) {
        super();
    }
    
    @kotlin.jvm.JvmStatic()
    public static final void initialize(@org.jetbrains.annotations.NotNull()
    android.content.Context context) {
    }
    
    @kotlin.jvm.JvmStatic()
    public static final void addDownloadListener(@org.jetbrains.annotations.NotNull()
    com.dorflix.app.video.DownloadListener listener) {
    }
    
    @kotlin.jvm.JvmStatic()
    public static final void removeDownloadListener(@org.jetbrains.annotations.NotNull()
    com.dorflix.app.video.DownloadListener listener) {
    }
    
    @kotlin.jvm.JvmStatic()
    public static final void nativeInitializeCache(@org.jetbrains.annotations.NotNull()
    java.lang.String cacheDir) {
    }
    
    @kotlin.jvm.JvmStatic()
    @org.jetbrains.annotations.NotNull()
    public static final java.lang.String nativeDownloadVideo(@org.jetbrains.annotations.NotNull()
    java.lang.String url) {
        return null;
    }
    
    @kotlin.jvm.JvmStatic()
    public static final boolean nativeIsVideoCached(@org.jetbrains.annotations.NotNull()
    java.lang.String url) {
        return false;
    }
    
    @kotlin.jvm.JvmStatic()
    @org.jetbrains.annotations.NotNull()
    public static final java.lang.String nativeGetCachedPath(@org.jetbrains.annotations.NotNull()
    java.lang.String url) {
        return null;
    }
    
    @kotlin.jvm.JvmStatic()
    public static final void nativePreloadVideo(@org.jetbrains.annotations.NotNull()
    java.lang.String url) {
    }
    
    @kotlin.jvm.JvmStatic()
    public static final void nativeCleanupCache() {
    }
    
    @kotlin.jvm.JvmStatic()
    public static final long nativeGetCacheSize() {
        return 0L;
    }
    
    @kotlin.jvm.JvmStatic()
    public static final void onDownloadProgress(@org.jetbrains.annotations.NotNull()
    java.lang.String url, long downloaded, long total) {
    }
    
    @kotlin.jvm.JvmStatic()
    public static final void onDownloadComplete(@org.jetbrains.annotations.NotNull()
    java.lang.String url, @org.jetbrains.annotations.NotNull()
    java.lang.String localPath) {
    }
    
    @kotlin.jvm.JvmStatic()
    public static final void onDownloadError(@org.jetbrains.annotations.NotNull()
    java.lang.String url, @org.jetbrains.annotations.NotNull()
    java.lang.String error) {
    }
    
    @org.jetbrains.annotations.NotNull()
    public final java.lang.String downloadVideo(@org.jetbrains.annotations.NotNull()
    java.lang.String url) {
        return null;
    }
    
    @org.jetbrains.annotations.NotNull()
    public final java.lang.String downloadVideoSync(@org.jetbrains.annotations.NotNull()
    java.lang.String url) {
        return null;
    }
    
    public final boolean isVideoCached(@org.jetbrains.annotations.NotNull()
    java.lang.String url) {
        return false;
    }
    
    @org.jetbrains.annotations.NotNull()
    public final java.lang.String getCachedPath(@org.jetbrains.annotations.NotNull()
    java.lang.String url) {
        return null;
    }
    
    public final void preloadVideo(@org.jetbrains.annotations.NotNull()
    java.lang.String url) {
    }
    
    public final void cancelDownload(@org.jetbrains.annotations.NotNull()
    java.lang.String url) {
    }
    
    private final void downloadFileSync(java.lang.String url, java.io.File outputFile) {
    }
    
    private final java.lang.Object downloadFile(java.lang.String url, java.io.File outputFile, kotlin.coroutines.Continuation<? super kotlin.Unit> $completion) {
        return null;
    }
    
    private final java.io.File getCachedFile(java.lang.String url) {
        return null;
    }
    
    public final void cleanupCache() {
    }
    
    public final long getCacheSize() {
        return 0L;
    }
    
    @kotlin.Metadata(mv = {2, 3, 0}, k = 1, xi = 48, d1 = {"\u0000@\n\u0002\u0018\u0002\n\u0002\u0010\u0000\n\u0002\b\u0003\n\u0002\u0018\u0002\n\u0000\n\u0002\u0010\u0002\n\u0000\n\u0002\u0018\u0002\n\u0002\b\u0002\n\u0002\u0018\u0002\n\u0002\b\u0003\n\u0002\u0010\u000e\n\u0002\b\u0003\n\u0002\u0010\u000b\n\u0002\b\u0004\n\u0002\u0010\t\n\u0002\b\b\b\u0086\u0003\u0018\u00002\u00020\u0001B\t\b\u0002\u00a2\u0006\u0004\b\u0002\u0010\u0003J\u0010\u0010\u0006\u001a\u00020\u00072\u0006\u0010\b\u001a\u00020\tH\u0007J\u0010\u0010\n\u001a\u00020\u00072\u0006\u0010\u000b\u001a\u00020\fH\u0007J\u0010\u0010\r\u001a\u00020\u00072\u0006\u0010\u000b\u001a\u00020\fH\u0007J\u0010\u0010\u000e\u001a\u00020\u00072\u0006\u0010\u000f\u001a\u00020\u0010H\u0007J\u0010\u0010\u0011\u001a\u00020\u00102\u0006\u0010\u0012\u001a\u00020\u0010H\u0007J\u0010\u0010\u0013\u001a\u00020\u00142\u0006\u0010\u0012\u001a\u00020\u0010H\u0007J\u0010\u0010\u0015\u001a\u00020\u00102\u0006\u0010\u0012\u001a\u00020\u0010H\u0007J\u0010\u0010\u0016\u001a\u00020\u00072\u0006\u0010\u0012\u001a\u00020\u0010H\u0007J\b\u0010\u0017\u001a\u00020\u0007H\u0007J\b\u0010\u0018\u001a\u00020\u0019H\u0007J \u0010\u001a\u001a\u00020\u00072\u0006\u0010\u0012\u001a\u00020\u00102\u0006\u0010\u001b\u001a\u00020\u00192\u0006\u0010\u001c\u001a\u00020\u0019H\u0007J\u0018\u0010\u001d\u001a\u00020\u00072\u0006\u0010\u0012\u001a\u00020\u00102\u0006\u0010\u001e\u001a\u00020\u0010H\u0007J\u0018\u0010\u001f\u001a\u00020\u00072\u0006\u0010\u0012\u001a\u00020\u00102\u0006\u0010 \u001a\u00020\u0010H\u0007R\u000e\u0010\u0004\u001a\u00020\u0005X\u0082.\u00a2\u0006\u0002\n\u0000\u00a8\u0006!"}, d2 = {"Lcom/dorflix/app/video/VideoDownloader$Companion;", "", "<init>", "()V", "instance", "Lcom/dorflix/app/video/VideoDownloader;", "initialize", "", "context", "Landroid/content/Context;", "addDownloadListener", "listener", "Lcom/dorflix/app/video/DownloadListener;", "removeDownloadListener", "nativeInitializeCache", "cacheDir", "", "nativeDownloadVideo", "url", "nativeIsVideoCached", "", "nativeGetCachedPath", "nativePreloadVideo", "nativeCleanupCache", "nativeGetCacheSize", "", "onDownloadProgress", "downloaded", "total", "onDownloadComplete", "localPath", "onDownloadError", "error", "DorflixNative_debug"})
    public static final class Companion {
        
        private Companion() {
            super();
        }
        
        @kotlin.jvm.JvmStatic()
        public final void initialize(@org.jetbrains.annotations.NotNull()
        android.content.Context context) {
        }
        
        @kotlin.jvm.JvmStatic()
        public final void addDownloadListener(@org.jetbrains.annotations.NotNull()
        com.dorflix.app.video.DownloadListener listener) {
        }
        
        @kotlin.jvm.JvmStatic()
        public final void removeDownloadListener(@org.jetbrains.annotations.NotNull()
        com.dorflix.app.video.DownloadListener listener) {
        }
        
        @kotlin.jvm.JvmStatic()
        public final void nativeInitializeCache(@org.jetbrains.annotations.NotNull()
        java.lang.String cacheDir) {
        }
        
        @kotlin.jvm.JvmStatic()
        @org.jetbrains.annotations.NotNull()
        public final java.lang.String nativeDownloadVideo(@org.jetbrains.annotations.NotNull()
        java.lang.String url) {
            return null;
        }
        
        @kotlin.jvm.JvmStatic()
        public final boolean nativeIsVideoCached(@org.jetbrains.annotations.NotNull()
        java.lang.String url) {
            return false;
        }
        
        @kotlin.jvm.JvmStatic()
        @org.jetbrains.annotations.NotNull()
        public final java.lang.String nativeGetCachedPath(@org.jetbrains.annotations.NotNull()
        java.lang.String url) {
            return null;
        }
        
        @kotlin.jvm.JvmStatic()
        public final void nativePreloadVideo(@org.jetbrains.annotations.NotNull()
        java.lang.String url) {
        }
        
        @kotlin.jvm.JvmStatic()
        public final void nativeCleanupCache() {
        }
        
        @kotlin.jvm.JvmStatic()
        public final long nativeGetCacheSize() {
            return 0L;
        }
        
        @kotlin.jvm.JvmStatic()
        public final void onDownloadProgress(@org.jetbrains.annotations.NotNull()
        java.lang.String url, long downloaded, long total) {
        }
        
        @kotlin.jvm.JvmStatic()
        public final void onDownloadComplete(@org.jetbrains.annotations.NotNull()
        java.lang.String url, @org.jetbrains.annotations.NotNull()
        java.lang.String localPath) {
        }
        
        @kotlin.jvm.JvmStatic()
        public final void onDownloadError(@org.jetbrains.annotations.NotNull()
        java.lang.String url, @org.jetbrains.annotations.NotNull()
        java.lang.String error) {
        }
    }
}