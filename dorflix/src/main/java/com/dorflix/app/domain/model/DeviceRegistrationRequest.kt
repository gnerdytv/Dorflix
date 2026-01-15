package com.dorflix.app.domain.model

/**
 * Request model for device registration
 */
data class DeviceRegistrationRequest(
    val deviceId: String,
    val deviceName: String?,
    val deviceType: String?,
    val deviceModel: String?,
    val osVersion: String?,
    val appVersion: String?
)
