package com.dorflix.app.video

import android.content.Context
import android.util.Log
import kotlinx.coroutines.*
import okhttp3.*
import okio.IOException
import java.io.File
import java.io.FileOutputStream
import java.util.concurrent.TimeUnit

class VideoDownloader(private val context: Context) {
    private val TAG = "VideoDownloader"
    private val client = OkHttpClient.Builder()
        .connectTimeout(30, TimeUnit.SECONDS)
        .readTimeout(30, TimeUnit.SECONDS)
        .writeTimeout(30, TimeUnit.SECONDS)
        .build()

    private val cacheDir: File = File(context.cacheDir, "videos").apply {
        if (!exists()) mkdirs()
    }

    private val downloadJobs = mutableMapOf<String, Job>()

    // =Ê TIKTOK-STYLE: Intelligent Buffering Strategy
    private val corruptedSegments = mutableSetOf<String>() // URLs marked as corrupted
    private val segmentRedownloadJobs = mutableMapOf<String, Job>() // Background re-download jobs
    private val segmentCache = mutableMapOf<String, File>() // URL -> cached file mapping

    companion object {
        // Global VideoDownloader instance for JNI calls
        private lateinit var instance: VideoDownloader

        // Initialize the global instance
        @JvmStatic
        fun initialize(context: Context) {
            instance = VideoDownloader(context)
            Log.i("VideoDownloader", "VideoDownloader instance initialized")
        }

        // Download control methods (called from JNI) - Kotlin implementations, not native
        @JvmStatic
        fun nativeInitializeCache(cacheDir: String) {
            Log.i("VideoDownloader", "nativeInitializeCache called with: $cacheDir")
            // Cache initialization is handled in constructor now
        }

        @JvmStatic
        fun nativeDownloadVideo(url: String): String {
            Log.i("VideoDownloader", "nativeDownloadVideo called for: $url")

            if (!::instance.isInitialized) {
                Log.e("VideoDownloader", "VideoDownloader instance not initialized!")
                return ""
            }

            try {
                val result = instance.downloadVideoSync(url)
                Log.i("VideoDownloader", "nativeDownloadVideo returning: '$result'")
                return result
            } catch (e: Exception) {
                Log.e("VideoDownloader", "Exception in nativeDownloadVideo: ${e.message}", e)
                return ""
            }
        }

        @JvmStatic
        fun nativeIsVideoCached(url: String): Boolean {
            if (!::instance.isInitialized) return false
            return instance.isVideoCached(url)
        }

        @JvmStatic
        fun nativeGetCachedPath(url: String): String {
            if (!::instance.isInitialized) return ""
            return instance.getCachedPath(url)
        }

        @JvmStatic
        fun nativePreloadVideo(url: String) {
            if (::instance.isInitialized) {
                instance.preloadVideo(url)
            }
        }

        @JvmStatic
        fun nativeCleanupCache() {
            if (::instance.isInitialized) {
                instance.cleanupCache()
            }
        }

        @JvmStatic
        fun nativeGetCacheSize(): Long {
            return if (::instance.isInitialized) instance.getCacheSize() else 0L
        }

        // Native callback methods (called from C++)
        @JvmStatic
        fun onDownloadProgress(url: String, downloaded: Long, total: Long) {
            Log.i("VideoDownloader", "Download progress: $url - ${downloaded}/${total}")
        }

        @JvmStatic
        fun onDownloadComplete(url: String, localPath: String) {
            Log.i("VideoDownloader", "Download complete: $url -> $localPath")
        }

        @JvmStatic
        fun onDownloadError(url: String, error: String) {
            Log.e("VideoDownloader", "Download error: $url - $error")
        }
    }

    fun downloadVideo(url: String): String {
        Log.i(TAG, "Starting download for: $url")

        // Check if already cached
        val cachedFile = getCachedFile(url)
        if (cachedFile.exists() && cachedFile.length() > 0) {
            Log.i(TAG, "Video already cached: ${cachedFile.absolutePath}")
            return cachedFile.absolutePath
        }

        // Start download in background coroutine
        val job = CoroutineScope(Dispatchers.IO).launch {
            try {
                downloadFile(url, cachedFile)
            } catch (e: Exception) {
                Log.e(TAG, "Download failed for $url", e)
                onDownloadError(url, e.message ?: "Unknown error")
            }
        }

        downloadJobs[url] = job

        // Return empty string initially - callback will provide path when complete
        return ""
    }

    // Synchronous download method for JNI calls
    fun downloadVideoSync(url: String): String {
        Log.i(TAG, "Synchronous download for: $url")

        // Check if already cached
        val cachedFile = getCachedFile(url)
        if (cachedFile.exists() && cachedFile.length() > 0) {
            Log.i(TAG, "Video already cached: ${cachedFile.absolutePath}")
            return cachedFile.absolutePath
        }

        Log.i(TAG, "Video not cached, downloading synchronously...")

        // Perform synchronous download on background thread to avoid NetworkOnMainThreadException
        return runBlocking {
            try {
                withContext(Dispatchers.IO) {
                    downloadFileSync(url, cachedFile)
                }
                Log.i(TAG, "Synchronous download complete: ${cachedFile.absolutePath}")

                // TikTok-style: Enforce cache limits after each download
                cleanupCache()

                cachedFile.absolutePath
            } catch (e: Exception) {
                Log.e(TAG, "Synchronous download failed for $url: ${e.message}", e)
                ""
            }
        }
    }

