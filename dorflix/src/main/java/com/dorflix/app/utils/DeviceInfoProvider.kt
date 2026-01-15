package com.dorflix.app.utils

import android.content.Context
import android.os.Build
import android.provider.Settings
import java.util.*

object DeviceInfoProvider {
    fun getDeviceId(context: Context): String {
        return Settings.Secure.getString(context.contentResolver, Settings.Secure.ANDROID_ID)
    }

    fun getDeviceModel(): String {
        return "${Build.MANUFACTURER} ${Build.MODEL}"
    }

    fun getOSVersion(): String {
        return Build.VERSION.RELEASE
    }

    fun getLocale(): String {
        return Locale.getDefault().language
    }

    fun getDeviceType(): String {
        return if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
            "Android TV"
        } else {
            "Mobile"
        }
    }
}
