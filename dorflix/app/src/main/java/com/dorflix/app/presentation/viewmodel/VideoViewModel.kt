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
        viewModelScope.launch {
            _isLoading.value = true
            _error.value = null
            
            val result = repository.getVideoById(videoId)
            
            result.onSuccess { video ->
                _currentVideo.value = video
                loadWatchProgress(videoId)
            }.onFailure { exception ->
                _error.value = exception.message ?: "Failed to load video details"
            }
            
            _isLoading.value = false
        }
    }
    
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
                    isLiked = true,
                    likesCount = (_currentVideo.value?.likesCount ?: 0) + 1
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
                    isLiked = false,
                    likesCount = (_currentVideo.value?.likesCount ?: 0) - 1
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
                    commentsCount = (_currentVideo.value?.commentsCount ?: 0) + 1
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
     * Clear search results
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
