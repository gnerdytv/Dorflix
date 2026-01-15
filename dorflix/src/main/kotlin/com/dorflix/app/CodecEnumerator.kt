package com.dorflix.app

import android.media.MediaCodecList
import android.media.MediaCodecInfo
import android.media.MediaFormat
import android.media.MediaCodecInfo.CodecCapabilities
import android.media.MediaCodecInfo.VideoCapabilities
import android.media.MediaCodec
import android.os.Build
import android.util.Log
import android.view.Surface
import android.content.Context
import android.content.SharedPreferences
import com.google.gson.Gson
import com.google.gson.reflect.TypeToken
import kotlin.system.measureTimeMillis

/**
 * Kotlin version of Java bridge for MediaCodec enumeration
 * Provides codec capability information to native C++ code
 */

// Data classes for codec information persistence
data class CodecInfo(
    val name: String,
    val mimeType: String,
    val isEncoder: Boolean,
    val isHardware: Boolean,
    val maxWidth: Int,
    val maxHeight: Int,
    val profile: Int,
    val level: Int,
    val performanceScore: Long = -1,
    val priorityScore: Int = 0,
    val lastValidated: Long = System.currentTimeMillis(),
    val hdrSupport: List<String> = emptyList(),
    val colorFormats: List<String> = emptyList(),
    val maxBitrate: Int = 0
)

data class CodecEnumerationResult(
    val codecs: List<CodecInfo>,
    val enumerationTime: Long,
    val deviceInfo: String,
    val androidVersion: Int = Build.VERSION.SDK_INT,
    val appVersionCode: Int = 0,
    val securityPatch: String = "",
    val totalCodecCount: Int = 0
)

data class CodecPriority(
    val name: String,
    val score: Int, // Higher = better
    val conditions: Map<String, Any> = emptyMap()
)

