package com.dorflix.app.utils

import android.content.Context
import android.os.Build
import java.util.*

/**
 * Provider for device-specific information
 */
class DeviceInfoProvider {
    
    fun getDeviceId(context: Context): String {
        // Simple implementation for demonstration
        return UUID.randomUUID().toString()
    }
    
    fun getDeviceName(): String {
        return "${Build.MANUFACTURER} ${Build.MODEL}"
    }
    
    fun getOsVersion(): String {
        return "Android ${Build.VERSION.RELEASE}"
    }
}
