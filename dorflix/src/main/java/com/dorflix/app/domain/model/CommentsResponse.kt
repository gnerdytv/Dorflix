package com.dorflix.app.domain.model

/**
 * Response model for video comments
 */
data class CommentsResponse(
    val comments: List<Comment>,
    val total: Int,
    val hasMore: Boolean
)
