package com.dorflix.app.domain.model

/**
 * Response wrapper for video details API endpoint
 */
data class VideoDetailsResponse(
    val message: String,
    val video: Video
)