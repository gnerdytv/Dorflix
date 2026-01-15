package com.dorflix.app.domain.model

import java.util.*

/**
 * Video data model representing a video from the backend API
 * Field names match backend snake_case JSON response
 */
data class Video(
    val id: String,
    val title: String,
    val description: String?,
    val video_url: String,
    val thumbnail_url: String?,
    val duration: Int, // in seconds
    val uploader_name: String,
    val uploader_avatar_url: String?,
    val likes_count: Int,
    val comments_count: Int,
    val shares_count: Int,
    val views_count: Int,
    val is_active: Boolean,
    val created_at: Date,
    val updated_at: Date,

    // Optional fields for device-specific data
    val is_liked: Boolean = false,
    val is_completed: Boolean = false,
    val current_progress: Int? = null
)

/**
 * Video catalog response from API
 */
data class VideoCatalogResponse(
    val videos: List<Video>,
    val total: Int,
    val hasMore: Boolean
)

/**
 * Video search response from API
 */
data class VideoSearchResponse(
    val searchTerm: String,
    val videos: List<Video>,
    val total: Int,
    val hasMore: Boolean
)

/**
 * Watch progress data model
 */
data class WatchProgress(
    val id: String,
    val videoId: String,
    val deviceId: String,
    val progressSeconds: Int,
    val totalDuration: Int,
    val percentageWatched: Int,
    val isCompleted: Boolean,
    val lastWatchedAt: Date,
    val createdAt: Date,
    val updatedAt: Date
)

/**
 * Device session data model
 */
data class DeviceSession(
    val deviceId: String,
    val sessionToken: String,
    val expiresAt: Date
)

/**
 * Device registration response
 */
data class DeviceRegistrationResponse(
    val message: String,
    val device: DeviceInfo,
    val session: DeviceSession
)

/**
 * Device information
 */
data class DeviceInfo(
    val id: String,
    val deviceId: String,
    val deviceName: String?,
    val deviceType: String?,
    val deviceModel: String?,
    val osVersion: String?,
    val appVersion: String?
)

/**
 * Social action types
 */
enum class SocialAction {
    LIKE,
    UNLIKE,
    SHARE,
    COMMENT
}

/**
 * Video statistics for analytics
 */
data class VideoStatistics(
    val totalVideosWatched: Int,
    val totalWatchTime: Int, // in seconds
    val completedVideos: Int,
    val averageWatchTime: Int // in seconds
)

/**
 * Video recommendation data
 */
data class VideoRecommendation(
    val video: Video,
    val reason: String,
    val score: Double
)
