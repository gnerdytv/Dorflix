package com.dorflix.app.utils

import java.text.SimpleDateFormat
import java.util.*

object DateUtils {
    private val dateFormat = SimpleDateFormat("yyyy-MM-dd HH:mm:ss", Locale.getDefault())
    private val relativeDateFormat = SimpleDateFormat("MMM d, yyyy", Locale.getDefault())
    
    fun formatDateTime(date: Date): String {
        return dateFormat.format(date)
    }
    
    fun formatRelativeDate(date: Date): String {
        val now = Date()
        val diff = now.time - date.time
        
        return when {
            diff < 60000 -> "Just now"
            diff < 3600000 -> "${diff / 60000} minutes ago"
            diff < 86400000 -> "${diff / 3600000} hours ago"
            diff < 604800000 -> "${diff / 86400000} days ago"
            else -> relativeDateFormat.format(date)
        }
    }
    
    fun formatDuration(seconds: Int): String {
        val minutes = seconds / 60
        val remainingSeconds = seconds % 60
        return String.format("%d:%02d", minutes, remainingSeconds)
    }
    
    fun formatViewCount(count: Int): String {
        return when {
            count >= 1000000 -> "${count / 1000000}M views"
            count >= 1000 -> "${count / 1000}K views"
            count == 1 -> "1 view"
            else -> "$count views"
        }
    }
}
