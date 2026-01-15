package com.dorflix.app.data.api

import com.dorflix.app.domain.model.*
import retrofit2.Response
import retrofit2.http.*

/**
 * Retrofit API service interface for Dorflix backend
 */
interface ApiService {
    
    // Health check
    @GET("/health")
    suspend fun healthCheck(): Response<Map<String, Any>>
    
    // Device management
    @POST("/api/devices/register")
    suspend fun registerDevice(@Body deviceData: DeviceRegistrationRequest): Response<DeviceRegistrationResponse>
    
    @POST("/api/devices/validate-session")
    suspend fun validateSession(@Header("Authorization") token: String): Response<Map<String, Any>>
    
    // Video catalog
    @GET("/api/videos")
    suspend fun getVideoCatalog(
        @Query("limit") limit: Int = 20,
        @Query("offset") offset: Int = 0,
        @Query("sortBy") sortBy: String = "created_at",
        @Query("sortOrder") sortOrder: String = "desc"
    ): Response<VideoCatalogResponse>
    
    @GET("/api/videos/search")
    suspend fun searchVideos(
        @Query("q") searchTerm: String,
        @Query("limit") limit: Int = 20,
        @Query("offset") offset: Int = 0
    ): Response<VideoSearchResponse>
    
    @GET("/api/videos/{id}")
    suspend fun getVideoById(@Path("id") videoId: String): Response<Video>
    
    @GET("/api/videos/{id}/stream")
    suspend fun getVideoStream(@Path("id") videoId: String): Response<Map<String, Any>>
    
    // Watch progress
    @POST("/api/watch-progress")
    suspend fun saveWatchProgress(@Body progressData: WatchProgressRequest): Response<WatchProgress>
    
    @GET("/api/watch-progress/{videoId}")
    suspend fun getWatchProgress(@Path("videoId") videoId: String): Response<WatchProgress?>
    
    @GET("/api/continue-watching")
    suspend fun getContinueWatching(@Query("limit") limit: Int = 10): Response<List<Video>>
    
    // Social features
    @POST("/api/videos/{id}/like")
    suspend fun likeVideo(@Path("id") videoId: String): Response<Map<String, Any>>
    
    @DELETE("/api/videos/{id}/like")
    suspend fun unlikeVideo(@Path("id") videoId: String): Response<Map<String, Any>>
    
    @GET("/api/videos/{id}/is-liked")
    suspend fun isVideoLiked(@Path("id") videoId: String): Response<Map<String, Boolean>>
    
    @POST("/api/videos/{id}/share")
    suspend fun shareVideo(
        @Path("id") videoId: String,
        @Body shareData: ShareRequest
    ): Response<Map<String, Any>>
    
    @GET("/api/videos/{id}/comments")
    suspend fun getComments(
        @Path("id") videoId: String,
        @Query("limit") limit: Int = 20,
        @Query("offset") offset: Int = 0
    ): Response<CommentsResponse>
    
    @POST("/api/videos/{id}/comments")
    suspend fun addComment(
        @Path("id") videoId: String,
        @Body commentData: CommentRequest
    ): Response<CommentResponse>
    
    @DELETE("/api/comments/{id}")
    suspend fun deleteComment(@Path("id") commentId: String): Response<Map<String, Any>>
    
    // Recommendations and analytics
    @GET("/api/recommendations")
    suspend fun getRecommendations(@Query("limit") limit: Int = 10): Response<List<Video>>
    
    @GET("/api/stats/watch")
    suspend fun getWatchStatistics(): Response<VideoStatistics>
    
    @GET("/api/stats/social")
    suspend fun getSocialActivity(): Response<SocialActivityResponse>
}

/**
 * Request data classes
 */
data class DeviceRegistrationRequest(
    val deviceId: String,
    val deviceName: String?,
    val deviceType: String?,
    val deviceModel: String?,
    val osVersion: String?,
    val appVersion: String?
)

data class WatchProgressRequest(
    val videoId: String,
    val progressSeconds: Int,
    val totalDuration: Int
)

data class ShareRequest(
    val sharePlatform: String?
)

data class CommentRequest(
    val commentText: String
)

/**
 * Response data classes
 */
data class CommentsResponse(
    val comments: List<Comment>,
    val total: Int,
    val hasMore: Boolean
)

data class CommentResponse(
    val comment: Comment
)

data class Comment(
    val id: String,
    val videoId: String,
    val deviceId: String,
    val commentText: String,
    val createdAt: String
)

data class SocialActivityResponse(
    val totalLikes: Int,
    val totalShares: Int,
    val totalComments: Int,
    val recentActivity: List<SocialActivity>
)

data class SocialActivity(
    val type: String,
    val videoId: String,
    val videoTitle: String,
    val timestamp: String
)