class CodecEnumerator private constructor() {
    companion object {
        private val codecBlacklist = setOf(
            "OMX.google.mpeg2.decoder", // Known problematic codecs
            "OMX.google.h263.decoder"
        )

        private val codecPriorityList = listOf(
            CodecPriority("OMX.qcom.", 100),
            CodecPriority("c2.android.", 90),
            CodecPriority("OMX.google.", 80),
            CodecPriority("OMX.", 70), // Generic hardware
            CodecPriority("c2.", 60)   // Generic Codec2
        )

        /**
         * Calculate priority score for a codec based on its name
         * Higher scores indicate better/preferred codecs
         */
        private fun calculateCodecPriority(codecName: String): Int {
            for (priority in codecPriorityList) {
                if (codecName.startsWith(priority.name)) {
                    return priority.score
                }
            }
            return 0 // Default priority for unmatched codecs
        }

        /**
         * Check if a codec should be skipped based on the blacklist
         */
        private fun shouldSkipCodec(name: String): Boolean {
            return codecBlacklist.any { blacklisted ->
                name.contains(blacklisted, ignoreCase = true)
            }
        }

        @JvmStatic
        fun onCodecEnumerated(name: String, mimeType: String,
                              isEncoder: Boolean, isHardware: Boolean,
                              maxWidth: Int, maxHeight: Int,
                              profile: Int, level: Int) {
            // This will be called from C++ as a callback
            Log.v(TAG, "Callback received: codec $name ($mimeType) HW=$isHardware ${maxWidth}x$maxHeight")
        }

        @JvmStatic
        fun onEnumerationComplete(totalCodecs: Int) {
            // This will be called from C++ as a callback
            Log.i(TAG, "Enumeration complete callback: $totalCodecs codecs")
        }

        @JvmStatic
        external fun storeCodecInfo(name: String, mimeType: String,
                                    isEncoder: Boolean, isHardware: Boolean,
                                    maxWidth: Int, maxHeight: Int,
                                    profile: Int, level: Int,
                                    hdrSupport: Array<String>, colorFormats: Array<String>,
                                    maxBitrate: Int)

        @JvmStatic
        external fun signalEnumerationComplete(totalCodecs: Int)
        private const val TAG = "CodecEnumerator"

        /**
         * Safely enumerate all codecs with error recovery
         * Returns true if enumeration succeeded, false if any errors occurred
         */
        @JvmStatic
        fun safeEnumerateAllCodecs(): Boolean {
            return try {
                enumerateAllCodecs()
                true
            } catch (e: SecurityException) {
                // Missing permissions
                Log.e(TAG, "Permission denied for codec enumeration", e)
                false
            } catch (e: IllegalArgumentException) {
                // Invalid parameters
                Log.e(TAG, "Invalid codec enumeration parameters", e)
                false
            } catch (e: Exception) {
                // Other errors
                Log.e(TAG, "Codec enumeration failed", e)
                false
            }
        }

        /**
         * Enumerate all available MediaCodec instances with comprehensive logging
         */
        @JvmStatic
        fun enumerateAllCodecs() {
            val totalStartTime = System.currentTimeMillis()
            Log.i(TAG, "🎯 [CODEC_ENUM_START] Starting MediaCodec enumeration (thread: ${Thread.currentThread().name})")

            try {
                val codecListTime = measureTimeMillis {
                    val codecList = MediaCodecList(MediaCodecList.ALL_CODECS)
                    codecList.codecInfos
                }

                val codecs = MediaCodecList(MediaCodecList.ALL_CODECS).codecInfos
                Log.i(TAG, "📊 [CODEC_LIST_ACQUIRED] Found ${codecs.size} total codecs in ${codecListTime}ms")

                var enumeratedCount = 0
                var processedCount = 0
                var skippedEncoders = 0
                var skippedNonVideo = 0
                var skippedBlacklisted = 0
                var errorCount = 0

                val codecProcessingTime = measureTimeMillis {
                    for ((index, codecInfo) in codecs.withIndex()) {
                        try {
                            val codecStartTime = System.currentTimeMillis()
                            val codecName = codecInfo.name
                            val isEncoder = codecInfo.isEncoder
                            val isHardware = getIsHardwareAccelerated(codecInfo)

                            Log.v(TAG, "🔍 [CODEC_CHECK_$index] Processing: $codecName (encoder=$isEncoder, hardware=$isHardware)")

                            // Skip blacklisted codecs
                            if (shouldSkipCodec(codecName)) {
                                Log.v(TAG, "🚫 [BLACKLIST_SKIP] Skipping blacklisted codec: $codecName")
                                skippedBlacklisted++
                                continue
                            }

                            // Skip encoders - we only want decoders for now
                            if (isEncoder) {
                                Log.v(TAG, "⏭️ [ENCODER_SKIP] Skipping encoder: $codecName")
                                skippedEncoders++
                                continue
                            }

                            // Get supported types for this codec
                            val supportedTypes = codecInfo.supportedTypes
                            Log.v(TAG, "📋 [TYPES_CHECK] $codecName supports ${supportedTypes.size} types")

                            for ((typeIndex, mimeType) in supportedTypes.withIndex()) {
                                Log.v(TAG, "📺 [MIME_CHECK_$typeIndex] Checking: $mimeType for $codecName")

                                // Check if this is a video MIME type
                                if (!mimeType.startsWith("video/")) {
                                    Log.v(TAG, "⏭️ [NON_VIDEO_SKIP] Skipping non-video: $mimeType")
                                    skippedNonVideo++
                                    continue
                                }

                                val capabilitiesTime = measureTimeMillis {
                                    codecInfo.getCapabilitiesForType(mimeType)
                                }

                                val capabilities = codecInfo.getCapabilitiesForType(mimeType)
                                if (capabilities == null) {
                                    Log.w(TAG, "⚠️ [NO_CAPABILITIES] $codecName has no capabilities for $mimeType (${capabilitiesTime}ms)")
                                    continue
                                }

                                Log.v(TAG, "✅ [CAPABILITIES_OK] $codecName capabilities acquired in ${capabilitiesTime}ms")

                                // Get video capabilities
                                val videoCaps = capabilities.videoCapabilities
                                if (videoCaps == null) {
                                    Log.w(TAG, "⚠️ [NO_VIDEO_CAPS] $codecName has no video capabilities")
                                    continue
                                }

                                // Two-stage validation: lightweight first, then full validation
                                val validationTime = measureTimeMillis {
                                    // Stage 1: Lightweight validation (fast)
                                    if (!validateCodecCapabilityLightweight(codecInfo, mimeType)) {
                                        Log.w(TAG, "⚠️ [LIGHTWEIGHT_VALIDATION_SKIP] Skipping codec with failed lightweight validation: $codecName for $mimeType")
                                        continue
                                    }

                                    // Stage 2: Full validation (expensive, but only for promising codecs)
                                    if (!validateCodecCapability(codecInfo, mimeType)) {
                                        Log.w(TAG, "⚠️ [FULL_VALIDATION_SKIP] Skipping codec with failed full validation: $codecName for $mimeType")
                                        continue
                                    }
                                }

                                Log.v(TAG, "✅ [VALIDATION_PASSED] Codec validated in ${validationTime}ms")

                                // Get supported resolutions
                                val maxWidth = videoCaps.supportedWidths.upper
                                val maxHeight = videoCaps.supportedHeights.upper

                                // Extract real profile/level from capabilities instead of heuristic approach
                                val (profile, level) = extractRealProfileLevel(codecInfo, mimeType)

                                // Extract HDR support information
                                val hdrSupport = extractHDRSupport(codecInfo, mimeType)

                                // Extract color format information
                                val colorFormats = extractColorFormats(codecInfo, mimeType)

                                // Extract maximum bitrate information
                                val maxBitrate = extractMaxBitrate(codecInfo, mimeType)

                                // Optional: Measure performance (can be expensive, so consider making this configurable)
                                val performanceTime = measureCodecPerformance(codecInfo, mimeType)
                                Log.v(TAG, "⏱️ [PERFORMANCE_MEASURED] ${codecInfo.name} performance: ${performanceTime}ms")

                                Log.i(TAG, "✅ [DECODER_VALIDATED] $codecName ($mimeType, HW=$isHardware, ${maxWidth}x$maxHeight, P=$profile L=$level, HDR=${hdrSupport.joinToString(",")}, Colors=${colorFormats.joinToString(",")}, Bitrate=${maxBitrate}bps, validation=${validationTime}ms, ${System.currentTimeMillis() - codecStartTime}ms)")

                                // Bridge to native code - store codec info in native memory
                                val nativeCallTime = measureTimeMillis {
                                    CodecEnumerator.storeCodecInfo(codecName, mimeType, isEncoder, isHardware,
                                                    maxWidth, maxHeight, profile, level,
                                                    hdrSupport.toTypedArray(), colorFormats.toTypedArray(),
                                                    maxBitrate)
                                }

                                Log.v(TAG, "🌉 [NATIVE_BRIDGE] JNI call completed in ${nativeCallTime}ms")

                                enumeratedCount++
                                // Don't break - continue to enumerate all supported types for this codec
                            }

                            processedCount++

                        } catch (e: Exception) {
                            Log.w(TAG, "❌ [CODEC_ERROR] Error processing codec ${codecInfo.name}: ${e.message}", e)
                            errorCount++
                        }
                    }
                }

                val totalTime = System.currentTimeMillis() - totalStartTime
                Log.i(TAG, "🎯 [ENUMERATION_COMPLETE] $enumeratedCount decoders found in ${totalTime}ms")
                Log.i(TAG, "📈 [ENUMERATION_STATS] Processed: $processedCount, Skipped encoders: $skippedEncoders, Skipped non-video: $skippedNonVideo, Skipped blacklisted: $skippedBlacklisted, Errors: $errorCount")
                Log.i(TAG, "⏱️ [PERFORMANCE] Codec processing: ${codecProcessingTime}ms, JNI calls: ${enumeratedCount * 2}ms estimated")

                // Notify native code that enumeration is complete
                val completionTime = measureTimeMillis {
                    CodecEnumerator.signalEnumerationComplete(enumeratedCount)
                }

                Log.i(TAG, "✅ [FINAL_CALLBACK] Enumeration completion notified in ${completionTime}ms")

            } catch (e: Exception) {
                val errorTime = System.currentTimeMillis() - totalStartTime
                Log.e(TAG, "❌ [ENUMERATION_FAILED] Fatal error after ${errorTime}ms: ${e.message}", e)
                CodecEnumerator.signalEnumerationComplete(0)
            }
        }

    /**
     * Get detailed information about a specific codec
     */
    @JvmStatic
    fun getCodecDetails(codecName: String) {
        Log.d(TAG, "Getting details for codec: $codecName")

        try {
            val codecList = MediaCodecList(MediaCodecList.ALL_CODECS)
            val codecs = codecList.codecInfos

            for (codecInfo in codecs) {
                if (codecName == codecInfo.name) {
                    Log.i(TAG, "📊 Details for $codecName:")

                    val types = codecInfo.supportedTypes
                    for (type in types) {
                        Log.i(TAG, "  - Supports: $type")

                        if (MediaFormat.MIMETYPE_VIDEO_AVC == type) {
                            val caps = codecInfo.getCapabilitiesForType(type)
                            if (caps != null) {
                                val videoCaps = caps.videoCapabilities
                                if (videoCaps != null) {
                                    Log.i(TAG, "    Max resolution: ${videoCaps.supportedWidths.upper}x${videoCaps.supportedHeights.upper}")
                                }
                            }
                        }
                    }

                    break
                }
            }

        } catch (e: Exception) {
            Log.e(TAG, "Error getting codec details for $codecName", e)
        }
    }

    /**
     * Safely determines if a codec is hardware accelerated, with backward compatibility
     * for Android API levels below 29 where isHardwareAccelerated() doesn't exist.
     */
    private fun getIsHardwareAccelerated(codecInfo: MediaCodecInfo): Boolean {
        return if (Build.VERSION.SDK_INT >= 29) {
            // Use the official API for Android 10+
            codecInfo.isHardwareAccelerated
        } else {
            // Fallback for older Android versions: check codec name patterns
            isHardwareAcceleratedFallback(codecInfo.name)
        }
    }

    /**
     * Extract H.264 profile/level information from codec name
     * This is a heuristic approach since Android doesn't expose profile/level for decoders
     */
    private fun extractH264ProfileLevel(codecName: String): Int {
        val name = codecName.lowercase()

        // Check for explicit profile/level in codec name
        if (name.contains("high")) {
            return 2 // High profile
        } else if (name.contains("main")) {
            return 1 // Main profile
        } else if (name.contains("baseline")) {
            return 0 // Baseline profile
        }

        // Default to main profile for most modern devices
        return 1
    }

    /**
     * Fallback method to determine hardware acceleration for API < 29
     * Based on common codec naming conventions:
     * - OMX.* : OpenMAX hardware codecs
     * - c2.* : Codec2.0 hardware codecs
     */
    private fun isHardwareAcceleratedFallback(codecName: String?): Boolean {
        if (codecName == null) {
            return false
        }

        // Hardware codecs typically start with OMX or c2
        return codecName.startsWith("OMX.") || codecName.startsWith("c2.")
    }

    /**
     * Extract real profile/level support from MediaCodec capabilities
     * Instead of heuristic name matching, query actual capabilities
     */
    private fun extractRealProfileLevel(codecInfo: MediaCodecInfo, mimeType: String): Pair<Int, Int> {
        try {
            val capabilities = codecInfo.getCapabilitiesForType(mimeType)
            if (capabilities?.profileLevels == null) {
                return Pair(-1, -1)
            }

            val profileLevels = capabilities.profileLevels
            if (profileLevels.isEmpty()) {
                return Pair(-1, -1)
            }

            // Find the highest supported profile and level
            var maxProfile = 0
            var maxLevel = 0

            for (profileLevel in profileLevels) {
                when (mimeType) {
                    MediaFormat.MIMETYPE_VIDEO_AVC -> {
                        maxProfile = maxOf(maxProfile, profileLevel.profile)
                        maxLevel = maxOf(maxLevel, profileLevel.level)
                    }
                    MediaFormat.MIMETYPE_VIDEO_HEVC -> {
                        maxProfile = maxOf(maxProfile, profileLevel.profile)
                        maxLevel = maxOf(maxLevel, profileLevel.level)
                    }
                    MediaFormat.MIMETYPE_VIDEO_VP9 -> {
                        maxProfile = maxOf(maxProfile, profileLevel.profile)
                        maxLevel = maxOf(maxLevel, profileLevel.level)
                    }
                    MediaFormat.MIMETYPE_VIDEO_AV1 -> {
                        maxProfile = maxOf(maxProfile, profileLevel.profile)
                        maxLevel = maxOf(maxLevel, profileLevel.level)
                    }
                }
            }

            Log.v(TAG, "📊 [PROFILE_LEVEL] $mimeType: profile=$maxProfile, level=$maxLevel")
            return Pair(maxProfile, maxLevel)

        } catch (e: Exception) {
            Log.w(TAG, "⚠️ [PROFILE_LEVEL_ERROR] Failed to extract profile/level for ${codecInfo.name}: ${e.message}")
            return Pair(-1, -1)
        }
    }

    /**
     * Extract HDR support information from codec capabilities
     */
    private fun extractHDRSupport(codecInfo: MediaCodecInfo, mimeType: String): List<String> {
        val hdrSupport = mutableListOf<String>()

        try {
            val capabilities = codecInfo.getCapabilitiesForType(mimeType)
            val profileLevels = capabilities?.profileLevels ?: return emptyList()

            for (profileLevel in profileLevels) {
                when (mimeType) {
                    MediaFormat.MIMETYPE_VIDEO_HEVC -> {
                        when {
                            profileLevel.profile >= MediaCodecInfo.CodecProfileLevel.HEVCProfileMain10HDR10 -> {
                                if (!hdrSupport.contains("HDR10")) hdrSupport.add("HDR10")
                                if (!hdrSupport.contains("HLG")) hdrSupport.add("HLG")
                            }
                            profileLevel.profile >= MediaCodecInfo.CodecProfileLevel.HEVCProfileMain10 -> {
                                if (!hdrSupport.contains("HDR10")) hdrSupport.add("HDR10")
                            }
                        }
                        // Check for Dolby Vision specific profiles
                        if (profileLevel.profile == 0x200 || profileLevel.profile == 0x201 ||
                            profileLevel.profile == 0x400 || profileLevel.profile == 0x401) {
                            if (!hdrSupport.contains("Dolby Vision")) hdrSupport.add("Dolby Vision")
                        }
                    }
                    MediaFormat.MIMETYPE_VIDEO_VP9 -> {
                        if (profileLevel.profile >= MediaCodecInfo.CodecProfileLevel.VP9Profile2HDR) {
                            if (!hdrSupport.contains("HDR10")) hdrSupport.add("HDR10")
                        } else if (profileLevel.profile >= MediaCodecInfo.CodecProfileLevel.VP9Profile2) {
                            if (!hdrSupport.contains("HDR10")) hdrSupport.add("HDR10")
                        }
                    }
                    MediaFormat.MIMETYPE_VIDEO_AV1 -> {
                        when {
                            profileLevel.profile >= MediaCodecInfo.CodecProfileLevel.AV1ProfileMain10HDR10 -> {
                                if (!hdrSupport.contains("HDR10")) hdrSupport.add("HDR10")
                                if (!hdrSupport.contains("HLG")) hdrSupport.add("HLG")
                            }
                            profileLevel.profile >= MediaCodecInfo.CodecProfileLevel.AV1ProfileMain10 -> {
                                if (!hdrSupport.contains("HDR10")) hdrSupport.add("HDR10")
                            }
                        }
                    }
                }
            }

            if (hdrSupport.isNotEmpty()) {
                Log.v(TAG, "🎨 [HDR_SUPPORT] ${codecInfo.name} supports: ${hdrSupport.joinToString(", ")}")
            }

        } catch (e: Exception) {
            Log.w(TAG, "⚠️ [HDR_EXTRACTION_ERROR] Failed to extract HDR support for ${codecInfo.name}: ${e.message}")
        }

        return hdrSupport
    }

    /**
     * Extract color format information from codec capabilities
     */
    private fun extractColorFormats(codecInfo: MediaCodecInfo, mimeType: String): List<String> {
        val colorFormats = mutableListOf<String>()

        try {
            val capabilities = codecInfo.getCapabilitiesForType(mimeType)
            val formats = capabilities?.colorFormats ?: return emptyList()

            for (format in formats) {
                when (format) {
                    MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420Planar -> colorFormats.add("YUV420")
                    MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420PackedPlanar -> colorFormats.add("YUV420")
                    MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420SemiPlanar -> colorFormats.add("YUV420")
                    MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420PackedSemiPlanar -> colorFormats.add("YUV420")
                    0x7F420888 -> colorFormats.add("YUV420_10BIT") // COLOR_FormatYUVP010 - 10-bit
                    0x7F420889 -> colorFormats.add("YUV420_10BIT") // COLOR_FormatYUV420Flexible - can be 10-bit
                    MediaCodecInfo.CodecCapabilities.COLOR_Format32bitABGR8888 -> colorFormats.add("RGBA")
                    MediaCodecInfo.CodecCapabilities.COLOR_Format32bitARGB8888 -> colorFormats.add("RGBA")
                    else -> {
                        // Log unknown formats for debugging
                        Log.v(TAG, "🔍 [UNKNOWN_COLOR_FORMAT] ${codecInfo.name}: $format")
                        colorFormats.add("UNKNOWN_$format")
                    }
                }
            }

            // Remove duplicates and sort
            val uniqueFormats = colorFormats.distinct().sorted()

            if (uniqueFormats.isNotEmpty()) {
                Log.v(TAG, "🎨 [COLOR_FORMATS] ${codecInfo.name} supports: ${uniqueFormats.joinToString(", ")}")
            }

            return uniqueFormats

        } catch (e: Exception) {
            Log.w(TAG, "⚠️ [COLOR_FORMAT_ERROR] Failed to extract color formats for ${codecInfo.name}: ${e.message}")
        }

        return emptyList()
    }

    /**
     * Extract maximum bitrate information from codec capabilities
     */
    private fun extractMaxBitrate(codecInfo: MediaCodecInfo, mimeType: String): Int {
        try {
            val capabilities = codecInfo.getCapabilitiesForType(mimeType)
            val videoCaps = capabilities?.videoCapabilities
            val bitrateRange = videoCaps?.bitrateRange

            if (bitrateRange != null) {
                val maxBitrate = bitrateRange.upper
                Log.v(TAG, "⚡ [MAX_BITRATE] ${codecInfo.name}: ${maxBitrate} bps")
                return maxBitrate
            }

        } catch (e: Exception) {
            Log.w(TAG, "⚠️ [BITRATE_ERROR] Failed to extract max bitrate for ${codecInfo.name}: ${e.message}")
        }

        return 0
    }

    /**
     * Lightweight codec capability validation without creating codec instances
     * Quick validation using only capabilities information
     */
    private fun validateCodecCapabilityLightweight(codecInfo: MediaCodecInfo, mimeType: String): Boolean {
        // Quick validation without creating codec
        return try {
            val capabilities = codecInfo.getCapabilitiesForType(mimeType)
            capabilities != null && capabilities.videoCapabilities != null
        } catch (e: Exception) {
            false
        }
    }

    /**
     * Validate codec capability by attempting to create and configure it
     * Tests basic codec functionality before marking as available
     */
    private fun validateCodecCapability(codecInfo: MediaCodecInfo, mimeType: String): Boolean {
        var codec: MediaCodec? = null
        try {
            Log.v(TAG, "🔬 [VALIDATION_START] Testing codec: ${codecInfo.name} for $mimeType")

            // Test 1: Basic codec creation
            val createTime = measureTimeMillis {
                codec = MediaCodec.createByCodecName(codecInfo.name)
            }

            if (codec == null) {
                Log.w(TAG, "❌ [VALIDATION_FAIL] Failed to create codec: ${codecInfo.name}")
                return false
            }

            Log.v(TAG, "✅ [VALIDATION_CREATE] Codec created in ${createTime}ms")

            // Test 2: Basic configuration with minimal format
            val format = MediaFormat.createVideoFormat(mimeType, 640, 480)
            format.setInteger(MediaFormat.KEY_FRAME_RATE, 30)
            format.setInteger(MediaFormat.KEY_BIT_RATE, 1000000)
            format.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, 1)

            // Add codec-specific parameters based on MIME type
            when (mimeType) {
                MediaFormat.MIMETYPE_VIDEO_AVC -> {
                    format.setInteger(MediaFormat.KEY_PROFILE, MediaCodecInfo.CodecProfileLevel.AVCProfileBaseline)
                    format.setInteger(MediaFormat.KEY_LEVEL, MediaCodecInfo.CodecProfileLevel.AVCLevel1)
                }
                MediaFormat.MIMETYPE_VIDEO_HEVC -> {
                    format.setInteger(MediaFormat.KEY_PROFILE, MediaCodecInfo.CodecProfileLevel.HEVCProfileMain)
                    format.setInteger(MediaFormat.KEY_LEVEL, MediaCodecInfo.CodecProfileLevel.HEVCMainTierLevel1)
                }
                MediaFormat.MIMETYPE_VIDEO_VP9 -> {
                    format.setInteger(MediaFormat.KEY_PROFILE, MediaCodecInfo.CodecProfileLevel.VP9Profile0)
                    format.setInteger(MediaFormat.KEY_LEVEL, MediaCodecInfo.CodecProfileLevel.VP9Level1)
                }
                MediaFormat.MIMETYPE_VIDEO_AV1 -> {
                    format.setInteger(MediaFormat.KEY_PROFILE, MediaCodecInfo.CodecProfileLevel.AV1ProfileMain8)
                    format.setInteger(MediaFormat.KEY_LEVEL, MediaCodecInfo.CodecProfileLevel.AV1Level2)
                }
            }

            val configTime = measureTimeMillis {
                codec?.configure(format, null, null, 0) // Decoder mode
            }

            Log.v(TAG, "✅ [VALIDATION_CONFIG] Codec configured in ${configTime}ms")

            // Test 3: Start/stop test (quick functional test)
            val startTime = measureTimeMillis {
                codec?.start()
            }

            Log.v(TAG, "✅ [VALIDATION_START] Codec started in ${startTime}ms")

            // Quick stop test
            val stopTime = measureTimeMillis {
                codec?.stop()
            }

            Log.v(TAG, "✅ [VALIDATION_STOP] Codec stopped in ${stopTime}ms")

            Log.i(TAG, "✅ [VALIDATION_SUCCESS] Codec ${codecInfo.name} validation passed")
            return true

        } catch (e: Exception) {
            Log.w(TAG, "❌ [VALIDATION_FAIL] Codec ${codecInfo.name} validation failed: ${e.message}")
            return false
        } finally {
            // Clean up
            try {
                codec?.release()
            } catch (e: Exception) {
                Log.w(TAG, "⚠️ [VALIDATION_CLEANUP] Error releasing codec: ${e.message}")
            }
        }
    }