    fun isVideoCached(url: String): Boolean {
        val cachedFile = getCachedFile(url)
        val exists = cachedFile.exists() && cachedFile.length() > 0
        Log.d(TAG, "isVideoCached($url) = $exists")
        return exists
    }

    fun getCachedPath(url: String): String {
        val cachedFile = getCachedFile(url)
        return if (cachedFile.exists() && cachedFile.length() > 0) {
            cachedFile.absolutePath
        } else {
            ""
        }
    }

    fun preloadVideo(url: String) {
        Log.i(TAG, "Preloading video: $url")
        downloadVideo(url) // Same logic, just preemptively
    }

    fun cancelDownload(url: String) {
        downloadJobs[url]?.cancel()
        downloadJobs.remove(url)
        Log.i(TAG, "Cancelled download for: $url")
    }

    // Synchronous download method
    private fun downloadFileSync(url: String, outputFile: File) {
        val request = Request.Builder()
            .url(url)
            .addHeader("User-Agent", "Dorflix/1.0 (Android)")
            .build()

        client.newCall(request).execute().use { response ->
            if (!response.isSuccessful) {
                throw IOException("HTTP ${response.code}: ${response.message}")
            }

            val body = response.body ?: throw IOException("Empty response body")
            val contentLength = body.contentLength()

            Log.i(TAG, "Starting synchronous download: $url (${contentLength} bytes)")

            FileOutputStream(outputFile).use { output ->
                val input = body.byteStream()
                val buffer = ByteArray(8192)
                var bytesRead: Int
                var totalBytesRead = 0L

                while (input.read(buffer).also { bytesRead = it } != -1) {
                    output.write(buffer, 0, bytesRead)
                    totalBytesRead += bytesRead

                    Log.d(TAG, "Downloaded $totalBytesRead bytes for $url")
                }
            }

            Log.i(TAG, "Synchronous download complete: $url -> ${outputFile.absolutePath}")
        }
    }

    private suspend fun downloadFile(url: String, outputFile: File) {
        val request = Request.Builder()
            .url(url)
            .addHeader("User-Agent", "Dorflix/1.0 (Android)")
            .build()

        client.newCall(request).execute().use { response ->
            if (!response.isSuccessful) {
                throw IOException("HTTP ${response.code}: ${response.message}")
            }

            val body = response.body ?: throw IOException("Empty response body")
            val contentLength = body.contentLength()

            Log.i(TAG, "Starting download: $url (${contentLength} bytes)")

            FileOutputStream(outputFile).use { output ->
                val input = body.byteStream()
                val buffer = ByteArray(8192)
                var bytesRead: Int
                var totalBytesRead = 0L

                while (input.read(buffer).also { bytesRead = it } != -1) {
                    output.write(buffer, 0, bytesRead)
                    totalBytesRead += bytesRead

                    // Report progress
                    if (contentLength > 0) {
                        onDownloadProgress(url, totalBytesRead, contentLength)
                    }
                }
            }

            Log.i(TAG, "Download complete: $url -> ${outputFile.absolutePath}")
            onDownloadComplete(url, outputFile.absolutePath)
        }
    }

    private fun getCachedFile(url: String): File {
        // Generate filename from URL hash
        val hash = url.hashCode().toString(16)
        return File(cacheDir, "video_$hash.mp4")
    }

    fun cleanupCache() {
        val maxCacheSize = 100 * 1024 * 1024L // TikTok-style: 100MB limit
        val targetSize = 50 * 1024 * 1024L   // Keep under 50MB after cleanup

        val files = cacheDir.listFiles()?.sortedBy { it.lastModified() } ?: return
        var totalSize = files.sumOf { it.length() }

        if (totalSize <= maxCacheSize) return

        Log.i(TAG, "Cache size ${totalSize / (1024 * 1024)}MB exceeds 100MB, aggressive cleanup")

        // Aggressive cleanup: Remove oldest files until under 50MB
        for (file in files) {
            if (totalSize <= targetSize) break

            val fileSize = file.length()
            if (file.delete()) {
                Log.i(TAG, "Deleted cache file: ${file.name} (${fileSize / (1024 * 1024)}MB)")
                totalSize -= fileSize
            }
        }

        Log.i(TAG, "Cache cleanup complete, new size: ${totalSize / (1024 * 1024)}MB")
    }

    fun getCacheSize(): Long {
        return cacheDir.listFiles()?.sumOf { it.length() } ?: 0L
    }
}