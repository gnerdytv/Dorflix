package com.dorflix.app.data.repository

import com.dorflix.app.data.api.ApiService
import com.dorflix.app.data.api.ApiServiceFactory
import com.dorflix.app.domain.model.*
import com.dorflix.app.utils.DeviceInfoProvider
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

/**
 * Repository class for video-related data operations
 */
class VideoRepository {
    
    private val apiService: ApiService = ApiServiceFactory.create()
    
    /**
     * Get video catalog with pagination
     */
    suspend fun getVideoCatalog(
        limit: Int = 20,
        offset: Int = 0,
        sortBy: String = "created_at",
        sortOrder: String = "desc"
    ): Result<VideoCatalogResponse> = withContext(Dispatchers.IO) {
        try {
            val response = apiService.getVideoCatalog(limit, offset, sortBy, sortOrder)
            if (response.isSuccessful) {
                Result.success(response.body()!!)
            } else {
                Result.failure(Exception("Failed to get video catalog: ${response.code()}"))
            }
        } catch (e: Exception) {
            Result.failure(e)
        }
    }
    
    /**
     * Search videos by title
     */
    suspend fun searchVideos(
        searchTerm: String,
        limit: Int = 20,
        offset: Int = 0
    ): Result<VideoSearchResponse> = withContext(Dispatchers.IO) {
        try {
            val response = apiService.searchVideos(searchTerm, limit, offset)
            if (response.isSuccessful) {
                Result.success(response.body()!!)
            } else {
                Result.failure(Exception("Failed to search videos: ${response.code()}"))
            }
        } catch (e: Exception) {
            Result.failure(e)
        }
    }
    
    /**
     * Get video details by ID
     */
    suspend fun getVideoById(videoId: String): Result<Video> = withContext(Dispatchers.IO) {
        try {
            val response = apiService.getVideoById(videoId)
            if (response.isSuccessful) {
                Result.success(response.body()!!)
            } else {
                Result.failure(Exception("Failed to get video: ${response.code()}"))
            }
        } catch (e: Exception) {
            Result.failure(e)
        }
    }
    
    /**
     * Get video stream URL
     */
    suspend fun getVideoStream(videoId: String): Result<String> = withContext(Dispatchers.IO) {
        try {
            val response = apiService.getVideoStream(videoId)
            if (response.isSuccessful) {
                val data = response.body()
                val streamUrl = data?.get("streamUrl") as? String
                if (streamUrl != null) {
                    Result.success(streamUrl)
                } else {
                    Result.failure(Exception("Stream URL not found in response"))
                }
            } else {
                Result.failure(Exception("Failed to get video stream: ${response.code()}"))
            }
        } catch (e: Exception) {
            Result.failure(e)
        }
    }
    
    /**
     * Save watch progress
     */
    suspend fun saveWatchProgress(
        videoId: String,
        progressSeconds: Int,
        totalDuration: Int
    ): Result<WatchProgress> = withContext(Dispatchers.IO) {
        try {
            val response = apiService.saveWatchProgress(
                WatchProgressRequest(videoId, progressSeconds, totalDuration)
            )
            if (response.isSuccessful) {
                Result.success(response.body()!!)
            } else {
                Result.failure(Exception("Failed to save watch progress: ${response.code()}"))
            }
        } catch (e: Exception) {
            Result.failure(e)
        }
    }
    
    /**
     * Get watch progress for a video
     */
    suspend fun getWatchProgress(videoId: String): Result<WatchProgress?> = withContext(Dispatchers.IO) {
        try {
            val response = apiService.getWatchProgress(videoId)
            if (response.isSuccessful) {
                Result.success(response.body())
            } else {
                Result.failure(Exception("Failed to get watch progress: ${response.code()}"))
            }
        } catch (e: Exception) {
            Result.failure(e)
        }
    }
    
    /**
     * Get continue watching videos
     */
    suspend fun getContinueWatching(limit: Int = 10): Result<List<Video>> = withContext(Dispatchers.IO) {
        try {
            val response = apiService.getContinueWatching(limit)
            if (response.isSuccessful) {
                Result.success(response.body() ?: emptyList())
            } else {
                Result.failure(Exception("Failed to get continue watching: ${response.code()}"))
            }
        } catch (e: Exception) {
            Result.failure(e)
        }
    }
    
    /**
     * Like a video
     */
    suspend fun likeVideo(videoId: String): Result<Unit> = withContext(Dispatchers.IO) {
        try {
            val response = apiService.likeVideo(videoId)
            if (response.isSuccessful) {
                Result.success(Unit)
            } else {
                Result.failure(Exception("Failed to like video: ${response.code()}"))
            }
        } catch (e: Exception) {
            Result.failure(e)
        }
    }
    