        /**
         * Get codec performance metrics by running a quick benchmark
         */
        private fun measureCodecPerformance(codecInfo: MediaCodecInfo, mimeType: String): Long {
            // This is a simplified performance measurement
            // In a real implementation, you'd want to test with actual video data
            var codec: MediaCodec? = null
            val startTime = System.nanoTime()

            try {
                codec = MediaCodec.createByCodecName(codecInfo.name)
                val format = MediaFormat.createVideoFormat(mimeType, 640, 480)
                format.setInteger(MediaFormat.KEY_FRAME_RATE, 30)
                format.setInteger(MediaFormat.KEY_BIT_RATE, 1000000)

                codec.configure(format, null, null, 0)
                codec.start()

                // Test actual buffer operations instead of artificial sleep
                val testBufferTime = measureTimeMillis {
                    try {
                        // Try to dequeue an input buffer (non-blocking test)
                        val bufferInfo = MediaCodec.BufferInfo()
                        val inputBufferId = codec.dequeueInputBuffer(0) // Non-blocking
                        if (inputBufferId >= 0) {
                            // Got a buffer, queue it with EOS to test basic functionality
                            codec.queueInputBuffer(inputBufferId, 0, 0, 0, MediaCodec.BUFFER_FLAG_END_OF_STREAM)
                            Log.v(TAG, "✅ [PERFORMANCE_TEST] Successfully queued EOS buffer")
                        }

                        // Try to dequeue output (should get EOS quickly)
                        val outputBufferId = codec.dequeueOutputBuffer(bufferInfo, 1000) // 1ms timeout
                        if (outputBufferId >= 0) {
                            codec.releaseOutputBuffer(outputBufferId, false)
                            Log.v(TAG, "✅ [PERFORMANCE_TEST] Successfully processed output buffer")
                        }
                    } catch (e: Exception) {
                        Log.v(TAG, "ℹ️ [PERFORMANCE_TEST] Buffer test completed with minor issues: ${e.message}")
                    }
                }

                Log.v(TAG, "⏱️ [PERFORMANCE_TEST] Buffer operations took ${testBufferTime}ms")

                codec.stop()

            } catch (e: Exception) {
                Log.w(TAG, "⚠️ [PERFORMANCE_TEST] Failed to measure performance for ${codecInfo.name}: ${e.message}")
                return -1
            } finally {
                try {
                    codec?.release()
                } catch (e: Exception) {
                    // Ignore cleanup errors
                }
            }

            val endTime = System.nanoTime()
            val durationMs = (endTime - startTime) / 1_000_000
            Log.v(TAG, "⏱️ [PERFORMANCE] ${codecInfo.name} took ${durationMs}ms for basic test")
            return durationMs
        }

