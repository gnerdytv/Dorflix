package com.dorflix.app.video

// Download listener interface for progress tracking
interface DownloadListener {
    fun onDownloadProgress(url: String, downloaded: Long, total: Long)
    fun onDownloadComplete(url: String, localPath: String)
    fun onDownloadError(url: String, error: String)
}