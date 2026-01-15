package com.dorflix.app.utils

import android.content.Context
import android.net.ConnectivityManager
import android.net.NetworkCapabilities
import android.util.Log
import com.dorflix.app.DorflixApplication

private const val TAG = "NetworkUtils"

object NetworkUtils {
    /**
     * Safely checks if network is available with enhanced context validation
     * @param context The context to use, can be null
     * @return true if network is available, false otherwise
     */
    fun isNetworkAvailable(context: Context?): Boolean {
        // First try to get a valid context
        val validContext = getValidContext(context)
        if (validContext == null) {
            Log.w(TAG, "No valid context available for network check")
            return false
        }
        
        return try {
            val connectivityManager = validContext.getSystemService(Context.CONNECTIVITY_SERVICE) as? ConnectivityManager
                ?: return false
            
            val network = connectivityManager.activeNetwork ?: return false
            val capabilities = connectivityManager.getNetworkCapabilities(network) ?: return false
            
            capabilities.hasCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET) &&
                   capabilities.hasCapability(NetworkCapabilities.NET_CAPABILITY_VALIDATED)
        } catch (e: SecurityException) {
            // Handle permission issues
            Log.w(TAG, "Security exception when checking network availability", e)
            false
        } catch (e: Exception) {
            Log.e(TAG, "Error checking network availability", e)
            false
        }
    }
    
    /**
     * Safely gets network type with enhanced context validation
     * @param context The context to use, can be null
     * @return network type string or "Unknown" if unavailable
     */
    fun getNetworkType(context: Context?): String {
        // First try to get a valid context
        val validContext = getValidContext(context)
        if (validContext == null) {
            Log.w(TAG, "No valid context available for network type check")
            return "Unknown"
        }
        
        return try {
            val connectivityManager = validContext.getSystemService(Context.CONNECTIVITY_SERVICE) as? ConnectivityManager
                ?: return "Unknown"
            
            val network = connectivityManager.activeNetwork ?: return "Unknown"
            val capabilities = connectivityManager.getNetworkCapabilities(network) ?: return "Unknown"
            
            when {
                capabilities.hasTransport(NetworkCapabilities.TRANSPORT_WIFI) -> "WiFi"
                capabilities.hasTransport(NetworkCapabilities.TRANSPORT_CELLULAR) -> "Cellular"
                capabilities.hasTransport(NetworkCapabilities.TRANSPORT_ETHERNET) -> "Ethernet"
                else -> "Unknown"
            }
        } catch (e: SecurityException) {
            // Handle permission issues
            Log.w(TAG, "Security exception when getting network type", e)
            "Unknown"
        } catch (e: Exception) {
            Log.e(TAG, "Error getting network type", e)
            "Unknown"
        }
    }
    
    /**
     * Gets a valid context for network operations
     * @param context The provided context
     * @return A valid context or null if none available
     */
    private fun getValidContext(context: Context?): Context? {
        // If we have a valid context, use it
        if (context != null && isContextValid(context)) {
            return context
        }
        
        // Try to get application context as fallback
        return try {
            DorflixApplication.instance?.applicationContext
        } catch (e: Exception) {
            Log.w(TAG, "Cannot access application context", e)
            null
        }
    }
    
    /**
     * Validates if a context is still usable
     * @param context The context to validate
     * @return true if context is valid, false otherwise
     */
    private fun isContextValid(context: Context): Boolean {
        return try {
            // Try to access a basic property to validate context
            context.packageName.isNotEmpty()
        } catch (e: Exception) {
            Log.w(TAG, "Context validation failed", e)
            false
        }
    }
}
