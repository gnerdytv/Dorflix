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
    private val deviceInfoProvider = DeviceInfoProvider
    
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
                val catalogResponse = response.body()!!
                // Debug logging for JSON parsing issues
                android.util.Log.i("VideoRepository", "=== VIDEO CATALOG RESPONSE DEBUG ===")
                android.util.Log.i("VideoRepository", "Total videos: ${catalogResponse.videos.size}")
                catalogResponse.videos.forEachIndexed { index, video ->
                    try {
                        val hasUrl = video.video_url.isNotBlank()
                        android.util.Log.i("VideoRepository", "Video[$index]: id=${video.id}, title=${video.title}, video_url=${video.video_url}, hasUrl=$hasUrl")
                    } catch (e: Exception) {
                        android.util.Log.e("VideoRepository", "Error logging video[$index]: ${e.message}")
                        android.util.Log.e("VideoRepository", "Video object: $video")
                    }
                }
                android.util.Log.i("VideoRepository", "=== END VIDEO CATALOG RESPONSE DEBUG ===")
                Result.success(catalogResponse)
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
            android.util.Log.i("VideoRepository", "=== VideoRepository.getVideoById() START ===")
            android.util.Log.i("VideoRepository", "Input videoId: '$videoId' (length: ${videoId.length})")
            android.util.Log.i("VideoRepository", "Current thread: ${Thread.currentThread().name} (${Thread.currentThread().id})")
            android.util.Log.i("VideoRepository", "API service instance: ${apiService.hashCode()}")

            android.util.Log.i("VideoRepository", "Making API call to getVideoById('$videoId')")
            val startTime = System.currentTimeMillis()
            val response = apiService.getVideoById(videoId)
            val apiCallTime = System.currentTimeMillis() - startTime

            android.util.Log.i("VideoRepository", "API call completed in ${apiCallTime}ms")
            android.util.Log.i("VideoRepository", "Response received: isSuccessful=${response.isSuccessful()}, code=${response.code()}, message='${response.message()}'")
            android.util.Log.i("VideoRepository", "Response headers: ${response.headers()}")

            if (response.isSuccessful) {
                android.util.Log.i("VideoRepository", "=== API RESPONSE SUCCESSFUL ===")
                val responseWrapper = response.body()
                android.util.Log.i("VideoRepository", "Raw response body: $responseWrapper")

                if (responseWrapper != null) {
                    android.util.Log.i("VideoRepository", "=== RESPONSE WRAPPER PARSED SUCCESSFULLY ===")
                    android.util.Log.i("VideoRepository", "Response message: ${responseWrapper.message}")

                    val video = responseWrapper.video
                    android.util.Log.i("VideoRepository", "Extracted video object: $video")

                    if (video != null) {
                        android.util.Log.i("VideoRepository", "=== VIDEO OBJECT PARSED SUCCESSFULLY ===")
                        android.util.Log.i("VideoRepository", "Video ID: ${video.id}")
                        android.util.Log.i("VideoRepository", "Video title: ${video.title}")
                        android.util.Log.i("VideoRepository", "Video URL: '${video.video_url}'")
                        android.util.Log.i("VideoRepository", "Video URL length: ${video.video_url.length}")
                        android.util.Log.i("VideoRepository", "Video URL isNullOrBlank: ${video.video_url.isNullOrBlank()}")
                        android.util.Log.i("VideoRepository", "Video URL starts with http: ${video.video_url.startsWith("http")}")
                        android.util.Log.i("VideoRepository", "Video URL ends with .mp4: ${video.video_url.lowercase().endsWith(".mp4")}")

                        // Additional video object inspection
                        android.util.Log.i("VideoRepository", "Video uploader: ${video.uploader_name}")
                        android.util.Log.i("VideoRepository", "Video duration: ${video.duration}")
                        android.util.Log.i("VideoRepository", "Video likes: ${video.likes_count}")
                        android.util.Log.i("VideoRepository", "Video comments: ${video.comments_count}")

                        android.util.Log.i("VideoRepository", "=== RETURNING SUCCESS RESULT ===")
                        Result.success(video)
                    } else {
                        android.util.Log.e("VideoRepository", "=== VIDEO OBJECT IS NULL ===")
                        android.util.Log.e("VideoRepository", "Response wrapper parsed but video field is null")
                        Result.failure(Exception("Server returned null video object"))
                    }
                } else {
                    android.util.Log.e("VideoRepository", "=== RESPONSE BODY IS NULL ===")
                    android.util.Log.e("VideoRepository", "Response was successful but body() returned null")
                    android.util.Log.e("VideoRepository", "This indicates a JSON parsing error or empty response")
                    Result.failure(Exception("Server returned empty response"))
                }
            } else {
                android.util.Log.e("VideoRepository", "=== API RESPONSE FAILED ===")
                android.util.Log.e("VideoRepository", "HTTP Status: ${response.code()} ${response.message()}")
                android.util.Log.e("VideoRepository", "Error body: ${response.errorBody()?.string()}")

                // Check for common HTTP errors
                when (response.code()) {
                    404 -> android.util.Log.e("VideoRepository", "Video not found (404)")
                    500 -> android.util.Log.e("VideoRepository", "Server error (500)")
                    403 -> android.util.Log.e("VideoRepository", "Access forbidden (403)")
                    401 -> android.util.Log.e("VideoRepository", "Unauthorized (401)")
                    else -> android.util.Log.e("VideoRepository", "Other HTTP error: ${response.code()}")
                }

                Result.failure(Exception("Failed to get video: ${response.code()} - ${response.message()}"))
            }
        } catch (e: Exception) {
            android.util.Log.e("VideoRepository", "=== EXCEPTION IN getVideoById ===")
            android.util.Log.e("VideoRepository", "Exception type: ${e.javaClass.simpleName}")
            android.util.Log.e("VideoRepository", "Exception message: ${e.message}")
            android.util.Log.e("VideoRepository", "Stack trace:", e)
            android.util.Log.e("VideoRepository", "Cause: ${e.cause}")

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
                val isLiked = data?.get("isLiked") as? Boolean ?: false
                Result.success(isLiked)
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