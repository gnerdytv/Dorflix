package com.dorflix.app.presentation.viewmodel

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.dorflix.app.data.repository.VideoRepository
import com.dorflix.app.domain.model.*
import kotlinx.coroutines.flow.*
import kotlinx.coroutines.launch

/**
 * ViewModel for video-related UI operations
 */
class VideoViewModel : ViewModel() {

    private val repository = VideoRepository()
    private var lastMonitorTime = 0L
    var autoPlayAttempted = false
    
    // State flows for UI
    private val _videoCatalog = MutableStateFlow<List<Video>>(emptyList())
    val videoCatalog: StateFlow<List<Video>> = _videoCatalog.asStateFlow()
    
    private val _searchResults = MutableStateFlow<List<Video>>(emptyList())
    val searchResults: StateFlow<List<Video>> = _searchResults.asStateFlow()
    
    private val _currentVideo = MutableStateFlow<Video?>(null)
    val currentVideo: StateFlow<Video?> = _currentVideo.asStateFlow()
    
    private val _watchProgress = MutableStateFlow<WatchProgress?>(null)
    val watchProgress: StateFlow<WatchProgress?> = _watchProgress.asStateFlow()
    
    private val _continueWatching = MutableStateFlow<List<Video>>(emptyList())
    val continueWatching: StateFlow<List<Video>> = _continueWatching.asStateFlow()
    
    private val _recommendations = MutableStateFlow<List<Video>>(emptyList())
    val recommendations: StateFlow<List<Video>> = _recommendations.asStateFlow()
    
    private val _watchStatistics = MutableStateFlow<VideoStatistics?>(null)
    val watchStatistics: StateFlow<VideoStatistics?> = _watchStatistics.asStateFlow()
    
    private val _socialActivity = MutableStateFlow<SocialActivityResponse?>(null)
    val socialActivity: StateFlow<SocialActivityResponse?> = _socialActivity.asStateFlow()
    
    private val _isLoading = MutableStateFlow(false)
    val isLoading: StateFlow<Boolean> = _isLoading.asStateFlow()
    
    private val _error = MutableStateFlow<String?>(null)
    val error: StateFlow<String?> = _error.asStateFlow()
    
    // Aggregate UI State
    val uiState: StateFlow<VideoUiState> = combine(
        _isLoading, _error, _currentVideo, _watchProgress
    ) { loading, err, video, progress ->
        VideoUiState(loading, err, video, progress)
    }.stateIn(
        scope = viewModelScope,
        started = SharingStarted.WhileSubscribed(5000),
        initialValue = VideoUiState()
    )
    
    /**
     * Load initial video catalog
     */
    fun loadInitialCatalog() {
        if (_videoCatalog.value.isEmpty()) {
            loadVideoCatalog()
        }
    }
    
    /**
     * Load video catalog with pagination
     */
    fun loadVideoCatalog(offset: Int = 0) {
        viewModelScope.launch {
            _isLoading.value = true
            _error.value = null
            
            val result = repository.getVideoCatalog(limit = 20, offset = offset)
            
            result.onSuccess { response ->
                if (offset == 0) {
                    _videoCatalog.value = response.videos
                } else {
                    _videoCatalog.value = _videoCatalog.value + response.videos
                }
            }.onFailure { exception ->
                _error.value = exception.message ?: "Failed to load video catalog"
            }
            
            _isLoading.value = false
        }
    }
    
    /**
     * Search videos
     */
    fun searchVideos(query: String) {
        if (query.isBlank()) {
            _searchResults.value = emptyList()
            return
        }
        
        viewModelScope.launch {
            _isLoading.value = true
            _error.value = null
            
            val result = repository.searchVideos(query, limit = 50)
            
            result.onSuccess { response ->
                _searchResults.value = response.videos
            }.onFailure { exception ->
                _error.value = exception.message ?: "Failed to search videos"
            }
            
            _isLoading.value = false
        }
    }
    
