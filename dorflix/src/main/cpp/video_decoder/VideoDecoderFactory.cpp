#include "VideoDecoderFactory.h"
#include "MediaCodecHardwareDecoder.h"
#include "FFmpegDecoder.h"
#include "../jni/jni_interface.h"
#include <media/NdkMediaCodec.h>
#include <android/log.h>
#include <algorithm>
#include <string>
#include <vector>

// Static storage for selected codec
static EnumeratedCodecInfo s_selectedCodec;

DecoderType VideoDecoderFactory::selectBestDecoder(const std::string& codecName, AVCodecID codecId) {
    LOGI_DECODER("🎯 HYBRID APPROACH: Selecting best decoder for %s (codec_id: %d)", codecName.c_str(), codecId);

    // Get MIME type for this codec
    const char* mimeType = getMimeType(codecId);
    if (!mimeType) {
        LOGE_DECODER("❌ HYBRID APPROACH: No MIME type mapping for codec_id %d", codecId);
        return DecoderType::NONE;
    }

    LOGI_DECODER("🎯 HYBRID APPROACH: MIME type is %s", mimeType);

    // ===== USE ENUMERATED CODEC DATA FOR INTELLIGENT SELECTION =====
    // Query the enumerated codecs to find the best matching hardware decoder
    EnumeratedCodecInfo selectedCodec = selectBestEnumeratedCodec(mimeType, 1920, 1080); // Default to 1080p

    if (selectedCodec.isValid) {
        LOGI_DECODER("✅ HYBRID APPROACH: Selected enumerated codec: %s (HW=%d, %dx%d, perf=%ldms)",
                    selectedCodec.name.c_str(), selectedCodec.isHardware,
                    selectedCodec.maxWidth, selectedCodec.maxHeight, selectedCodec.performanceScore);

        // Store the selected codec for later use
        s_selectedCodec = selectedCodec;

        // Check if the selected codec can actually be created
        if (isEnumeratedCodecSupported(selectedCodec, mimeType)) {
            LOGI_DECODER("✅ HYBRID APPROACH: Enumerated codec is supported - SELECTING MEDIACODEC_HARDWARE");
            return DecoderType::MEDIACODEC_HARDWARE;
        } else {
            LOGW_DECODER("⚠️ HYBRID APPROACH: Selected enumerated codec not supported - trying fallback");
        }
    } else {
        LOGW_DECODER("⚠️ HYBRID APPROACH: No suitable enumerated codec found - falling back to basic check");
    }

    // ===== FALLBACK: Basic MediaCodec availability check =====
    LOGI_DECODER("🎯 HYBRID APPROACH: Using fallback MediaCodec availability check");
    if (isMediaCodecHardwareSupported(codecName, codecId)) {
        LOGI_DECODER("✅ HYBRID APPROACH: MediaCodec hardware available via fallback - SELECTING MEDIACODEC_HARDWARE");
        return DecoderType::MEDIACODEC_HARDWARE;
    } else {
        LOGE_DECODER("❌ HYBRID APPROACH: No MediaCodec hardware support available for %s", codecName.c_str());
        return DecoderType::NONE;
    }
}

std::unique_ptr<VideoDecoderBase> VideoDecoderFactory::createDecoder(DecoderType type) {
    LOGI_DECODER("Creating decoder of type: %d", static_cast<int>(type));

    switch (type) {
        case DecoderType::MEDIACODEC_HARDWARE:
            LOGI_DECODER("Creating MediaCodec hardware decoder");
            return std::make_unique<MediaCodecHardwareDecoder>();

        case DecoderType::FFMPEG_SOFTWARE:
            LOGI_DECODER("Creating FFmpeg software decoder");
            return std::make_unique<FFmpegDecoder>();

        default:
            LOGE_DECODER("Unknown decoder type requested: %d", static_cast<int>(type));
            return nullptr;
    }
}

bool VideoDecoderFactory::isMediaCodecHardwareSupported(const std::string& codecName, AVCodecID codecId) {
    LOGI_DECODER("🎯 MEDIACODEC_HW_FIRST: Checking MediaCodec HW for %s (codec_id: %d)", codecName.c_str(), codecId);

    // Get MIME type
    const char* mimeType = getMimeType(codecId);
    if (!mimeType) {
        LOGW_DECODER("🎯 MEDIACODEC_HW_FIRST: No MIME type for codec %s (codec_id: %d)", codecName.c_str(), codecId);
        return false;
    }

    LOGI_DECODER("🎯 MEDIACODEC_HW_FIRST: MIME type for %s is %s", codecName.c_str(), mimeType);

    // Try to create MediaCodec decoder
    LOGI_DECODER("🎯 MEDIACODEC_HW_FIRST: Attempting to create MediaCodec decoder for %s...", mimeType);
    AMediaCodec* codec = AMediaCodec_createDecoderByType(mimeType);
    if (!codec) {
        LOGW_DECODER("🎯 MEDIACODEC_HW_FIRST: ❌ AMediaCodec_createDecoderByType(%s) returned NULL - HW not available", mimeType);
        LOGW_DECODER("🎯 MEDIACODEC_HW_FIRST: Possible reasons:");
        LOGW_DECODER("🎯 MEDIACODEC_HW_FIRST:   - Device doesn't have HW decoder for this codec");
        LOGW_DECODER("🎯 MEDIACODEC_HW_FIRST:   - MediaCodec NDK library not properly linked");
        LOGW_DECODER("🎯 MEDIACODEC_HW_FIRST:   - Codec not supported in current Android version");
        LOGW_DECODER("🎯 MEDIACODEC_HW_FIRST:   - HW decoder disabled/broken on this device");
        return false;
    }

    // Clean up
    AMediaCodec_delete(codec);
    LOGI_DECODER("🎯 MEDIACODEC_HW_FIRST: ✅ MediaCodec HW decoder successfully created and deleted for %s", mimeType);

    return true;
}



