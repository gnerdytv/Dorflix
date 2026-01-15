package com.dorflix.app.domain.model

/**
 * Request model for saving watch progress
 */
data class WatchProgressRequest(
    val videoId: String,
    val progressSeconds: Int,
    val totalDuration: Int
)