    /**
     * Unlike a video
     */
    suspend fun unlikeVideo(videoId: String): Result<Unit> = withContext(Dispatchers.IO) {
        try {
            val response = apiService.unlikeVideo(videoId)
            if (response.isSuccessful) {
                Result.success(Unit)
            } else {
                Result.failure(Exception("Failed to unlike video: ${response.code()}"))
            }
        } catch (e: Exception) {
            Result.failure(e)
        }
    }
    
    /**
     * Check if video is liked
     */
    suspend fun isVideoLiked(videoId: String): Result<Boolean> = withContext(Dispatchers.IO) {
        try {
            val response = apiService.isVideoLiked(videoId)
            if (response.isSuccessful) {
                val data = response.body()
                val isLiked = data?.get("isLiked") as? Boolean
                Result.success(isLiked ?: false)
            } else {
                Result.failure(Exception("Failed to check like status: ${response.code()}"))
            }
        } catch (e: Exception) {
            Result.failure(e)
        }
    }
    
    /**
     * Share a video
     */
    suspend fun shareVideo(
        videoId: String,
        sharePlatform: String?
    ): Result<Unit> = withContext(Dispatchers.IO) {
        try {
            val response = apiService.shareVideo(videoId, ShareRequest(sharePlatform))
            if (response.isSuccessful) {
                Result.success(Unit)
            } else {
                Result.failure(Exception("Failed to share video: ${response.code()}"))
            }
        } catch (e: Exception) {
            Result.failure(e)
        }
    }
    
    /**
     * Get video comments
     */
    suspend fun getComments(
        videoId: String,
        limit: Int = 20,
        offset: Int = 0
    ): Result<CommentsResponse> = withContext(Dispatchers.IO) {
        try {
            val response = apiService.getComments(videoId, limit, offset)
            if (response.isSuccessful) {
                Result.success(response.body()!!)
            } else {
                Result.failure(Exception("Failed to get comments: ${response.code()}"))
            }
        } catch (e: Exception) {
            Result.failure(e)
        }
    }
    
    /**
     * Add comment to video
     */
    suspend fun addComment(
        videoId: String,
        commentText: String
    ): Result<Comment> = withContext(Dispatchers.IO) {
        try {
            val response = apiService.addComment(videoId, CommentRequest(commentText))
            if (response.isSuccessful) {
                Result.success(response.body()!!.comment)
            } else {
                Result.failure(Exception("Failed to add comment: ${response.code()}"))
            }
        } catch (e: Exception) {
            Result.failure(e)
        }
    }
    
    /**
     * Delete comment
     */
    suspend fun deleteComment(commentId: String): Result<Unit> = withContext(Dispatchers.IO) {
        try {
            val response = apiService.deleteComment(commentId)
            if (response.isSuccessful) {
                Result.success(Unit)
            } else {
                Result.failure(Exception("Failed to delete comment: ${response.code()}"))
            }
        } catch (e: Exception) {
            Result.failure(e)
        }
    }
    
    /**
     * Get video recommendations
     */
    suspend fun getRecommendations(limit: Int = 10): Result<List<Video>> = withContext(Dispatchers.IO) {
        try {
            val response = apiService.getRecommendations(limit)
            if (response.isSuccessful) {
                Result.success(response.body() ?: emptyList())
            } else {
                Result.failure(Exception("Failed to get recommendations: ${response.code()}"))
            }
        } catch (e: Exception) {
            Result.failure(e)
        }
    }
    
    /**
     * Get watch statistics
     */
    suspend fun getWatchStatistics(): Result<VideoStatistics> = withContext(Dispatchers.IO) {
        try {
            val response = apiService.getWatchStatistics()
            if (response.isSuccessful) {
                Result.success(response.body()!!)
            } else {
                Result.failure(Exception("Failed to get watch statistics: ${response.code()}"))
            }
        } catch (e: Exception) {
            Result.failure(e)
        }
    }
    
    /**
     * Get social activity
     */
    suspend fun getSocialActivity(): Result<SocialActivityResponse> = withContext(Dispatchers.IO) {
        try {
            val response = apiService.getSocialActivity()
            if (response.isSuccessful) {
                Result.success(response.body()!!)
            } else {
                Result.failure(Exception("Failed to get social activity: ${response.code()}"))
            }
        } catch (e: Exception) {
            Result.failure(e)
        }
    }
}