    /**
     * Load video details
     */
    fun loadVideoDetails(videoId: String) {
        android.util.Log.i("VideoViewModel", "=== VideoViewModel.loadVideoDetails() CALLED ===")
        android.util.Log.i("VideoViewModel", "Input videoId: '$videoId' (length: ${videoId.length})")
        android.util.Log.i("VideoViewModel", "Current thread: ${Thread.currentThread().name} (${Thread.currentThread().id})")
        android.util.Log.i("VideoViewModel", "ViewModel instance: ${this.hashCode()}")

        viewModelScope.launch {
            android.util.Log.i("VideoViewModel", "=== COROUTINE LAUNCHED FOR VIDEO DETAILS ===")
            android.util.Log.i("VideoViewModel", "Setting isLoading = true")
            _isLoading.value = true
            _error.value = null

            android.util.Log.i("VideoViewModel", "Calling repository.getVideoById('$videoId')")
            val startTime = System.currentTimeMillis()
            val result = repository.getVideoById(videoId)
            val apiCallTime = System.currentTimeMillis() - startTime

            android.util.Log.i("VideoViewModel", "Repository call completed in ${apiCallTime}ms")

            result.onSuccess { video ->
                android.util.Log.i("VideoViewModel", "=== REPOSITORY CALL SUCCESS ===")
                android.util.Log.i("VideoViewModel", "Video object received: ${video.id}")
                android.util.Log.i("VideoViewModel", "Video title: ${video.title}")
                android.util.Log.i("VideoViewModel", "Raw video URL: '${video.video_url}'")

                android.util.Log.i("VideoViewModel", "Setting _currentVideo.value = video")
                _currentVideo.value = video

                android.util.Log.i("VideoViewModel", "Calling loadWatchProgress('$videoId')")
                loadWatchProgress(videoId)

                android.util.Log.i("VideoViewModel", "=== VIDEO DETAILS LOAD SUCCESS ===")
            }.onFailure { exception ->
                android.util.Log.e("VideoViewModel", "=== REPOSITORY CALL FAILED ===")
                android.util.Log.e("VideoViewModel", "Exception: ${exception.javaClass.simpleName}: ${exception.message}")
                android.util.Log.e("VideoViewModel", "Stack trace:", exception)
                _error.value = exception.message ?: "Failed to load video details"
                android.util.Log.e("VideoViewModel", "Set error message: '${_error.value}'")
            }

            android.util.Log.i("VideoViewModel", "Setting isLoading = false")
            _isLoading.value = false
            android.util.Log.i("VideoViewModel", "=== VideoViewModel.loadVideoDetails() COMPLETED ===")
        }
    }

    /**
     * Monitor video URL availability and format - SILENT METHOD
     * Returns monitoring results without any logging to prevent flow restarts
     */
    private fun monitorVideoUrl(videoId: String, videoUrl: String): VideoUrlStatus {
        val isUrlPresent = videoUrl.isNotBlank()
        val isMp4Format = videoUrl.lowercase().endsWith(".mp4")
        val urlLength = videoUrl.length

        return VideoUrlStatus(
            videoId = videoId,
            isUrlPresent = isUrlPresent,
            isMp4Format = isMp4Format,
            urlLength = urlLength,
            hasValidProtocol = videoUrl.startsWith("http://") || videoUrl.startsWith("https://") || videoUrl.startsWith("file://")
        )
    }

    /**
     * Log video URL monitoring results outside the flow emission
     */
    private fun logVideoUrlStatus(status: VideoUrlStatus) {
        android.util.Log.i("VideoViewModel", "=== VIDEO URL STATUS ===")
        android.util.Log.i("VideoViewModel", "Video ID: ${status.videoId}")
        android.util.Log.i("VideoViewModel", "URL present: ${status.isUrlPresent}")
        android.util.Log.i("VideoViewModel", "Length: ${status.urlLength}")
        android.util.Log.i("VideoViewModel", "MP4 format: ${status.isMp4Format}")
        android.util.Log.i("VideoViewModel", "Valid protocol: ${status.hasValidProtocol}")

        if (!status.isUrlPresent) {
            android.util.Log.e("VideoViewModel", "❌ CRITICAL: Video URL is missing!")
        } else if (!status.isMp4Format) {
            android.util.Log.w("VideoViewModel", "⚠️ WARNING: Video URL is not MP4 format")
        } else if (!status.hasValidProtocol) {
            android.util.Log.w("VideoViewModel", "⚠️ WARNING: Video URL has invalid protocol")
        } else {
            android.util.Log.i("VideoViewModel", "✅ Video URL validation passed")
        }
    }

    /**
     * Data class for video URL monitoring results
     */
    private data class VideoUrlStatus(
        val videoId: String,
        val isUrlPresent: Boolean,
        val isMp4Format: Boolean,
        val urlLength: Int,
        val hasValidProtocol: Boolean
    )
    
    /**
     * Load watch progress for current video
     */
    private fun loadWatchProgress(videoId: String) {
        viewModelScope.launch {
            val result = repository.getWatchProgress(videoId)
            result.onSuccess { progress ->
                _watchProgress.value = progress
            }
        }
    }
    
