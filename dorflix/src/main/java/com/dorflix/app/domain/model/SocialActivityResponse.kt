package com.dorflix.app.domain.model

import java.util.*

/**
 * Response model for social activity
 */
data class SocialActivityResponse(
    val likesCount: Int,
    val sharesCount: Int,
    val commentsCount: Int,
    val recentActivity: List<SocialActivity>
)

/**
 * Social activity data model
 */
data class SocialActivity(
    val id: String,
    val type: String, // "like", "share", "comment"
    val videoId: String,
    val videoTitle: String,
    val userId: String,
    val userName: String,
    val createdAt: Date
)