const char* VideoDecoderFactory::getMimeType(AVCodecID codecId) {
    switch (codecId) {
        case AV_CODEC_ID_H264:
            return "video/avc";  // H.264
        case AV_CODEC_ID_HEVC:
            return "video/hevc"; // H.265/HEVC
        case AV_CODEC_ID_VP8:
            return "video/x-vnd.on2.vp8";  // VP8
        case AV_CODEC_ID_VP9:
            return "video/x-vnd.on2.vp9";  // VP9
        case AV_CODEC_ID_MPEG4:
            return "video/mp4v-es"; // MPEG-4
        case AV_CODEC_ID_H263:
            return "video/3gpp";   // H.263
        default:
            return nullptr;
    }
}

// ===== ENUMERATION-BASED CODEC SELECTION =====
EnumeratedCodecInfo VideoDecoderFactory::selectBestEnumeratedCodec(const char* mimeType, int requiredWidth, int requiredHeight) {
    LOGI_DECODER("🎯 Selecting best enumerated codec for %s (%dx%d)", mimeType, requiredWidth, requiredHeight);

    // Get enumerated codecs from JNI bridge
    auto enumeratedCodecs = getEnumeratedCodecs();

    if (enumeratedCodecs.empty()) {
        LOGW_DECODER("⚠️ No enumerated codecs available - enumeration may not have completed");
        EnumeratedCodecInfo empty;
        return empty;
    }

    LOGI_DECODER("📊 Found %zu enumerated codecs, filtering for %s decoders", enumeratedCodecs.size(), mimeType);

    // Filter codecs: video decoders only, matching MIME type, hardware preferred
    std::vector<EnumeratedCodecInfo> candidates;

    for (const auto& codecInfo : enumeratedCodecs) {
        // Skip encoders, non-matching MIME types
        if (codecInfo.isEncoder || codecInfo.mimeType != mimeType) {
            continue;
        }

        // Check resolution capability
        if (codecInfo.maxWidth < requiredWidth || codecInfo.maxHeight < requiredHeight) {
            LOGD_DECODER("Skipping %s - insufficient resolution: %dx%d < %dx%d",
                        codecInfo.name.c_str(), codecInfo.maxWidth, codecInfo.maxHeight, requiredWidth, requiredHeight);
            continue;
        }

        // Create candidate with preserved performance score from Java enumeration
        EnumeratedCodecInfo candidate = codecInfo;
        candidate.isValid = true;

        candidates.push_back(candidate);
        LOGD_DECODER("✅ Candidate codec: %s (HW=%d, %dx%d)",
                    codecInfo.name.c_str(), codecInfo.isHardware, codecInfo.maxWidth, codecInfo.maxHeight);
    }

    if (candidates.empty()) {
        LOGW_DECODER("⚠️ No suitable enumerated codecs found for %s %dx%d", mimeType, requiredWidth, requiredHeight);
        EnumeratedCodecInfo empty;
        return empty;
    }

    LOGI_DECODER("📋 Found %zu candidate codecs, selecting best one", candidates.size());

    // Sort candidates by priority: hardware > performance score > resolution capability
    // This matches the Kotlin selectOptimalCodec() logic for consistent behavior
    std::sort(candidates.begin(), candidates.end(), [](const EnumeratedCodecInfo& a, const EnumeratedCodecInfo& b) {
        // Hardware acceleration first
        if (a.isHardware != b.isHardware) {
            return a.isHardware > b.isHardware;
        }

        // Then by performance score (lower = faster/better, -1 means not measured so treated as worst)
        long aPerf = a.performanceScore >= 0 ? a.performanceScore : LONG_MAX;
        long bPerf = b.performanceScore >= 0 ? b.performanceScore : LONG_MAX;
        if (aPerf != bPerf) {
            return aPerf < bPerf; // Lower performance score (faster) is better
        }

        // Finally by resolution capability (higher resolution preferred)
        int aResolution = a.maxWidth * a.maxHeight;
        int bResolution = b.maxWidth * b.maxHeight;
        if (aResolution != bResolution) {
            return aResolution > bResolution;
        }

        // Last resort: by name for consistent selection
        return a.name < b.name;
    });

    EnumeratedCodecInfo selected = candidates[0];
    LOGI_DECODER("✅ Selected enumerated codec: %s (HW=%d, %dx%d, profile=%d, level=%d)",
                selected.name.c_str(), selected.isHardware, selected.maxWidth, selected.maxHeight,
                selected.profile, selected.level);

    return selected;
}

bool VideoDecoderFactory::isEnumeratedCodecSupported(const EnumeratedCodecInfo& codec, const char* mimeType) {
    LOGI_DECODER("🔍 Checking if enumerated codec %s is actually supported", codec.name.c_str());

    // For now, just try to create a MediaCodec with the codec name
    // In the future, this could be enhanced to use AMediaCodec_createByCodecName
    AMediaCodec* testCodec = AMediaCodec_createDecoderByType(mimeType);
    if (!testCodec) {
        LOGW_DECODER("❌ Enumerated codec %s not supported - cannot create MediaCodec for %s",
                    codec.name.c_str(), mimeType);
        return false;
    }

    // Clean up
    AMediaCodec_delete(testCodec);
    LOGI_DECODER("✅ Enumerated codec %s is supported", codec.name.c_str());
    return true;
}

// ===== GETTER FOR SELECTED CODEC =====
const EnumeratedCodecInfo& VideoDecoderFactory::getSelectedCodec() {
    return s_selectedCodec;
}