    /**
     * Save watch progress
     */
    fun saveWatchProgress(videoId: String, progressSeconds: Int, totalDuration: Int) {
        viewModelScope.launch {
            val result = repository.saveWatchProgress(videoId, progressSeconds, totalDuration)
            result.onSuccess { progress ->
                _watchProgress.value = progress
            }.onFailure { exception ->
                _error.value = exception.message ?: "Failed to save watch progress"
            }
        }
    }
    
    /**
     * Load continue watching videos
     */
    fun loadContinueWatching() {
        viewModelScope.launch {
            _isLoading.value = true
            _error.value = null
            
            val result = repository.getContinueWatching(limit = 20)
            result.onSuccess { videos ->
                _continueWatching.value = videos
            }.onFailure { exception ->
                _error.value = exception.message ?: "Failed to load continue watching"
            }
            
            _isLoading.value = false
        }
    }
    
    /**
     * Load video recommendations
     */
    fun loadRecommendations() {
        viewModelScope.launch {
            _isLoading.value = true
            _error.value = null
            
            val result = repository.getRecommendations(limit = 20)
            result.onSuccess { videos ->
                _recommendations.value = videos
            }.onFailure { exception ->
                _error.value = exception.message ?: "Failed to load recommendations"
            }
            
            _isLoading.value = false
        }
    }
    
    /**
     * Like a video
     */
    fun likeVideo(videoId: String) {
        viewModelScope.launch {
            val result = repository.likeVideo(videoId)
            result.onSuccess {
                _currentVideo.value = _currentVideo.value?.copy(
                    is_liked = true,
                    likes_count = (_currentVideo.value?.likes_count ?: 0) + 1
                )
            }.onFailure { exception ->
                _error.value = exception.message ?: "Failed to like video"
            }
        }
    }
    
    /**
     * Unlike a video
     */
    fun unlikeVideo(videoId: String) {
        viewModelScope.launch {
            val result = repository.unlikeVideo(videoId)
            result.onSuccess {
                _currentVideo.value = _currentVideo.value?.copy(
                    is_liked = false,
                    likes_count = (_currentVideo.value?.likes_count ?: 0) - 1
                )
            }.onFailure { exception ->
                _error.value = exception.message ?: "Failed to unlike video"
            }
        }
    }
    
    /**
     * Share a video
     */
    fun shareVideo(videoId: String, platform: String? = null) {
        viewModelScope.launch {
            val result = repository.shareVideo(videoId, platform)
            if (!result.isSuccess) {
                _error.value = result.exceptionOrNull()?.message ?: "Failed to share video"
            }
        }
    }
    
    /**
     * Add comment to video
     */
    fun addComment(videoId: String, commentText: String) {
        viewModelScope.launch {
            val result = repository.addComment(videoId, commentText)
            result.onSuccess {
                _currentVideo.value = _currentVideo.value?.copy(
                    comments_count = (_currentVideo.value?.comments_count ?: 0) + 1
                )
            }.onFailure { exception ->
                _error.value = exception.message ?: "Failed to add comment"
            }
        }
    }
    
    /**
     * Clear error state
     */
    fun clearError() {
        _error.value = null
    }
    
    /**
     * Load watch statistics
     */
    fun loadWatchStatistics() {
        viewModelScope.launch {
            _isLoading.value = true
            _error.value = null
            
            val result = repository.getWatchStatistics()
            result.onSuccess { statistics ->
                _watchStatistics.value = statistics
            }.onFailure { exception ->
                _error.value = exception.message ?: "Failed to load watch statistics"
            }
            
            _isLoading.value = false
        }
    }
    
    /**
     * Get social activity
     */
    fun getSocialActivity() {
        viewModelScope.launch {
            _isLoading.value = true
            _error.value = null
            
            val result = repository.getSocialActivity()
            result.onSuccess { activity ->
                _socialActivity.value = activity
            }.onFailure { exception ->
                _error.value = exception.message ?: "Failed to get social activity"
            }
            
            _isLoading.value = false
        }
    }
    
    /**
     * Reset search results
     */
    fun clearSearch() {
        _searchResults.value = emptyList()
    }
}

/**
 * UI State for video operations
 */
data class VideoUiState(
    val isLoading: Boolean = false,
    val error: String? = null,
    val currentVideo: Video? = null,
    val watchProgress: WatchProgress? = null
)