        /**
         * Save enumeration results to persistent storage
         */
        private fun saveEnumerationResults(codecs: List<CodecInfo>, enumerationTime: Long) {
            try {
                val context = DorflixApplication.instance?.applicationContext ?: return
                val prefs = context.getSharedPreferences("codec_enumeration", Context.MODE_PRIVATE)
                val gson = Gson()

                val deviceInfo = "${Build.MANUFACTURER} ${Build.MODEL} (Android ${Build.VERSION.RELEASE})"
                val appVersionCode = try {
                    context.packageManager.getPackageInfo(context.packageName, 0).versionCode
                } catch (e: Exception) {
                    Log.w(TAG, "⚠️ [VERSION_CODE_ERROR] Failed to get app version code: ${e.message}")
                    0
                }
                val securityPatch = try {
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                        Build.VERSION.SECURITY_PATCH
                    } else {
                        ""
                    }
                } catch (e: Exception) {
                    Log.w(TAG, "⚠️ [SECURITY_PATCH_ERROR] Failed to get security patch: ${e.message}")
                    ""
                }
                val totalCodecCount = MediaCodecList(MediaCodecList.ALL_CODECS).codecInfos.size

                val result = CodecEnumerationResult(
                    codecs = codecs,
                    enumerationTime = enumerationTime,
                    deviceInfo = deviceInfo,
                    androidVersion = Build.VERSION.SDK_INT,
                    appVersionCode = appVersionCode,
                    securityPatch = securityPatch,
                    totalCodecCount = totalCodecCount
                )

                val json = gson.toJson(result)
                prefs.edit()
                    .putString("enumeration_result", json)
                    .putLong("last_enumeration_time", System.currentTimeMillis())
                    .apply()

                Log.i(TAG, "💾 [CACHE_SAVED] Saved ${codecs.size} codecs to cache (Android ${Build.VERSION.SDK_INT}, App v$appVersionCode, Security: $securityPatch, Total codecs: $totalCodecCount)")
            } catch (e: Exception) {
                Log.w(TAG, "⚠️ [CACHE_SAVE_ERROR] Failed to save enumeration results: ${e.message}")
            }
        }

        /**
         * Load enumeration results from persistent storage
         */
        private fun loadEnumerationResults(): CodecEnumerationResult? {
            return try {
                val context = DorflixApplication.instance?.applicationContext ?: return null
                val prefs = context.getSharedPreferences("codec_enumeration", Context.MODE_PRIVATE)
                val gson = Gson()

                val json = prefs.getString("enumeration_result", null) ?: return null
                val result: CodecEnumerationResult = gson.fromJson(json, object : TypeToken<CodecEnumerationResult>() {}.type)

                // Check if device has changed (invalidate cache if so)
                val currentDeviceInfo = "${Build.MANUFACTURER} ${Build.MODEL} (Android ${Build.VERSION.RELEASE})"
                if (result.deviceInfo != currentDeviceInfo) {
                    Log.i(TAG, "📱 [CACHE_INVALIDATED] Device changed, invalidating cache")
                    clearEnumerationCache()
                    return null
                }

                // Check if cache is too old (invalidate after 7 days)
                val lastEnumerationTime = prefs.getLong("last_enumeration_time", 0)
                val cacheAgeDays = (System.currentTimeMillis() - lastEnumerationTime) / (1000 * 60 * 60 * 24)
                if (cacheAgeDays > 7) {
                    Log.i(TAG, "⏰ [CACHE_EXPIRED] Cache is ${cacheAgeDays} days old, refreshing")
                    clearEnumerationCache()
                    return null
                }

                Log.i(TAG, "💾 [CACHE_LOADED] Loaded ${result.codecs.size} codecs from cache (${cacheAgeDays} days old)")
                result
            } catch (e: Exception) {
                Log.w(TAG, "⚠️ [CACHE_LOAD_ERROR] Failed to load enumeration results: ${e.message}")
                null
            }
        }

        /**
         * Clear the enumeration cache
         */
        private fun clearEnumerationCache() {
            try {
                val context = DorflixApplication.instance?.applicationContext ?: return
                val prefs = context.getSharedPreferences("codec_enumeration", Context.MODE_PRIVATE)
                prefs.edit().clear().apply()
                Log.i(TAG, "🗑️ [CACHE_CLEARED] Enumeration cache cleared")
            } catch (e: Exception) {
                Log.w(TAG, "⚠️ [CACHE_CLEAR_ERROR] Failed to clear cache: ${e.message}")
            }
        }

        /**
         * Check if enumeration results need to be refreshed
         * Enhanced cache invalidation with multiple triggers for robustness
         */
        private fun shouldRefreshEnumeration(): Boolean {
            val cached = loadEnumerationResults() ?: return true

            // Check if system codecs might have changed due to OS updates
            val hasSystemUpdate = Build.VERSION.SDK_INT > cached.androidVersion

            // Check if it's been more than 7 days since enumeration
            val hasBeenLongTime = System.currentTimeMillis() - cached.enumerationTime > 7 * 24 * 60 * 60 * 1000L

            // Check if cached codec list is empty (corrupted cache)
            val hasEmptyCodecs = cached.codecs.isEmpty()

            // Check if app has been updated (different version)
            val currentAppVersion = try {
                DorflixApplication.instance?.applicationContext?.packageManager
                    ?.getPackageInfo(DorflixApplication.instance?.applicationContext?.packageName ?: "", 0)
                    ?.versionCode ?: 0
            } catch (e: Exception) {
                0
            }
            val hasAppUpdate = currentAppVersion > cached.appVersionCode

            // Check if security patch has changed
            val currentSecurityPatch = try {
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                    Build.VERSION.SECURITY_PATCH
                } else {
                    ""
                }
            } catch (e: Exception) {
                ""
            }
            val hasSecurityUpdate = currentSecurityPatch != cached.securityPatch

            // Check if total codec count has changed significantly (system update indicator)
            val currentCodecCount = MediaCodecList(MediaCodecList.ALL_CODECS).codecInfos.size
            val codecCountThreshold = 5 // Allow for small variations
            val hasCodecCountChanged = kotlin.math.abs(currentCodecCount - cached.totalCodecCount) > codecCountThreshold

            // Determine if refresh is needed
            val needsRefresh = hasSystemUpdate || hasBeenLongTime || hasEmptyCodecs ||
                              hasAppUpdate || hasSecurityUpdate || hasCodecCountChanged

            // Log the reason for cache invalidation
            if (needsRefresh) {
                val reasons = mutableListOf<String>()
                if (hasSystemUpdate) reasons.add("Android version changed (${cached.androidVersion} → ${Build.VERSION.SDK_INT})")
                if (hasBeenLongTime) {
                    val days = (System.currentTimeMillis() - cached.enumerationTime) / (1000 * 60 * 60 * 24)
                    reasons.add("Cache is ${days} days old")
                }
                if (hasEmptyCodecs) reasons.add("Empty codec list in cache")
                if (hasAppUpdate) reasons.add("App updated (${cached.appVersionCode} → $currentAppVersion)")
                if (hasSecurityUpdate) reasons.add("Security patch changed ('${cached.securityPatch}' → '$currentSecurityPatch')")
                if (hasCodecCountChanged) reasons.add("Codec count changed (${cached.totalCodecCount} → $currentCodecCount)")

                Log.i(TAG, "🔄 [CACHE_INVALIDATION] Refreshing cache due to: ${reasons.joinToString(", ")}")
            }

            return needsRefresh
        }

        /**
         * Enumerate all available MediaCodec instances with selective validation
         * @param validate Whether to perform expensive full validation (default: true)
         * @param measurePerformance Whether to measure codec performance (default: false)
         * @return List of validated codec information
         */
        @JvmStatic
        fun enumerateCodecs(
            validate: Boolean = true,
            measurePerformance: Boolean = false
        ): List<CodecInfo> {
            val codecs = mutableListOf<CodecInfo>()
            enumerateCodecsInternal(codecs, validate, measurePerformance)
            return codecs
        }

        /**
         * Internal enumeration function with conditional validation and performance measurement
         */
        private fun enumerateCodecsInternal(
            collectedCodecs: MutableList<CodecInfo>,
            validate: Boolean,
            measurePerformance: Boolean
        ) {
            val totalStartTime = System.currentTimeMillis()
            Log.i(TAG, "🎯 [SELECTIVE_ENUM_START] Starting MediaCodec enumeration (validate=$validate, measurePerformance=$measurePerformance, thread: ${Thread.currentThread().name})")

            try {
                val codecListTime = measureTimeMillis {
                    val codecList = MediaCodecList(MediaCodecList.ALL_CODECS)
                    codecList.codecInfos
                }

                val codecs = MediaCodecList(MediaCodecList.ALL_CODECS).codecInfos
                Log.i(TAG, "📊 [CODEC_LIST_ACQUIRED] Found ${codecs.size} total codecs in ${codecListTime}ms")

                var enumeratedCount = 0
                var processedCount = 0
                var skippedEncoders = 0
                var skippedNonVideo = 0
                var skippedValidation = 0
                var skippedBlacklisted = 0
                var errorCount = 0

                val codecProcessingTime = measureTimeMillis {
                    for ((index, codecInfo) in codecs.withIndex()) {
                        try {
                            val codecStartTime = System.currentTimeMillis()
                            val codecName = codecInfo.name
                            val isEncoder = codecInfo.isEncoder
                            val isHardware = getIsHardwareAccelerated(codecInfo)

                            Log.v(TAG, "🔍 [CODEC_CHECK_$index] Processing: $codecName (encoder=$isEncoder, hardware=$isHardware)")

                            // Skip blacklisted codecs
                            if (shouldSkipCodec(codecName)) {
                                Log.v(TAG, "🚫 [BLACKLIST_SKIP] Skipping blacklisted codec: $codecName")
                                skippedBlacklisted++
                                continue
                            }

                            // Skip encoders - we only want decoders for now
                            if (isEncoder) {
                                Log.v(TAG, "⏭️ [ENCODER_SKIP] Skipping encoder: $codecName")
                                skippedEncoders++
                                continue
                            }

                            // Get supported types for this codec
                            val supportedTypes = codecInfo.supportedTypes
                            Log.v(TAG, "📋 [TYPES_CHECK] $codecName supports ${supportedTypes.size} types")

                            for ((typeIndex, mimeType) in supportedTypes.withIndex()) {
                                Log.v(TAG, "📺 [MIME_CHECK_$typeIndex] Checking: $mimeType for $codecName")

                                // Check if this is a video MIME type
                                if (!mimeType.startsWith("video/")) {
                                    Log.v(TAG, "⏭️ [NON_VIDEO_SKIP] Skipping non-video: $mimeType")
                                    skippedNonVideo++
                                    continue
                                }

                                val capabilitiesTime = measureTimeMillis {
                                    codecInfo.getCapabilitiesForType(mimeType)
                                }

                                val capabilities = codecInfo.getCapabilitiesForType(mimeType)
                                if (capabilities == null) {
                                    Log.w(TAG, "⚠️ [NO_CAPABILITIES] $codecName has no capabilities for $mimeType (${capabilitiesTime}ms)")
                                    continue
                                }

                                Log.v(TAG, "✅ [CAPABILITIES_OK] $codecName capabilities acquired in ${capabilitiesTime}ms")

                                // Get video capabilities
                                val videoCaps = capabilities.videoCapabilities
                                if (videoCaps == null) {
                                    Log.w(TAG, "⚠️ [NO_VIDEO_CAPS] $codecName has no video capabilities")
                                    continue
                                }

                                // Conditional validation: lightweight always, full validation optional
                                val validationTime = measureTimeMillis {
                                    // Stage 1: Lightweight validation (always performed - fast and necessary)
                                    if (!validateCodecCapabilityLightweight(codecInfo, mimeType)) {
                                        Log.w(TAG, "⚠️ [LIGHTWEIGHT_VALIDATION_SKIP] Skipping codec with failed lightweight validation: $codecName for $mimeType")
                                        skippedValidation++
                                        continue
                                    }

                                    // Stage 2: Full validation (optional, expensive)
                                    if (validate && !validateCodecCapability(codecInfo, mimeType)) {
                                        Log.w(TAG, "⚠️ [FULL_VALIDATION_SKIP] Skipping codec with failed full validation: $codecName for $mimeType")
                                        skippedValidation++
                                        continue
                                    }
                                }

                                Log.v(TAG, "✅ [VALIDATION_PASSED] Codec validated in ${validationTime}ms (validate=$validate)")

                                // Get supported resolutions
                                val maxWidth = videoCaps.supportedWidths.upper
                                val maxHeight = videoCaps.supportedHeights.upper

                                // Extract real profile/level from capabilities instead of heuristic approach
                                val (profile, level) = extractRealProfileLevel(codecInfo, mimeType)

                                // Extract HDR support information
                                val hdrSupport = extractHDRSupport(codecInfo, mimeType)

                                // Extract color format information
                                val colorFormats = extractColorFormats(codecInfo, mimeType)

                                // Extract maximum bitrate information
                                val maxBitrate = extractMaxBitrate(codecInfo, mimeType)

                                // Conditional performance measurement
                                val performanceTime = if (measurePerformance) {
                                    val perfTime = measureCodecPerformance(codecInfo, mimeType)
                                    Log.v(TAG, "⏱️ [PERFORMANCE_MEASURED] ${codecInfo.name} performance: ${perfTime}ms")
                                    perfTime
                                } else {
                                    Log.v(TAG, "⏱️ [PERFORMANCE_SKIPPED] Skipping performance measurement for ${codecInfo.name}")
                                    -1L // Not measured
                                }

                                Log.i(TAG, "✅ [DECODER_VALIDATED] $codecName ($mimeType, HW=$isHardware, ${maxWidth}x$maxHeight, P=$profile L=$level, HDR=${hdrSupport.joinToString(",")}, Colors=${colorFormats.joinToString(",")}, Bitrate=${maxBitrate}bps, validation=${validationTime}ms, perf=${performanceTime}ms, ${System.currentTimeMillis() - codecStartTime}ms)")

                                // Calculate priority score for this codec
                                val priorityScore = calculateCodecPriority(codecName)

                                // Create codec info for collection
                                val codecInfoData = CodecInfo(
                                    name = codecName,
                                    mimeType = mimeType,
                                    isEncoder = isEncoder,
                                    isHardware = isHardware,
                                    maxWidth = maxWidth,
                                    maxHeight = maxHeight,
                                    profile = profile,
                                    level = level,
                                    performanceScore = performanceTime,
                                    priorityScore = priorityScore,
                                    lastValidated = if (validate) System.currentTimeMillis() else 0L, // 0L indicates not fully validated
                                    hdrSupport = hdrSupport,
                                    colorFormats = colorFormats,
                                    maxBitrate = maxBitrate
                                )

                                collectedCodecs.add(codecInfoData)

                                enumeratedCount++
                                // Don't break - continue to enumerate all supported types for this codec
                            }

                            processedCount++

                        } catch (e: Exception) {
                            Log.w(TAG, "❌ [CODEC_ERROR] Error processing codec ${codecInfo.name}: ${e.message}", e)
                            errorCount++
                        }
                    }
                }

                val totalTime = System.currentTimeMillis() - totalStartTime
                Log.i(TAG, "🎯 [SELECTIVE_ENUM_COMPLETE] $enumeratedCount decoders found in ${totalTime}ms")
                Log.i(TAG, "📈 [ENUMERATION_STATS] Processed: $processedCount, Skipped encoders: $skippedEncoders, Skipped non-video: $skippedNonVideo, Skipped validation: $skippedValidation, Errors: $errorCount")
                Log.i(TAG, "⏱️ [PERFORMANCE] Codec processing: ${codecProcessingTime}ms")

            } catch (e: Exception) {
                val errorTime = System.currentTimeMillis() - totalStartTime
                Log.e(TAG, "❌ [ENUMERATION_FAILED] Fatal error after ${errorTime}ms: ${e.message}", e)
            }
        }

        /**
         * Enhanced enumeration with caching support
         */
        @JvmStatic
        fun enumerateAllCodecsWithCache() {
            // Try to load from cache first
            val cachedResult = loadEnumerationResults()
            if (cachedResult != null && !shouldRefreshEnumeration()) {
                Log.i(TAG, "💾 [CACHE_HIT] Using cached enumeration results")

                // Enumerate cached codecs to native code
                for (codec in cachedResult.codecs) {
                    CodecEnumerator.storeCodecInfo(
                        codec.name, codec.mimeType, codec.isEncoder, codec.isHardware,
                        codec.maxWidth, codec.maxHeight, codec.profile, codec.level,
                        codec.hdrSupport.toTypedArray(), codec.colorFormats.toTypedArray(),
                        codec.maxBitrate
                    )
                }

                CodecEnumerator.signalEnumerationComplete(cachedResult.codecs.size)
                return
            }

            Log.i(TAG, "🔄 [CACHE_MISS] Performing fresh enumeration")

            // Perform fresh enumeration and collect results
            val enumeratedCodecs = mutableListOf<CodecInfo>()
            val enumerationStartTime = System.currentTimeMillis()

            // Modified enumeration to collect results
            enumerateAndCollect(enumeratedCodecs)

            val enumerationTime = System.currentTimeMillis() - enumerationStartTime

            // Save to cache
            if (enumeratedCodecs.isNotEmpty()) {
                saveEnumerationResults(enumeratedCodecs, enumerationTime)
            }
        }

        /**
         * Intelligent codec selection based on video format requirements
         * Prioritizes hardware acceleration, then performance metrics
         */
        @JvmStatic
        fun selectOptimalCodec(
            mimeType: String,
            width: Int,
            height: Int,
            bitrate: Int = 0,
            requireHardware: Boolean = true
        ): CodecInfo? {
            Log.i(TAG, "🎯 [CODEC_SELECTION] Selecting optimal codec for $mimeType ${width}x$height (HW=$requireHardware)")

            val cachedResult = loadEnumerationResults()
            if (cachedResult == null) {
                Log.w(TAG, "⚠️ [SELECTION_FAIL] No cached codec data available")
                return null
            }

            // Filter codecs by requirements
            val candidates = cachedResult.codecs.filter { codec ->
                codec.mimeType == mimeType &&
                codec.maxWidth >= width &&
                codec.maxHeight >= height &&
                (!requireHardware || codec.isHardware) // Respect hardware requirement
            }

            if (candidates.isEmpty()) {
                Log.w(TAG, "⚠️ [SELECTION_FAIL] No codecs match requirements")
                return null
            }

            Log.i(TAG, "📋 [CANDIDATES] Found ${candidates.size} candidate codecs")

            // Sort by priority: priority score > hardware > performance > resolution capability
            val sortedCandidates = candidates.sortedWith(compareByDescending<CodecInfo> { it.priorityScore } // Higher priority score is better
                .thenByDescending { it.isHardware }
                .thenBy { it.performanceScore } // Lower performance score (faster) is better
                .thenByDescending { it.maxWidth * it.maxHeight }) // Higher resolution capability

            val selected = sortedCandidates.first()
            Log.i(TAG, "✅ [SELECTION_SUCCESS] Selected: ${selected.name} (${selected.mimeType}, HW=${selected.isHardware}, ${selected.maxWidth}x${selected.maxHeight}, perf=${selected.performanceScore}ms)")
            return selected
        }

        /**
         * Create prioritized fallback codec chains for hardware-only codecs
         */
        @JvmStatic
        fun createHardwareFallbackChain(mimeType: String): List<CodecInfo> {
            Log.i(TAG, "🔗 [FALLBACK_CHAIN] Creating hardware-only fallback chain for $mimeType")

            val cachedResult = loadEnumerationResults()
            if (cachedResult == null) {
                Log.w(TAG, "⚠️ [FALLBACK_FAIL] No cached codec data available")
                return emptyList()
            }

            // Get only hardware codecs for this MIME type
            val hardwareCodecs = cachedResult.codecs.filter { codec ->
                codec.mimeType == mimeType && codec.isHardware
            }

            if (hardwareCodecs.isEmpty()) {
                Log.w(TAG, "⚠️ [FALLBACK_FAIL] No hardware codecs available for $mimeType")
                return emptyList()
            }

            // Sort by chipset preference and performance
            val chipsetPriority = mapOf(
                "qualcomm" to 1, "qcom" to 1,
                "mediatek" to 2, "mt" to 2,
                "samsung" to 3, "exynos" to 3,
                "huawei" to 4, "kirin" to 4,
                "google" to 5,
                "intel" to 6,
                "nvidia" to 7
            )

            val sortedCodecs = hardwareCodecs.sortedWith(compareBy<CodecInfo> {
                // Primary sort: chipset priority
                val codecName = it.name.lowercase()
                chipsetPriority.entries.firstOrNull { (key, _) ->
                    codecName.contains(key)
                }?.value ?: 99
            }.thenBy {
                // Secondary sort: performance score
                it.performanceScore
            }.thenByDescending {
                // Tertiary sort: resolution capability
                it.maxWidth * it.maxHeight
            })

            Log.i(TAG, "✅ [FALLBACK_CHAIN] Created chain with ${sortedCodecs.size} hardware codecs:")
            sortedCodecs.forEachIndexed { index, codec ->
                Log.i(TAG, "  ${index + 1}. ${codec.name} (perf: ${codec.performanceScore}ms)")
            }

            return sortedCodecs
        }

        /**
         * Get codec selection recommendations for different video formats
         */
        @JvmStatic
        fun getCodecRecommendations(): Map<String, List<CodecInfo>> {
            val recommendations = mutableMapOf<String, List<CodecInfo>>()

            // Define common video formats
            val formats = listOf(
                MediaFormat.MIMETYPE_VIDEO_AVC,   // H.264
                MediaFormat.MIMETYPE_VIDEO_HEVC,  // H.265/HEVC
                MediaFormat.MIMETYPE_VIDEO_VP9,   // VP9
                MediaFormat.MIMETYPE_VIDEO_AV1    // AV1
            )

            for (format in formats) {
                val fallbackChain = createHardwareFallbackChain(format)
                if (fallbackChain.isNotEmpty()) {
                    recommendations[format] = fallbackChain
                }
            }

            Log.i(TAG, "📊 [RECOMMENDATIONS] Generated codec recommendations for ${recommendations.size} formats")
            return recommendations
        }

        /**
         * Check if modern codec support is available (AV1, HEVC HDR, VP9 HDR)
         */
        @JvmStatic
        fun checkModernCodecSupport(): Map<String, Boolean> {
            val cachedResult = loadEnumerationResults() ?: return emptyMap()

            val modernCodecs = mapOf(
                "AV1" to MediaFormat.MIMETYPE_VIDEO_AV1,
                "HEVC" to MediaFormat.MIMETYPE_VIDEO_HEVC,
                "VP9" to MediaFormat.MIMETYPE_VIDEO_VP9
            )

            val support = mutableMapOf<String, Boolean>()

            for ((codecName, mimeType) in modernCodecs) {
                val hasSupport = cachedResult.codecs.any { codec ->
                    codec.mimeType == mimeType && codec.isHardware
                }
                support[codecName] = hasSupport
                Log.i(TAG, "🎨 [MODERN_SUPPORT] $codecName hardware support: $hasSupport")
            }

            // Check for HDR support (simplified check)
            val hasHDR = cachedResult.codecs.any { codec ->
                val capabilities = try {
                    MediaCodecList(MediaCodecList.ALL_CODECS).codecInfos
                        .find { it.name == codec.name }
                        ?.getCapabilitiesForType(codec.mimeType)
                } catch (e: Exception) { null }

                capabilities?.let {
                    // Check for HDR capabilities in profile levels
                    it.profileLevels.any { profileLevel ->
                        when (codec.mimeType) {
                            MediaFormat.MIMETYPE_VIDEO_HEVC -> profileLevel.profile >= MediaCodecInfo.CodecProfileLevel.HEVCProfileMain10
                            MediaFormat.MIMETYPE_VIDEO_VP9 -> profileLevel.profile >= MediaCodecInfo.CodecProfileLevel.VP9Profile2
                            else -> false
                        }
                    }
                } ?: false
            }

            support["HDR"] = hasHDR
            Log.i(TAG, "🎨 [MODERN_SUPPORT] HDR support: $hasHDR")

            return support
        }

        /**
         * Force cache refresh (useful when device capabilities change)
         */
        @JvmStatic
        fun refreshCodecCache() {
            Log.i(TAG, "🔄 [CACHE_REFRESH] Forcing codec cache refresh")
            clearEnumerationCache()
            enumerateAllCodecsWithCache()
        }

        /**
         * Perform enumeration and collect results for caching
         */
        private fun enumerateAndCollect(collectedCodecs: MutableList<CodecInfo>) {
            val totalStartTime = System.currentTimeMillis()
            Log.i(TAG, "🎯 [CODEC_ENUM_START] Starting MediaCodec enumeration (thread: ${Thread.currentThread().name})")

            try {
                val codecListTime = measureTimeMillis {
                    val codecList = MediaCodecList(MediaCodecList.ALL_CODECS)
                    codecList.codecInfos
                }

                val codecs = MediaCodecList(MediaCodecList.ALL_CODECS).codecInfos
                Log.i(TAG, "📊 [CODEC_LIST_ACQUIRED] Found ${codecs.size} total codecs in ${codecListTime}ms")

                var enumeratedCount = 0
                var processedCount = 0
                var skippedEncoders = 0
                var skippedNonVideo = 0
                var skippedBlacklisted = 0
                var errorCount = 0

                val codecProcessingTime = measureTimeMillis {
                    for ((index, codecInfo) in codecs.withIndex()) {
                        try {
                            val codecStartTime = System.currentTimeMillis()
                            val codecName = codecInfo.name
                            val isEncoder = codecInfo.isEncoder
                            val isHardware = getIsHardwareAccelerated(codecInfo)

                            Log.v(TAG, "🔍 [CODEC_CHECK_$index] Processing: $codecName (encoder=$isEncoder, hardware=$isHardware)")

                            // Skip blacklisted codecs
                            if (shouldSkipCodec(codecName)) {
                                Log.v(TAG, "🚫 [BLACKLIST_SKIP] Skipping blacklisted codec: $codecName")
                                skippedBlacklisted++
                                continue
                            }

                            // Skip encoders - we only want decoders for now
                            if (isEncoder) {
                                Log.v(TAG, "⏭️ [ENCODER_SKIP] Skipping encoder: $codecName")
                                skippedEncoders++
                                continue
                            }

                            // Get supported types for this codec
                            val supportedTypes = codecInfo.supportedTypes
                            Log.v(TAG, "📋 [TYPES_CHECK] $codecName supports ${supportedTypes.size} types")

                            for ((typeIndex, mimeType) in supportedTypes.withIndex()) {
                                Log.v(TAG, "📺 [MIME_CHECK_$typeIndex] Checking: $mimeType for $codecName")

                                // Check if this is a video MIME type
                                if (!mimeType.startsWith("video/")) {
                                    Log.v(TAG, "⏭️ [NON_VIDEO_SKIP] Skipping non-video: $mimeType")
                                    skippedNonVideo++
                                    continue
                                }

                                val capabilitiesTime = measureTimeMillis {
                                    codecInfo.getCapabilitiesForType(mimeType)
                                }

                                val capabilities = codecInfo.getCapabilitiesForType(mimeType)
                                if (capabilities == null) {
                                    Log.w(TAG, "⚠️ [NO_CAPABILITIES] $codecName has no capabilities for $mimeType (${capabilitiesTime}ms)")
                                    continue
                                }

                                Log.v(TAG, "✅ [CAPABILITIES_OK] $codecName capabilities acquired in ${capabilitiesTime}ms")

                                // Get video capabilities
                                val videoCaps = capabilities.videoCapabilities
                                if (videoCaps == null) {
                                    Log.w(TAG, "⚠️ [NO_VIDEO_CAPS] $codecName has no video capabilities")
                                    continue
                                }

                                // Two-stage validation: lightweight first, then full validation
                                val validationTime = measureTimeMillis {
                                    // Stage 1: Lightweight validation (fast)
                                    if (!validateCodecCapabilityLightweight(codecInfo, mimeType)) {
                                        Log.w(TAG, "⚠️ [LIGHTWEIGHT_VALIDATION_SKIP] Skipping codec with failed lightweight validation: $codecName for $mimeType")
                                        continue
                                    }

                                    // Stage 2: Full validation (expensive, but only for promising codecs)
                                    if (!validateCodecCapability(codecInfo, mimeType)) {
                                        Log.w(TAG, "⚠️ [FULL_VALIDATION_SKIP] Skipping codec with failed full validation: $codecName for $mimeType")
                                        continue
                                    }
                                }

                                Log.v(TAG, "✅ [VALIDATION_PASSED] Codec validated in ${validationTime}ms")

                                // Get supported resolutions
                                val maxWidth = videoCaps.supportedWidths.upper
                                val maxHeight = videoCaps.supportedHeights.upper

                                // Extract real profile/level from capabilities instead of heuristic approach
                                val (profile, level) = extractRealProfileLevel(codecInfo, mimeType)

                                // Extract HDR support information
                                val hdrSupport = extractHDRSupport(codecInfo, mimeType)

                                // Extract color format information
                                val colorFormats = extractColorFormats(codecInfo, mimeType)

                                // Extract maximum bitrate information
                                val maxBitrate = extractMaxBitrate(codecInfo, mimeType)

                                // Measure performance for caching
                                val performanceTime = measureCodecPerformance(codecInfo, mimeType)
                                Log.v(TAG, "⏱️ [PERFORMANCE_MEASURED] ${codecInfo.name} performance: ${performanceTime}ms")

                                Log.i(TAG, "✅ [DECODER_VALIDATED] $codecName ($mimeType, HW=$isHardware, ${maxWidth}x$maxHeight, P=$profile L=$level, HDR=${hdrSupport.joinToString(",")}, Colors=${colorFormats.joinToString(",")}, Bitrate=${maxBitrate}bps, validation=${validationTime}ms, ${System.currentTimeMillis() - codecStartTime}ms)")

                                // Calculate priority score for this codec
                                val priorityScore = calculateCodecPriority(codecName)

                                // Create codec info for collection
                                val codecInfoData = CodecInfo(
                                    name = codecName,
                                    mimeType = mimeType,
                                    isEncoder = isEncoder,
                                    isHardware = isHardware,
                                    maxWidth = maxWidth,
                                    maxHeight = maxHeight,
                                    profile = profile,
                                    level = level,
                                    performanceScore = performanceTime,
                                    priorityScore = priorityScore,
                                    lastValidated = System.currentTimeMillis(),
                                    hdrSupport = hdrSupport,
                                    colorFormats = colorFormats,
                                    maxBitrate = maxBitrate
                                )

                                collectedCodecs.add(codecInfoData)

                                // Bridge to native code - store codec info in native memory
                                val nativeCallTime = measureTimeMillis {
                                    CodecEnumerator.storeCodecInfo(codecName, mimeType, isEncoder, isHardware,
                                                    maxWidth, maxHeight, profile, level,
                                                    hdrSupport.toTypedArray(), colorFormats.toTypedArray(),
                                                    maxBitrate)
                                }

                                Log.v(TAG, "🌉 [NATIVE_BRIDGE] JNI call completed in ${nativeCallTime}ms")

                                enumeratedCount++
                                // Don't break - continue to enumerate all supported types for this codec
                            }

                            processedCount++

                        } catch (e: Exception) {
                            Log.w(TAG, "❌ [CODEC_ERROR] Error processing codec ${codecInfo.name}: ${e.message}", e)
                            errorCount++
                        }
                    }
                }

                val totalTime = System.currentTimeMillis() - totalStartTime
                Log.i(TAG, "🎯 [ENUMERATION_COMPLETE] $enumeratedCount decoders found in ${totalTime}ms")
                Log.i(TAG, "📈 [ENUMERATION_STATS] Processed: $processedCount, Skipped encoders: $skippedEncoders, Skipped non-video: $skippedNonVideo, Errors: $errorCount")
                Log.i(TAG, "⏱️ [PERFORMANCE] Codec processing: ${codecProcessingTime}ms, JNI calls: ${enumeratedCount * 2}ms estimated")

                // Notify native code that enumeration is complete
                val completionTime = measureTimeMillis {
                    CodecEnumerator.signalEnumerationComplete(enumeratedCount)
                }

                Log.i(TAG, "✅ [FINAL_CALLBACK] Enumeration completion notified in ${completionTime}ms")

            } catch (e: Exception) {
                val errorTime = System.currentTimeMillis() - totalStartTime
                Log.e(TAG, "❌ [ENUMERATION_FAILED] Fatal error after ${errorTime}ms: ${e.message}", e)
                CodecEnumerator.signalEnumerationComplete(0)
            }
        }
}
}