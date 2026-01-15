package com.dorflix.app.domain.model

import java.util.*

/**
 * Comment data model
 */
data class Comment(
    val id: String,
    val videoId: String,
    val userId: String,
    val userName: String,
    val userAvatarUrl: String?,
    val text: String,
    val likesCount: Int,
    val createdAt: Date,
    val updatedAt: Date
)
