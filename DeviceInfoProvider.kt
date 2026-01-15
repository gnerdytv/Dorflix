package com.dorflix.app.utils

import android.content.Context
import android.os.Build
import android.provider.Settings
import java.util.*

/**
 * Utility class to provide device information for API requests
 */
object DeviceInfoProvider {
    
    /**
     * Generate a unique device ID for the application
     */
    fun getDeviceId(context: Context): String {
        val androidId = Settings.Secure.getString(
            context.contentResolver,
            Settings.Secure.ANDROID_ID
        )
        
        // Combine Android ID with package name for uniqueness
        val packageName = context.packageName
        return "${packageName}_${androidId}"
    }
    
    /**
     * Get device name (model)
     */
    fun getDeviceName(): String {
        val manufacturer = Build.MANUFACTURER
        val model = Build.MODEL
        
        return if (model.startsWith(manufacturer)) {
            model
        } else {
            "$manufacturer $model"
        }
    }
    
    /**
     * Get device type
     */
    fun getDeviceType(): String {
        return "android"
    }
    
    /**
     * Get device model
     */
    fun getDeviceModel(): String {
        return Build.MODEL
    }
    
    /**
     * Get OS version
     */
    fun getOSVersion(): String {
        return Build.VERSION.RELEASE
    }
    
    /**
     * Get app version
     */
    fun getAppVersion(context: Context): String {
        return try {
            val packageInfo = context.packageManager.getPackageInfo(context.packageName, 0)
            packageInfo.versionName ?: "1.0.0"
        } catch (e: Exception) {
            "1.0.0"
        }
    }
    
    /**
     * Get all device information as a map
     */
    fun getDeviceData(context: Context): Map<String, String> {
        return mapOf(
            "deviceId" to getDeviceId(context),
            "deviceName" to getDeviceName(),
            "deviceType" to getDeviceType(),
            "deviceModel" to getDeviceModel(),
            "osVersion" to getOSVersion(),
            "appVersion" to getAppVersion(context)
        )
    }
}
