#include "MediaCodecHardwareDecoder.h"
#include "AddressSpaceDetector.h"
#include "MemoryConstrainedBufferManager.h"
#include "../jni/jni_interface.h"
#include <cstring>
#include <android/log.h>
#include <unistd.h>
#include <sys/system_properties.h>
#include <algorithm>

extern "C" {
#include <libswscale/swscale.h>
}

// Helper function to get pixel format name (since av_get_pix_fmt_name may not be available)
const char* getPixelFormatName(enum AVPixelFormat format) {
    switch (format) {
        case AV_PIX_FMT_YUV420P: return "YUV420P";
        case AV_PIX_FMT_NV12: return "NV12";
        case AV_PIX_FMT_NV21: return "NV21";
        case AV_PIX_FMT_YUV422P: return "YUV422P";
        case AV_PIX_FMT_YUV444P: return "YUV444P";
        case AV_PIX_FMT_BGRA: return "BGRA";
        case AV_PIX_FMT_RGBA: return "RGBA";
        case AV_PIX_FMT_RGB24: return "RGB24";
        case AV_PIX_FMT_BGR24: return "BGR24";
        default: return "UNKNOWN";
    }
}

// Logging macros
#define LOG_TAG_HW "MediaCodecHW"
#define LOGE_HW(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_HW, __VA_ARGS__)
#define LOGI_HW(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_HW, __VA_ARGS__)
#define LOGW_HW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG_HW, __VA_ARGS__)
#define LOGD_HW(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_HW, __VA_ARGS__)

#include "../jni/jni_interface.h"

// Forward declarations for enumeration-based functions
// These are member functions of MediaCodecHardwareDecoder



// ===== ENHANCED MEDIACODEC ERROR LOGGING =====
// Convert MediaCodec error codes to human-readable strings
const char* getMediaCodecErrorString(media_status_t status) {
    switch (status) {
        case AMEDIA_OK: return "AMEDIA_OK (success)";
        case AMEDIA_ERROR_UNKNOWN: return "AMEDIA_ERROR_UNKNOWN";
        case AMEDIA_ERROR_MALFORMED: return "AMEDIA_ERROR_MALFORMED (malformed media data)";
        case AMEDIA_ERROR_UNSUPPORTED: return "AMEDIA_ERROR_UNSUPPORTED (unsupported format/parameters)";
        case AMEDIA_ERROR_INVALID_OBJECT: return "AMEDIA_ERROR_INVALID_OBJECT (invalid MediaCodec object)";
        case AMEDIA_ERROR_INVALID_PARAMETER: return "AMEDIA_ERROR_INVALID_PARAMETER (invalid parameter)";
        case AMEDIA_ERROR_INVALID_OPERATION: return "AMEDIA_ERROR_INVALID_OPERATION (invalid operation)";
        case AMEDIA_ERROR_END_OF_STREAM: return "AMEDIA_ERROR_END_OF_STREAM";
        case AMEDIA_ERROR_IO: return "AMEDIA_ERROR_IO (I/O error)";
        case AMEDIA_ERROR_WOULD_BLOCK: return "AMEDIA_ERROR_WOULD_BLOCK";
        default: return "UNKNOWN_ERROR";
    }
}

// ===== COMPREHENSIVE SURFACE VALIDATION =====
bool validateSurfaceForMediaCodec(ANativeWindow* surface, const char* context) {
    if (!surface) {
        LOGE_HW("❌ SURFACE VALIDATION FAILED [%s]: Surface is null", context);
        return false;
    }

    // Get surface dimensions (ANativeWindow_isValid not available in this NDK version)
    int width = ANativeWindow_getWidth(surface);
    int height = ANativeWindow_getHeight(surface);

    if (width <= 0 || height <= 0) {
        LOGE_HW("❌ SURFACE VALIDATION FAILED [%s]: Invalid dimensions %dx%d (must be > 0)", context, width, height);
        return false;
    }

    // Check for reasonable maximum dimensions (4K limit)
    const int MAX_SURFACE_DIMENSION = 4096;
    if (width > MAX_SURFACE_DIMENSION || height > MAX_SURFACE_DIMENSION) {
        LOGE_HW("❌ SURFACE VALIDATION FAILED [%s]: Dimensions %dx%d exceed maximum allowed (%dx%d)",
                context, width, height, MAX_SURFACE_DIMENSION, MAX_SURFACE_DIMENSION);
        return false;
    }

    // Get surface format
    int format = ANativeWindow_getFormat(surface);
    LOGI_HW("✅ SURFACE VALIDATION [%s]: Surface dimensions valid - %dx%d format=%d", context, width, height, format);

    // Validate format compatibility with MediaCodec (common formats)
    switch (format) {
        case AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM:
        case AHARDWAREBUFFER_FORMAT_R8G8B8X8_UNORM:
        case AHARDWAREBUFFER_FORMAT_R5G6B5_UNORM:
        case AHARDWAREBUFFER_FORMAT_R10G10B10A2_UNORM:
            LOGI_HW("✅ SURFACE VALIDATION [%s]: Surface format %d is compatible with MediaCodec", context, format);
            break;
        default:
            LOGW_HW("⚠️ SURFACE VALIDATION [%s]: Surface format %d may not be optimal for MediaCodec", context, format);
            break;
    }

    return true;
}



MediaCodecHardwareDecoder::MediaCodecHardwareDecoder()
    : m_codec(nullptr)
    , m_surface(nullptr)
    , m_initialized(false)
    , m_width(0)
    , m_height(0)
    , m_codecId(AV_CODEC_ID_NONE)
    , m_swsContext(nullptr)
    , m_outputFrame(nullptr)
    , m_consecutiveFailures(0)
    , m_maxConsecutiveFailures(10)
    , m_consecutiveTryAgainLater(0)
    , m_maxPersistentTryAgainLater(50)  // Flush after 50 persistent TRY_AGAIN_LATER
    , m_inputBufferCount(0)
    , m_inputBufferSize(0)
    , m_maxPacketsPerSecond(30)  // Default conservative limit
    , m_minPacketIntervalUs(33333)  // ~30 FPS
    , m_lastPacketTimeUs(0)
    , m_stopQueueProcessor(false)
    , m_isInLoadingState(false)
    , m_surfaceMode(false)
    , m_watchdogTimeoutMs(300)  // 300ms timeout for stalled decoding
    , m_watchdogActive(false)
    , m_rateLimitState(RateLimitState::STARTUP)
    , m_startupBurstCount(0)
    , m_maxStartupBurst(5)  // Allow burst of 5 packets during startup
    , m_firstFrameReceived(false)
    , m_selectedCodec()  // Initialize empty codec info
{
    LOGI_HW("MediaCodecHardwareDecoder created");
    startQueueProcessor();
}



MediaCodecHardwareDecoder::~MediaCodecHardwareDecoder() {
    LOGI_HW("Destroying MediaCodecHardwareDecoder");

    // Stop queue processor first (it owns MediaCodec access)
    stopQueueProcessor();

    if (m_outputFrame) {
        av_frame_free(&m_outputFrame);
        m_outputFrame = nullptr;
    }

    if (m_swsContext) {
        sws_freeContext(m_swsContext);
        m_swsContext = nullptr;
    }

    if (m_codec) {
        AMediaCodec_stop(m_codec);
        AMediaCodec_delete(m_codec);
        m_codec = nullptr;
    }

    m_initialized = false;
    LOGI_HW("MediaCodecHardwareDecoder destroyed");
}

void MediaCodecHardwareDecoder::setOutputSurface(ANativeWindow* surface) {
    m_surface = surface;
    LOGI_HW("Output surface set: %p", surface);
}

void MediaCodecHardwareDecoder::setFrameDeliveryCallback(std::function<void(AVFrame* frame)> callback) {
    m_frameDeliveryCallback = callback;
    LOGI_HW("Frame delivery callback set successfully");
}

void MediaCodecHardwareDecoder::setSelectedCodecInfo(const EnumeratedCodecInfo& codecInfo) {
    m_selectedCodec = codecInfo;
    LOGI_HW("Selected codec info set: %s (HW=%d, %dx%d, profile=%d, level=%d)",
            codecInfo.name.c_str(), codecInfo.isHardware,
            codecInfo.maxWidth, codecInfo.maxHeight,
            codecInfo.profile, codecInfo.level);
}

bool MediaCodecHardwareDecoder::initialize(AVFormatContext* formatContext, int streamIndex) {
    LOGI_HW("=== INITIALIZING MEDIACODEC HARDWARE DECODER ===");

    if (!formatContext || streamIndex < 0 || streamIndex >= (int)formatContext->nb_streams) {
        LOGE_HW("Invalid format context or stream index");
        return false;
    }

    AVStream* stream = formatContext->streams[streamIndex];
    AVCodecParameters* codecParams = stream->codecpar;

    m_codecId = codecParams->codec_id;
    m_width = codecParams->width;
    m_height = codecParams->height;

    LOGI_HW("Codec: %s, Resolution: %dx%d", avcodec_get_name(m_codecId), m_width, m_height);

    // Get MIME type
    const char* mimeType = VideoDecoderFactory::getMimeType(m_codecId);
    if (!mimeType) {
        LOGE_HW("Unsupported codec for MediaCodec: %s", avcodec_get_name(m_codecId));
        return false;
    }

    // ===== PHASE 2: SIMPLIFIED MEDIACODEC INITIALIZATION WITH ENUMERATED CODECS =====
    LOGI_HW("🎯 PHASE 2: Using enumerated codec information for MediaCodec creation");

    // Always try enumerated codec first if available
    bool codecCreated = false;
    if (m_selectedCodec.isValid && !m_selectedCodec.name.empty()) {
        LOGI_HW("🎯 PHASE 2: Creating MediaCodec with enumerated codec: %s", m_selectedCodec.name.c_str());

        // Use AMediaCodec_createCodecByName for specific enumerated codec
        m_codec = AMediaCodec_createCodecByName(m_selectedCodec.name.c_str());
        if (m_codec) {
            LOGI_HW("✅ PHASE 2: Successfully created MediaCodec with enumerated codec: %s", m_selectedCodec.name.c_str());
            LOGI_HW("   → Hardware=%d, MaxRes=%dx%d, MIME=%s",
                    m_selectedCodec.isHardware, m_selectedCodec.maxWidth, m_selectedCodec.maxHeight,
                    m_selectedCodec.mimeType.c_str());
            codecCreated = true;
        } else {
            LOGW_HW("⚠️ PHASE 2: Failed to create MediaCodec with enumerated codec: %s", m_selectedCodec.name.c_str());
            LOGW_HW("   → Falling back to MIME type creation");
        }
    } else {
        LOGW_HW("⚠️ PHASE 2: No valid enumerated codec selected - using MIME type fallback");
        LOGW_HW("   → This indicates enumeration failed or no hardware codecs available");
    }

    // Fallback: Create MediaCodec decoder by MIME type
    if (!codecCreated) {
        LOGI_HW("🎯 PHASE 2: Creating MediaCodec by MIME type: %s", mimeType);
        m_codec = AMediaCodec_createDecoderByType(mimeType);
        if (!m_codec) {
            LOGE_HW("❌ PHASE 2: Failed to create MediaCodec decoder for %s", mimeType);
            LOGE_HW("   → Hardware decoding not available on this device");
            return false;
        }
        LOGI_HW("✅ PHASE 2: MediaCodec created by MIME type (enumeration-based selection failed)");
    }

    // Determine surface mode - critical for correct operation
    // Surface mode requires surface to be set before configuration
    m_surfaceMode = (m_surface != nullptr);
    LOGI_HW("🎯 PHASE 2: Operating in %s mode", m_surfaceMode ? "SURFACE" : "BUFFER");

    // Configure codec with enumerated codec optimization
    if (!configureCodec(mimeType, m_width, m_height, codecParams, formatContext)) {
        LOGE_HW("❌ PHASE 2: Failed to configure MediaCodec");
        AMediaCodec_delete(m_codec);
        m_codec = nullptr;
        return false;
    }

    // Start codec
    media_status_t status = AMediaCodec_start(m_codec);
    if (status != AMEDIA_OK) {
        LOGE_HW("❌ PHASE 2: Failed to start MediaCodec: %s", getMediaCodecErrorString(status));
        AMediaCodec_delete(m_codec);
        m_codec = nullptr;
        return false;
    }

    LOGI_HW("✅ PHASE 2: MediaCodec started successfully");

    // Setup frame conversion for FFmpeg compatibility (only needed in buffer mode)
    if (!m_surfaceMode) {
        if (!setupFrameConversion(m_width, m_height)) {
            LOGE_HW("❌ PHASE 2: Failed to setup frame conversion");
            AMediaCodec_stop(m_codec);
            AMediaCodec_delete(m_codec);
            m_codec = nullptr;
            return false;
        }
    } else {
        LOGI_HW("🎯 PHASE 2: Surface mode - skipping frame conversion setup");
    }

    // Query buffer capabilities for adaptive rate limiting
    if (!queryBufferCapabilities()) {
        LOGW_HW("⚠️ PHASE 2: Failed to query buffer capabilities - using defaults");
        // Continue with defaults rather than failing
    }

    m_initialized = true;
    LOGI_HW("✅ PHASE 2: MediaCodec hardware decoder initialized successfully");
    LOGI_HW("   → Mode: %s, Codec: %s, Resolution: %dx%d",
            m_surfaceMode ? "Surface" : "Buffer",
            codecCreated ? m_selectedCodec.name.c_str() : mimeType,
            m_width, m_height);
    return true;
}

bool MediaCodecHardwareDecoder::configureCodec(const char* mimeType, int width, int height, AVCodecParameters* codecParams, AVFormatContext* formatContext) {
    LOGI_HW("🎯 CAPABILITY-BASED MEDIACODEC CONFIGURATION");
    LOGI_HW("   → Video: %dx%d, MIME: %s", width, height, mimeType);
    LOGI_HW("   → Profile: %d, Level: %d", codecParams->profile, codecParams->level);

    // ===== CAPABILITY-BASED CSD CONFIGURATION =====
    LOGI_HW("📦 Extracting SPS/PPS and configuring CSD with capability-based selection");

    uint8_t* spsData = nullptr;
    size_t spsSize = 0;
    uint8_t* ppsData = nullptr;
    size_t ppsSize = 0;

    if (!extractSpsPpsFromExtradata(codecParams->extradata, codecParams->extradata_size,
                                    &spsData, &spsSize, &ppsData, &ppsSize)) {
        LOGE_HW("❌ Failed to extract SPS/PPS from extradata");
        return false;
    }

    LOGI_HW("   → ✅ Extracted SPS: %zu bytes, PPS: %zu bytes", spsSize, ppsSize);

    // Create MediaFormat with dynamic CSD format selection
    AMediaFormat* format = AMediaFormat_new();
    if (!format) {
        LOGE_HW("❌ Failed to create media format");
        av_free(spsData);
        av_free(ppsData);
        return false;
    }

    // Set basic format properties
    AMediaFormat_setString(format, AMEDIAFORMAT_KEY_MIME, mimeType);
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_WIDTH, width);
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_HEIGHT, height);
    AMediaFormat_setInt32(format, "priority", 0);
    AMediaFormat_setInt32(format, "vendor.qti.media.decoder.force_sw", 0);
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_MAX_INPUT_SIZE, 2097152);

    // ===== ENUMERATION-BASED CSD FORMAT SELECTION =====
    // Use Java bridge enumeration results to determine optimal CSD format
    LOGI_HW("🎯 Waiting for codec enumeration to determine CSD format...");
    if (!ensureCodecEnumeration()) {
        LOGW_HW("⚠️ Codec enumeration failed or timed out - using conservative defaults");
        LOGW_HW("   → This may result in suboptimal MediaCodec configuration");
        LOGW_HW("   → Consider checking Java enumeration in DorflixApplication.onCreate()");
    }
    bool useAnnexB = shouldUseAnnexBFormat(mimeType);
    LOGI_HW("📋 CSD format decision: %s for %s", useAnnexB ? "Annex B (start codes)" : "Raw NAL units", mimeType);

    uint8_t* csd0Data = nullptr;
    size_t csd0Size = 0;
    uint8_t* csd1Data = nullptr;
    size_t csd1Size = 0;

    if (useAnnexB) {
        LOGI_HW("📋 Enumeration indicates Annex B CSD format required");
        // Add start codes for Annex B format
        csd0Data = addStartCode(spsData, spsSize, &csd0Size);
        csd1Data = addStartCode(ppsData, ppsSize, &csd1Size);
    } else {
        LOGI_HW("📋 Enumeration indicates raw NAL units CSD format");
        // Use raw NAL units directly
        csd0Data = spsData;
        csd0Size = spsSize;
        csd1Data = ppsData;
        csd1Size = ppsSize;
    }

    if (!csd0Data || !csd1Data) {
        LOGE_HW("❌ Failed to prepare CSD data for format: %s", useAnnexB ? "Annex B" : "Raw NAL");
        if (useAnnexB) {
            if (csd0Data) av_free(csd0Data);
            if (csd1Data) av_free(csd1Data);
        }
        av_free(spsData);
        av_free(ppsData);
        return false;
    }

    AMediaFormat_setBuffer(format, "csd-0", csd0Data, csd0Size);
    AMediaFormat_setBuffer(format, "csd-1", csd1Data, csd1Size);
    LOGI_HW("   → Set CSD (%s): csd-0 (%zu bytes SPS), csd-1 (%zu bytes PPS)",
            useAnnexB ? "Annex B" : "raw NAL units", csd0Size, csd1Size);

    auto configStartTime = std::chrono::steady_clock::now();
    media_status_t status = AMediaCodec_configure(m_codec, format, m_surface, nullptr, 0);
    auto configEndTime = std::chrono::steady_clock::now();
    auto configDuration = std::chrono::duration_cast<std::chrono::milliseconds>(configEndTime - configStartTime);

    if (status == AMEDIA_OK) {
        LOGI_HW("✅ MediaCodec configuration SUCCESSFUL!");
        LOGI_HW("   → Configuration time: %" PRId64 "ms", (int64_t)configDuration.count());
        LOGI_HW("   → CSD format: %s", useAnnexB ? "Annex B" : "Raw NAL units");
        LOGI_HW("   → Resolution: %dx%d", width, height);

        AMediaFormat_delete(format);
        if (useAnnexB) {
            av_free(csd0Data);
            av_free(csd1Data);
        }
        av_free(spsData);
        av_free(ppsData);
        return true;
    }

    // ===== CONFIGURATION FAILURE =====
    LOGE_HW("❌ MediaCodec configuration failed (status=%s)", getMediaCodecErrorString(status));
    LOGE_HW("   → CSD format attempted: %s", useAnnexB ? "Annex B" : "Raw NAL units");
    LOGE_HW("   → This codec may not be supported or enumeration data may be incorrect");

    AMediaFormat_delete(format);
    if (useAnnexB) {
        av_free(csd0Data);
        av_free(csd1Data);
    }
    av_free(spsData);
    av_free(ppsData);
    return false;
}

// ===== ADD START CODE TO RAW NAL UNIT =====
// Adds Annex B start code (0x00000001) to raw NAL unit data
uint8_t* MediaCodecHardwareDecoder::addStartCode(const uint8_t* nalData, size_t nalSize, size_t* outputSize) {
    if (!nalData || nalSize == 0) {
        LOGE_HW("❌ Invalid NAL data for start code addition");
        *outputSize = 0;
        return nullptr;
    }

    // Allocate buffer for NAL data + 4-byte start code
    *outputSize = nalSize + 4;
    uint8_t* outputBuffer = (uint8_t*)av_malloc(*outputSize);
    if (!outputBuffer) {
        LOGE_HW("❌ Failed to allocate buffer for start code addition");
        *outputSize = 0;
        return nullptr;
    }

    // Add start code (0x00000001)
    outputBuffer[0] = 0x00;
    outputBuffer[1] = 0x00;
    outputBuffer[2] = 0x00;
    outputBuffer[3] = 0x01;

    // Copy NAL data
    memcpy(outputBuffer + 4, nalData, nalSize);

    LOGD_HW("✅ Added start code to NAL unit: %zu bytes → %zu bytes", nalSize, *outputSize);
    return outputBuffer;
}

bool MediaCodecHardwareDecoder::setupFrameConversion(int width, int height) {
    LOGI_HW("Frame conversion setup complete - frames allocated per decode operation");
    return true;
}

AVFrame* MediaCodecHardwareDecoder::decodePacket(AVPacket* packet) {
    if (!m_initialized) {
        LOGW_HW("Decoder not ready");
        return nullptr;
    }

    // ===== COMPREHENSIVE PACKET VALIDATION =====
    // Reject corrupted packets before any processing to prevent crashes

    // NULL packet check
    if (!packet) {
        LOGW_HW("❌ NULL packet received - rejecting");
        return nullptr;
    }

    // EOS handling (must come after NULL check)
    if (packet->size == 0) {
        LOGI_HW("🎬 EOS DETECTED: End of stream signal received");

        // Queue EOS packet for processing
        LOGI_HW("📤 Queuing EOS packet for MediaCodec processing");
        queuePacketForProcessing(nullptr, false, false); // nullptr indicates EOS

        // Return nullptr - EOS handling happens asynchronously
        return nullptr;
    }

    // Size validation - reject invalid sizes
    if (packet->size <= 0 || packet->size > 10 * 1024 * 1024) { // > 10MB is suspicious
        LOGW_HW("❌ Invalid packet size: %d - rejecting", packet->size);
        return nullptr;
    }

    // Data pointer validation - critical for crash prevention
    if (!packet->data) {
        LOGW_HW("❌ Packet data pointer is null - rejecting");
        return nullptr;
    }

    // Check for known crash addresses in packet data
    uintptr_t packetDataPtr = reinterpret_cast<uintptr_t>(packet->data);
#if UINTPTR_MAX == UINT64_MAX
    if (packetDataPtr == 0x100000000ULL) {
        LOGW_HW("❌ CRITICAL: Packet data points to crash address 0x100000000 - rejecting");
        return nullptr;
    }
#endif

    // PTS validation - reject obviously corrupted timestamps
    if (packet->pts == INT64_MIN || packet->pts == INT64_MAX) {
        LOGW_HW("❌ Invalid PTS value: %" PRId64 " - rejecting", packet->pts);
        return nullptr;
    }

    LOGI_HW("📦 Received valid packet: size=%d, pts=%" PRId64, packet->size, packet->pts);

    LOGI_HW("📦 Received packet: size=%d, pts=%" PRId64, packet->size, packet->pts);

    // ===== PACKET VALIDATION AND QUEUING ONLY =====
    // decodePacket() should ONLY queue packets - NO MediaCodec access
    // All MediaCodec operations happen in queueProcessorThread() only

    // Check for corrupted packets first
    bool isCorrupted = isPacketCorrupted(packet);
    if (isCorrupted) {
        LOGW_HW("🔍 Detected corrupted packet: pts=%" PRId64 ", size=%d", packet->pts, packet->size);
        return nullptr; // Log and drop - no further processing
    }

    // ===== ADAPTIVE STARTUP RATE LIMITING =====
    // Allow burst during startup, then clamp to steady-state FPS
    auto currentTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();

    int64_t timeSinceLastPacket = currentTimeUs - m_lastPacketTimeUs;

    // Check if we should apply rate limiting based on startup state
    bool shouldApplyRateLimit = true;

    if (m_rateLimitState == RateLimitState::STARTUP) {
        if (!m_firstFrameReceived) {
            // Allow burst during startup - count packets
            m_startupBurstCount++;
            if (m_startupBurstCount <= m_maxStartupBurst) {
                LOGI_HW("🚀 STARTUP BURST: Allowing packet %d/%d without rate limit", m_startupBurstCount, m_maxStartupBurst);
                shouldApplyRateLimit = false;
            }
        } else {
            // First frame received - transition to steady state
            LOGI_HW("🎯 FIRST FRAME RECEIVED - Transitioning to steady-state rate limiting");
            m_rateLimitState = RateLimitState::STEADY_STATE;
            calculateAdaptiveLimits(m_inputBufferCount, m_inputBufferSize);
            shouldApplyRateLimit = true; // Apply limits now
        }
    }

    // Apply rate limiting if required
    if (shouldApplyRateLimit && m_lastPacketTimeUs > 0 && timeSinceLastPacket < m_minPacketIntervalUs) {
        // ===== DROP PACKET INSTEAD OF QUEUEING =====
        // Dropping is better than killing the codec
        LOGW_HW("⚠️ PACKET DROPPED - RATE TOO HIGH: %" PRId64 "us since last, need %" PRId64 "us minimum",
                timeSinceLastPacket, m_minPacketIntervalUs);
        LOGW_HW("🗑️ Dropping packet pts=%" PRId64 " to prevent codec overload", packet->pts);
        return nullptr; // Drop packet completely
    }

    // Packet rate is acceptable - update timing
    m_lastPacketTimeUs = currentTimeUs;

    // Packet can be processed immediately - queue it for MediaCodec processing
    LOGI_HW("✅ Packet queued for immediate MediaCodec processing");
    queuePacketForProcessing(packet, false, false); // Queue for immediate processing

    // Return nullptr - actual decoding happens asynchronously in queue processor
    // This maintains the expected interface while ensuring thread safety
    return nullptr;
}

AVFrame* MediaCodecHardwareDecoder::convertHardwareFrame(const uint8_t* data, size_t size,
                                                        int width, int height) {
    LOGI_HW("=== CONVERTING HARDWARE FRAME ===");
    LOGI_HW("Input: data=%p, size=%zu, dimensions=%dx%d", data, size, width, height);

    // CRITICAL: Check if data is valid
    if (!data) {
        LOGE_HW("Input data is null - rejecting frame");
        return nullptr;
    }

    // Validate data pointer is in valid range (not corrupted)
    uintptr_t dataPtr = reinterpret_cast<uintptr_t>(data);
#if UINTPTR_MAX == UINT64_MAX
    // 64-bit architecture
    if (dataPtr < 0x1000 || dataPtr >= 0x8000000000000000ULL) {
        LOGE_HW("Data pointer appears corrupted: %p (uintptr=%" PRIuPTR ")", data, dataPtr);
        return nullptr;
    }
    // Additional validation: check for known bad addresses
    if (dataPtr == 0x100000000ULL) {
        LOGE_HW("CRITICAL: Detected the exact crash address 0x100000000 in input data!");
        return nullptr;
    }
#else
    // 32-bit architecture
    if (dataPtr < 0x1000 || dataPtr >= 0xC0000000) {  // Check against typical 32-bit address space limit
        LOGE_HW("Data pointer appears corrupted: %p (uintptr=%" PRIuPTR ")", data, dataPtr);
        return nullptr;
    }
    // Additional validation: check for known bad addresses
    if (dataPtr == 0x10000000) {
        LOGE_HW("CRITICAL: Detected the exact crash address 0x10000000 in input data!");
        return nullptr;
    }
#endif

    // Validate dimensions with stricter bounds
    if (width <= 0 || height <= 0 || width > 4096 || height > 4096) {
        LOGE_HW("Invalid dimensions: %dx%d (must be 1-4096)", width, height);
        return nullptr;
    }

    // ===== DETECT MEDIACODEC OUTPUT FORMAT =====
    // MediaCodec NEVER outputs BGRA - it outputs YUV variants
    // We must detect the actual format and convert properly

    // Query the actual output format from MediaCodec
    AMediaFormat* outputFormat = AMediaCodec_getOutputFormat(m_codec);
    if (!outputFormat) {
        LOGW_HW("⚠️ Could not get output format - assuming NV12 as fallback");
        // Fall back to NV12 assumption (most common YUV420SP variant)
    }

    int32_t colorFormat = 0;
    if (outputFormat) {
        AMediaFormat_getInt32(outputFormat, AMEDIAFORMAT_KEY_COLOR_FORMAT, &colorFormat);
        LOGI_HW("MediaCodec output format: color_format=%d", colorFormat);
    }

    // Determine actual pixel format based on MediaCodec output
    enum AVPixelFormat inputFormat = AV_PIX_FMT_NV12; // Default fallback
    bool needsConversion = true;

    switch (colorFormat) {
        case 19: // COLOR_FormatYUV420Planar (YUV420P - 3 planes)
            inputFormat = AV_PIX_FMT_YUV420P;
            LOGI_HW("✅ Detected YUV420P format (3-plane)");
            break;
        case 21: // COLOR_FormatYUV420SemiPlanar (NV12 - 2 planes, most common)
        case 2141391876: // OMX_QCOM_COLOR_FormatYVU420SemiPlanar (NV21)
            inputFormat = AV_PIX_FMT_NV12; // Treat NV21 as NV12 for simplicity (minor color difference)
            LOGI_HW("✅ Detected YUV420SP format (2-plane)");
            break;
        case 0x7F000200: // COLOR_FormatYUV420Flexible (common on newer devices)
            inputFormat = AV_PIX_FMT_NV12; // Assume NV12 for flexible format
            LOGI_HW("✅ Detected YUV420Flexible (assuming NV12)");
            break;
        default:
            // Unknown format - try to detect from size analysis
            size_t expectedYUV420SP = (width * height * 3) / 2; // YUV420SP = 1.5 bytes per pixel
            if (size >= expectedYUV420SP * 0.9 && size <= expectedYUV420SP * 1.1) {
                inputFormat = AV_PIX_FMT_NV12;
                LOGI_HW("✅ Assuming YUV420SP/NV12 based on size analysis (%zu bytes)", size);
            } else {
                LOGW_HW("⚠️ Unknown color format %d, size %zu - will attempt direct BGRA copy", colorFormat, size);
                inputFormat = AV_PIX_FMT_BGRA;
                needsConversion = false; // Already BGRA or compatible
            }
            break;
    }

    if (outputFormat) {
        AMediaFormat_delete(outputFormat);
    }

    // Allocate output frame - always BGRA for FFmpeg compatibility
    AVFrame* outputFrame = av_frame_alloc();
    if (!outputFrame) {
        LOGE_HW("Failed to allocate output frame");
        return nullptr;
    }

    outputFrame->format = AV_PIX_FMT_BGRA;
    outputFrame->width = width;
    outputFrame->height = height;

    // Allocate output buffer
    int ret = av_frame_get_buffer(outputFrame, 32);
    if (ret < 0) {
        LOGE_HW("Failed to allocate output frame buffer: %d", ret);
        av_frame_free(&outputFrame);
        return nullptr;
    }

    if (needsConversion) {
        LOGI_HW("🔄 Converting %s → BGRA using libswscale", getPixelFormatName(inputFormat));

        // ===== YUV TO RGB CONVERSION =====
        // Set up input frame with YUV data
        AVFrame* inputFrame = av_frame_alloc();
        if (!inputFrame) {
            LOGE_HW("Failed to allocate input frame for conversion");
            av_frame_free(&outputFrame);
            return nullptr;
        }

        inputFrame->format = inputFormat;
        inputFrame->width = width;
        inputFrame->height = height;

        // Set up input frame data pointers based on format
        if (inputFormat == AV_PIX_FMT_YUV420P) {
            // YUV420P: Y plane, U plane, V plane
            size_t ySize = width * height;
            size_t uvSize = (width / 2) * (height / 2);

            if (size < ySize + 2 * uvSize) {
                LOGE_HW("YUV420P buffer too small: %zu < %zu", size, ySize + 2 * uvSize);
                av_frame_free(&inputFrame);
                av_frame_free(&outputFrame);
                return nullptr;
            }

            inputFrame->data[0] = (uint8_t*)data;                    // Y plane
            inputFrame->data[1] = (uint8_t*)data + ySize;           // U plane
            inputFrame->data[2] = (uint8_t*)data + ySize + uvSize;  // V plane

            inputFrame->linesize[0] = width;
            inputFrame->linesize[1] = width / 2;
            inputFrame->linesize[2] = width / 2;

            LOGI_HW("YUV420P setup: Y=%p, U=%p, V=%p", inputFrame->data[0], inputFrame->data[1], inputFrame->data[2]);

        } else if (inputFormat == AV_PIX_FMT_NV12) {
            // YUV420SP/NV12: Y plane, UV interleaved plane
            size_t ySize = width * height;

            if (size < (ySize * 3) / 2) {
                LOGE_HW("NV12 buffer too small: %zu < %zu", size, (ySize * 3) / 2);
                av_frame_free(&inputFrame);
                av_frame_free(&outputFrame);
                return nullptr;
            }

            inputFrame->data[0] = (uint8_t*)data;          // Y plane
            inputFrame->data[1] = (uint8_t*)data + ySize;  // UV plane

            inputFrame->linesize[0] = width;
            inputFrame->linesize[1] = width; // UV plane has same width as Y in NV12

            LOGI_HW("NV12 setup: Y=%p, UV=%p", inputFrame->data[0], inputFrame->data[1]);
        }

        // Initialize or reinitialize swscale context for YUV->RGB conversion
        if (!m_swsContext) {
            m_swsContext = sws_getContext(width, height, inputFormat,
                                         width, height, AV_PIX_FMT_BGRA,
                                         SWS_BILINEAR, nullptr, nullptr, nullptr);

            if (!m_swsContext) {
                LOGE_HW("Failed to create swscale context for YUV→RGB conversion");
                av_frame_free(&inputFrame);
                av_frame_free(&outputFrame);
                return nullptr;
            }

            LOGI_HW("✅ Created swscale context: %dx%d %s → BGRA", width, height, getPixelFormatName(inputFormat));
        }

        // Perform the YUV to RGB conversion
        ret = sws_scale(m_swsContext,
                       inputFrame->data, inputFrame->linesize, 0, height,
                       outputFrame->data, outputFrame->linesize);

        if (ret < 0) {
            LOGE_HW("❌ sws_scale conversion failed: %d", ret);
            av_frame_free(&inputFrame);
            av_frame_free(&outputFrame);
            return nullptr;
        }

        av_frame_free(&inputFrame);
        LOGI_HW("✅ YUV→BGRA conversion successful: %dx%d %s → BGRA (sws_scale returned %d)",
                width, height, getPixelFormatName(inputFormat), ret);

    } else {
        // No conversion needed - direct BGRA copy
        LOGI_HW("Direct BGRA copy (no YUV conversion needed)");
        size_t expectedSize = (size_t)width * height * 4; // BGRA = 4 bytes per pixel
        size_t copySize = std::min(size, expectedSize);

        if (copySize > 0 && copySize <= outputFrame->buf[0]->size) {
            memcpy(outputFrame->data[0], data, copySize);

            // Zero-fill any remaining space if needed
            if (copySize < expectedSize) {
                size_t remaining = expectedSize - copySize;
                if (remaining <= outputFrame->buf[0]->size - copySize) {
                    memset(outputFrame->data[0] + copySize, 0, remaining);
                }
            }

            LOGI_HW("✅ Frame copy successful: copied %zu bytes", copySize);
        } else {
            LOGE_HW("❌ Invalid copy size: %zu (buffer size: %zu)", copySize, outputFrame->buf[0]->size);
            av_frame_free(&outputFrame);
            return nullptr;
        }
    }

    // Set proper frame metadata
    outputFrame->pts = 0; // Will be set by caller
    outputFrame->pict_type = AV_PICTURE_TYPE_NONE;

    LOGI_HW("🎬 Frame conversion completed successfully: %dx%d %s → BGRA",
            width, height, needsConversion ? getPixelFormatName(inputFormat) : "BGRA");

    return outputFrame;
}

bool MediaCodecHardwareDecoder::dequeueInputBuffer(ssize_t* index, int64_t timeoutUs) {
    LOGD_HW("=== DEQUEUE INPUT BUFFER ===");
    LOGD_HW("Requesting input buffer with timeout: %" PRId64 "us", timeoutUs);

    // Log current codec state before dequeue
    LOGD_HW("Codec state: initialized=%d, consecutive_failures=%d, try_again_later=%d",
            m_initialized, m_consecutiveFailures, m_consecutiveTryAgainLater);

    auto startTime = std::chrono::steady_clock::now();
    *index = AMediaCodec_dequeueInputBuffer(m_codec, timeoutUs);
    auto endTime = std::chrono::steady_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);

    if (*index >= 0) {
        LOGD_HW("✅ Input buffer dequeued successfully: index=%zd, time_taken=%" PRId64 "us",
                *index, (int64_t)duration.count());

        // ===== SUCCESS - RESET ALL FAILURE COUNTERS =====
        m_consecutiveFailures = 0;
        m_consecutiveTryAgainLater = 0;
        LOGD_HW("✅ Reset all failure counters on successful buffer dequeue");

        return true;
    } else {
        LOGW_HW("❌ Failed to dequeue input buffer: index=%zd, time_taken=%" PRId64 "us, timeout=%" PRId64 "us",
                *index, (int64_t)duration.count(), timeoutUs);

        // Detailed error analysis and recovery
        if (*index == AMEDIACODEC_INFO_TRY_AGAIN_LATER) {
            // ===== CRITICAL: HARD BACKOFF ON TRY_AGAIN_LATER =====
            // DO NOT retry immediately - this kills Exynos decoders
            LOGW_HW("  → Reason: AMEDIACODEC_INFO_TRY_AGAIN_LATER (all input buffers busy)");
            LOGW_HW("  → HARD BACKOFF: sleeping %d us before retry", m_exponentialBackoffUs);

            // Sleep with exponential backoff (minimum 2ms, maximum 16ms)
            usleep(m_exponentialBackoffUs);
            m_exponentialBackoffUs = std::min(m_exponentialBackoffUs * 2, 16000);

            // ===== TRACK PERSISTENT TRY_AGAIN_LATER FAILURES =====
            m_consecutiveTryAgainLater++;
            LOGW_HW("  → Consecutive TRY_AGAIN_LATER failures: %d/%d", m_consecutiveTryAgainLater, m_maxPersistentTryAgainLater);

            // ===== FLUSH AFTER 5 CONSECUTIVE FAILURES =====
            if (m_consecutiveTryAgainLater >= 5) {
                LOGE_HW("🚨 CRITICAL: %d consecutive TRY_AGAIN_LATER failures - FLUSHING CODEC", m_consecutiveTryAgainLater);

                // Flush the codec to clear internal state and reset buffer pools
                media_status_t flushStatus = AMediaCodec_flush(m_codec);
                if (flushStatus == AMEDIA_OK) {
                    LOGI_HW("✅ Successfully flushed MediaCodec after %d TRY_AGAIN_LATER failures", m_consecutiveTryAgainLater);
                    m_consecutiveTryAgainLater = 0; // Reset counter after successful flush
                    m_exponentialBackoffUs = 2000;  // Reset backoff

                    // Try one more time immediately after flush
                    LOGI_HW("🔄 Retrying buffer dequeue after flush...");
                    *index = AMediaCodec_dequeueInputBuffer(m_codec, 1000); // Short timeout
                    if (*index >= 0) {
                        LOGI_HW("✅ Buffer available immediately after flush!");
                        m_consecutiveFailures = 0;
                        return true;
                    } else {
                        LOGW_HW("❌ Still no buffer available after flush - chipset may be permanently broken");
                    }
                } else {
                    LOGE_HW("❌ Failed to flush MediaCodec: %d", flushStatus);
                }
            }

            // ===== RESTART CODEC AFTER 10 CONSECUTIVE FAILURES =====
            if (m_consecutiveTryAgainLater >= 10) {
                LOGE_HW("🚨 FATAL: %d consecutive TRY_AGAIN_LATER failures - RESTARTING CODEC", m_consecutiveTryAgainLater);

                // Force complete codec restart
                if (m_codec) {
                    LOGI_HW("Stopping and destroying failing MediaCodec instance...");
                    AMediaCodec_stop(m_codec);
                    AMediaCodec_delete(m_codec);
                    m_codec = nullptr;
                    LOGI_HW("✅ Destroyed failing MediaCodec instance");
                }

                // Mark as not initialized to force re-initialization
                m_initialized = false;
                m_consecutiveTryAgainLater = 0;
                m_exponentialBackoffUs = 2000;

                LOGE_HW("🔄 MediaCodec forcefully restarted - decoder marked as uninitialized for re-init");
                LOGE_HW("   → This should resolve permanent buffer starvation on Exynos");
            }

        } else {
            LOGW_HW("  → Unknown error code: %zd", *index);
            // Reset TRY_AGAIN_LATER counter for other errors
            m_consecutiveTryAgainLater = 0;
            m_exponentialBackoffUs = 2000; // Reset backoff for other errors
        }

        // Increment general failure counter
        m_consecutiveFailures++;

        return false;
    }
}

bool MediaCodecHardwareDecoder::dequeueOutputBuffer(AMediaCodecBufferInfo* info, ssize_t* index, int64_t timeoutUs) {
    *index = AMediaCodec_dequeueOutputBuffer(m_codec, info, timeoutUs);
    return *index >= 0;
}

bool MediaCodecHardwareDecoder::queryBufferCapabilities() {
    LOGI_HW("=== QUERYING MEDIACODEC BUFFER CAPABILITIES ===");

    if (!m_codec) {
        LOGE_HW("Cannot query buffer capabilities - codec is null");
        return false;
    }

    // Note: AMediaCodec_getInputBufferCount/OutputBufferCount are only available in API 28+
    // For API 24 compatibility, we use empirical testing and conservative defaults

    // Estimate buffer capabilities through empirical testing
    int inputBufferCount = estimateInputBufferCount();
    size_t inputBufferSize = estimateInputBufferSize();

    // Estimate output buffer count (typically matches input or is higher)
    int outputBufferCount = inputBufferCount; // Conservative estimate

    LOGI_HW("Estimated buffer capabilities (API 24 compatible):");
    LOGI_HW("   → Input buffers: %d (estimated)", inputBufferCount);
    LOGI_HW("   → Input buffer size: %zu bytes (estimated)", inputBufferSize);
    LOGI_HW("   → Output buffers: %d (estimated)", outputBufferCount);

    // Calculate adaptive rate limiting based on estimated capabilities
    calculateAdaptiveLimits(inputBufferCount, inputBufferSize);

    LOGI_HW("✅ Buffer capabilities estimated successfully");
    LOGI_HW("   → Using adaptive rate limiting for chipset compatibility");

    return true;
}

int MediaCodecHardwareDecoder::estimateInputBufferCount() {
    // Conservative estimates based on common Android device capabilities
    // Most devices have 4-8 input buffers for video decoding
    return 4; // Conservative default that works on most devices
}

size_t MediaCodecHardwareDecoder::estimateInputBufferSize() {
    // Estimate based on video resolution and codec requirements
    // H.264 typically needs buffers sized for compressed frames
    // Conservative estimate: enough for 1080p compressed frame
    return 2 * 1024 * 1024; // 2MB conservative estimate
}

void MediaCodecHardwareDecoder::calculateAdaptiveLimits(int bufferCount, size_t bufferSize) {
    LOGI_HW("=== CALCULATING ADAPTIVE RATE LIMITS ===");
    LOGI_HW("Hardware capabilities: buffers=%d, size=%zu", bufferCount, bufferSize);

    // Adaptive packet rate based on buffer count
    // More buffers = higher throughput allowed
    if (bufferCount >= 8) {
        m_maxPacketsPerSecond = 120; // High-end devices
        m_minPacketIntervalUs = 8333; // ~8.3ms (120 FPS)
        LOGI_HW("High-end device detected: 120 FPS limit");
    } else if (bufferCount >= 4) {
        m_maxPacketsPerSecond = 60; // Mid-range devices
        m_minPacketIntervalUs = 16667; // ~16.7ms (60 FPS)
        LOGI_HW("Mid-range device detected: 60 FPS limit");
    } else {
        m_maxPacketsPerSecond = 30; // Low-end/conservative devices
        m_minPacketIntervalUs = 33333; // ~33.3ms (30 FPS)
        LOGI_HW("Conservative device detected: 30 FPS limit");
    }

    // Adjust based on buffer size (smaller buffers need slower rates)
    if (bufferSize < 512 * 1024) { // < 512KB
        m_maxPacketsPerSecond = std::max(15, m_maxPacketsPerSecond / 2);
        m_minPacketIntervalUs *= 2;
        LOGI_HW("Small buffers detected - reducing rate by 50%%");
    }

    // Store capabilities for monitoring
    m_inputBufferCount = bufferCount;
    m_inputBufferSize = bufferSize;

    LOGI_HW("✅ Adaptive limits calculated:");
    LOGI_HW("   → Max packets/sec: %d", m_maxPacketsPerSecond);
    LOGI_HW("   → Min interval: %" PRId64 " μs", m_minPacketIntervalUs);
    LOGI_HW("   → Safe for chipset capabilities: buffers=%d, size=%zu",
            m_inputBufferCount, m_inputBufferSize);
}

void MediaCodecHardwareDecoder::flush() {
    if (m_codec && m_initialized) {
        LOGI_HW("Flushing MediaCodec hardware decoder");
        AMediaCodec_flush(m_codec);
    }
}

void MediaCodecHardwareDecoder::checkFailureRecovery() {
    LOGD_HW("=== CHECKING FAILURE RECOVERY ===");
    LOGD_HW("Current consecutive failures: %d/%d", m_consecutiveFailures, m_maxConsecutiveFailures);

    if (m_consecutiveFailures >= m_maxConsecutiveFailures) {
        LOGE_HW("🚨 CRITICAL: MediaCodec has failed %d times consecutively - forcing complete restart",
                m_consecutiveFailures);

        // Force complete codec restart
        if (m_codec) {
            LOGI_HW("Stopping and destroying failing MediaCodec instance...");
            AMediaCodec_stop(m_codec);
            AMediaCodec_delete(m_codec);
            m_codec = nullptr;
            LOGI_HW("✅ Destroyed failing MediaCodec instance");
        }

        // Mark as not initialized to force re-initialization
        m_initialized = false;

        // Reset failure counter completely
        m_consecutiveFailures = 0;

        LOGE_HW("🔄 MediaCodec forcefully restarted - decoder marked as uninitialized for re-init");
        LOGE_HW("   → This should resolve persistent buffer starvation issues");

    } else if (m_consecutiveFailures >= m_maxConsecutiveFailures / 2) {
        LOGW_HW("⚠️  WARNING: MediaCodec has failed %d times - attempting flush recovery",
                m_consecutiveFailures);

        // Attempt flush recovery for less severe failures
        if (m_codec) {
            LOGI_HW("Attempting MediaCodec flush recovery...");
            AMediaCodec_flush(m_codec);
            LOGI_HW("✅ Flushed MediaCodec after %d consecutive failures", m_consecutiveFailures);

            // Give it a chance to recover by reducing failure count slightly
            m_consecutiveFailures = m_consecutiveFailures / 2;
            LOGD_HW("Reduced failure count to %d after flush", m_consecutiveFailures);
        } else {
            LOGW_HW("Cannot flush - MediaCodec is null");
        }

    } else if (m_consecutiveFailures > 0 && m_consecutiveFailures % 5 == 0) {
        LOGW_HW("👀 MediaCodec has failed %d times - monitoring closely (restart threshold: %d)",
                m_consecutiveFailures, m_maxConsecutiveFailures);

        // Log additional diagnostics every 5 failures
        LOGW_HW("   → Buffer starvation detected - packets arriving faster than processing");
        LOGW_HW("   → Possible solutions: Reduce packet rate, increase timeout, check threading");
    }
}

// ===== ADVANCED PACKET QUEUING AND CORRUPTION HANDLING SYSTEM =====

void MediaCodecHardwareDecoder::startQueueProcessor() {
    LOGI_HW("Starting packet queue processor thread");
    m_stopQueueProcessor = false;
    m_queueProcessorThread = std::thread(&MediaCodecHardwareDecoder::queueProcessorThread, this);
}

void MediaCodecHardwareDecoder::stopQueueProcessor() {
    LOGI_HW("Stopping packet queue processor thread");
    m_stopQueueProcessor = true;
    m_queueCondition.notify_all();

    if (m_queueProcessorThread.joinable()) {
        m_queueProcessorThread.join();
    }

    // Clear any remaining queued packets
    std::lock_guard<std::mutex> lock(m_queueMutex);
    while (!m_packetQueue.empty()) {
        auto& queuedPacket = m_packetQueue.front();
        if (queuedPacket->packet) {
            av_packet_free(&queuedPacket->packet);
        }
        m_packetQueue.pop();
    }
    LOGI_HW("Packet queue processor stopped and cleaned up");
}

AVFrame* MediaCodecHardwareDecoder::decodeQueuedPacket(AVPacket* packet) {
    // ===== MODE-AWARE DECODE =====
    // This method is only called in BUFFER mode - SURFACE mode handles output differently
    if (m_surfaceMode) {
        LOGE_HW("❌ decodeQueuedPacket called in SURFACE mode - this should never happen!");
        return nullptr;
    }

    // Similar to decodePacket but without the rate limiting checks
    // since we already verified timing in the queue processor
    if (!m_initialized || !m_codec || !packet) {
        LOGW_HW("Decoder not ready or invalid queued packet");
        return nullptr;
    }

    LOGI_HW("Decoding queued packet: size=%d, pts=%" PRId64, packet->size, packet->pts);

    // Feed input packet to MediaCodec (same logic as decodePacket)
    ssize_t inputIndex;
    if (!dequeueInputBuffer(&inputIndex, 10000)) {
        LOGW_HW("No available input buffer for queued packet");
        return nullptr;
    }

    size_t bufferSize;
    uint8_t* inputBuffer = AMediaCodec_getInputBuffer(m_codec, inputIndex, &bufferSize);

    if (!inputBuffer) {
        LOGE_HW("Input buffer issue for queued packet");
        return nullptr;
    }

    // ===== CONVERT AVCC TO ANNEX B FORMAT =====
    // MediaCodec expects Annex B (start-code prefixed) when CSD is provided separately
    uint8_t* convertedData = nullptr;
    size_t convertedSize = 0;

    if (!convertAvccToAnnexB(packet->data, packet->size, &convertedData, &convertedSize)) {
        LOGE_HW("❌ Failed to convert AVCC packet to Annex B format");
        return nullptr;
    }

    if (bufferSize < convertedSize) {
        LOGE_HW("Input buffer too small for converted packet: %zu < %zu", bufferSize, convertedSize);
        av_free(convertedData);
        return nullptr;
    }

    memcpy(inputBuffer, convertedData, convertedSize);
    av_free(convertedData); // Free the converted buffer

    media_status_t status = AMediaCodec_queueInputBuffer(m_codec, inputIndex, 0, convertedSize,
                               packet->pts, 0);
    if (status != AMEDIA_OK) {
        LOGE_HW("Failed to queue input buffer for queued packet: %d", status);
        return nullptr;
    }

    // Try to get output - BUFFER MODE ONLY
    AMediaCodecBufferInfo info;
    ssize_t outputIndex;
    if (!dequeueOutputBuffer(&info, &outputIndex, 0)) {
        LOGI_HW("No output available yet for queued packet");
        return nullptr;
    }

    if (outputIndex >= 0) {
        size_t outputSize;
        uint8_t* outputBuffer = AMediaCodec_getOutputBuffer(m_codec, outputIndex, &outputSize);

        if (outputBuffer && outputSize > 0) {
            AVFrame* result = convertHardwareFrame(outputBuffer + info.offset,
                                                 info.size, m_width, m_height);
            if (result) {
                result->pts = info.presentationTimeUs;
                LOGI_HW("✅ Queued frame decoded successfully: pts=%" PRId64, result->pts);
                AMediaCodec_releaseOutputBuffer(m_codec, outputIndex, false);
                return result;
            }
        }

        AMediaCodec_releaseOutputBuffer(m_codec, outputIndex, false);
    }

    return nullptr;
}

void MediaCodecHardwareDecoder::queueProcessorThread() {
    LOGI_HW("🔄 QUEUE PROCESSOR THREAD STARTED - ONLY THREAD TOUCHING MEDIACODEC");

    while (!m_stopQueueProcessor) {
        std::unique_lock<std::mutex> lock(m_queueMutex);

        // Wait for packets to process or stop signal
        m_queueCondition.wait(lock, [this]() {
            return m_stopQueueProcessor || !m_packetQueue.empty();
        });

        if (m_stopQueueProcessor) {
            break;
        }

        // Process queued packets - MediaCodec access is THREAD-SAFE HERE ONLY
        while (!m_packetQueue.empty() && !m_stopQueueProcessor) {
            // ===== CHECK DECODER WATCHDOG =====
            // Monitor for stalled decoding and trigger recovery if needed
            checkWatchdogTimeout();

            auto& queuedPacket = m_packetQueue.front();

            // ===== EOS HANDLING =====
            if (!queuedPacket->packet) {
                // This is an EOS packet (nullptr)
                LOGI_HW("🎬 Processing EOS packet - signaling end of stream");

                // Get input buffer for EOS
                ssize_t inputIndex;
                if (!dequeueInputBuffer(&inputIndex, 10000)) {
                    LOGW_HW("❌ No input buffer available for EOS - will retry later");
                    break; // Exit inner loop, wait for next cycle
                }

                // Queue EOS buffer - size=0, pts=0, flags=END_OF_STREAM
                LOGI_HW("📤 Queuing EOS buffer to MediaCodec");
                media_status_t status = AMediaCodec_queueInputBuffer(m_codec, inputIndex, 0, 0, 0,
                                                   AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM);
                if (status != AMEDIA_OK) {
                    LOGE_HW("❌ Failed to queue EOS buffer: %d", status);
                    break;
                }

                LOGI_HW("✅ EOS buffer queued successfully - decoder will drain remaining frames");

                // ===== DRAIN ALL REMAINING OUTPUT BUFFERS =====
                // EOS ensures all pending frames are output before EOS frame
                LOGI_HW("🗑️ Draining all remaining output buffers after EOS");
                AMediaCodecBufferInfo info;
                bool eosReceived = false;

                while (!eosReceived && !m_stopQueueProcessor) {
                    ssize_t outputIndex = AMediaCodec_dequeueOutputBuffer(m_codec, &info, 10000); // 10ms timeout

                    if (outputIndex >= 0) {
                        // Check if this is the EOS frame
                        if (info.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM) {
                            LOGI_HW("🎬 EOS frame received - end of stream complete");
                            eosReceived = true;
                        } else {
                            // Process regular frame - MODE SPECIFIC HANDLING
                            if (m_surfaceMode) {
                                // SURFACE MODE: Render to surface, create synthetic frame
                                // CRITICAL: Never call getOutputBuffer() in surface mode - causes crashes
                                AMediaCodec_releaseOutputBuffer(m_codec, outputIndex, true);
                                LOGI_HW("🎬 EOS drain frame rendered to surface: pts=%" PRId64, info.presentationTimeUs);

                                // Create synthetic frame for timing - NO PIXEL DATA ACCESS
                                AVFrame* syntheticFrame = av_frame_alloc();
                                if (syntheticFrame) {
                                    syntheticFrame->pts = info.presentationTimeUs;
                                    syntheticFrame->width = m_width;
                                    syntheticFrame->height = m_height;
                                    syntheticFrame->format = AV_PIX_FMT_NONE; // No data
                                    syntheticFrame->pict_type = AV_PICTURE_TYPE_NONE;
                                    // Explicitly set data pointers to NULL for safety
                                    memset(syntheticFrame->data, 0, sizeof(syntheticFrame->data));
                                    memset(syntheticFrame->linesize, 0, sizeof(syntheticFrame->linesize));

                                    // ===== DELIVER SYNTHETIC FRAME TO VIDEO PIPELINE =====
                                    if (m_frameDeliveryCallback) {
                                        LOGI_HW("📤 Delivering EOS drain synthetic frame: pts=%" PRId64, syntheticFrame->pts);
                                        m_frameDeliveryCallback(syntheticFrame);
                                        LOGI_HW("✅ EOS drain synthetic frame delivered successfully");
                                    } else {
                                        LOGW_HW("⚠️ No frame delivery callback set - dropping EOS drain frame");
                                        av_frame_free(&syntheticFrame);
                                    }
                                }
                            } else {
                                // BUFFER MODE: Convert CPU buffer to BGRA
                                size_t outputSize;
                                uint8_t* outputBuffer = AMediaCodec_getOutputBuffer(m_codec, outputIndex, &outputSize);

                                if (outputBuffer && outputSize > 0) {
                                    AVFrame* result = convertHardwareFrame(outputBuffer + info.offset,
                                                                         info.size, m_width, m_height);
                                    if (result) {
                                        result->pts = info.presentationTimeUs;
                                        LOGI_HW("✅ EOS drain frame decoded: pts=%" PRId64, result->pts);

                                        // ===== DELIVER FRAME TO VIDEO PIPELINE =====
                                        if (m_frameDeliveryCallback) {
                                            LOGI_HW("📤 Delivering EOS drain frame to pipeline: pts=%" PRId64, result->pts);
                                            m_frameDeliveryCallback(result);
                                            LOGI_HW("✅ EOS drain frame delivered successfully");
                                        } else {
                                            LOGW_HW("⚠️ No frame delivery callback set - dropping EOS drain frame");
                                            av_frame_free(&result);
                                        }
                                    }
                                }
                            }
                        }

                        // Always release output buffer
                        AMediaCodec_releaseOutputBuffer(m_codec, outputIndex, false);

                    } else if (outputIndex == AMEDIACODEC_INFO_TRY_AGAIN_LATER) {
                        LOGD_HW("⏳ Waiting for more output buffers during EOS drain");
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    } else if (outputIndex == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED) {
                        LOGI_HW("📺 Output format changed during EOS drain");
                        continue;
                    } else {
                        LOGW_HW("Unknown output index during EOS drain: %zd", outputIndex);
                        break;
                    }
                }

                LOGI_HW("✅ EOS processing complete - all frames drained");

                // Remove EOS packet from queue
                m_packetQueue.pop();
                continue; // Process next packet
            }

            // ===== REGULAR PACKET PROCESSING =====
            // Check if we can process this packet now (rate limiting)
            auto currentTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count();

            int64_t timeSinceLastPacket = currentTimeUs - m_lastPacketTimeUs;

            if (timeSinceLastPacket >= m_minPacketIntervalUs) {
                // Can process this packet now
                LOGI_HW("🎯 Processing queued packet: pts=%" PRId64 ", queue_time=%" PRId64 "us",
                        (int64_t)queuedPacket->packet->pts, (int64_t)(currentTimeUs - queuedPacket->queueTimeUs));

                // ===== FULL MEDIACODEC PROCESSING CYCLE =====
                // 1. Get input buffer
                ssize_t inputIndex;
                if (!dequeueInputBuffer(&inputIndex, 10000)) {
                    LOGW_HW("❌ No input buffer available for queued packet - will retry later");
                    break; // Exit inner loop, wait for next cycle
                }

                // 2. Get buffer pointer and validate
                size_t bufferSize;
                uint8_t* inputBuffer = AMediaCodec_getInputBuffer(m_codec, inputIndex, &bufferSize);

                if (!inputBuffer) {
                    LOGE_HW("❌ Failed to get input buffer pointer");
                    break;
                }

                if (bufferSize < (size_t)queuedPacket->packet->size) {
                    LOGE_HW("❌ Input buffer too small: %zu < %d", bufferSize, queuedPacket->packet->size);
                    // ===== GUARANTEE BUFFER QUEUEING - NEVER LEAK =====
                    AMediaCodec_queueInputBuffer(m_codec, inputIndex, 0, 0, queuedPacket->packet->pts, 0);
                    break;
                }

                // 3. Convert AVCC to Annex B and copy packet data safely
                // MediaCodec expects Annex B (start-code prefixed) when CSD is provided separately

                // CRITICAL: Validate packet data before conversion
                if (!queuedPacket->packet->data || queuedPacket->packet->size == 0) {
                    LOGE_HW("❌ Invalid packet data: data=%p, size=%d",
                           queuedPacket->packet->data, queuedPacket->packet->size);
                    break;
                }

                // Check for known crash addresses in packet data
                uintptr_t packetDataPtr = reinterpret_cast<uintptr_t>(queuedPacket->packet->data);
#if UINTPTR_MAX == UINT64_MAX
                if (packetDataPtr == 0x100000000ULL) {
                    LOGE_HW("❌ CRITICAL: Packet data points to crash address 0x100000000!");
                    break;
                }
#endif

                uint8_t* convertedData = nullptr;
                size_t convertedSize = 0;

                if (!convertAvccToAnnexB(queuedPacket->packet->data, queuedPacket->packet->size,
                                       &convertedData, &convertedSize)) {
                    LOGE_HW("❌ Failed to convert queued AVCC packet to Annex B format");
                    break;
                }

                if (bufferSize < convertedSize) {
                    LOGE_HW("❌ Input buffer too small for converted queued packet: %zu < %zu", bufferSize, convertedSize);
                    av_free(convertedData);
                    break;
                }

                memcpy(inputBuffer, convertedData, convertedSize);
                av_free(convertedData); // Free the converted buffer

                // 4. Queue input buffer (NO EOS FLAGS)
                media_status_t status = AMediaCodec_queueInputBuffer(m_codec, inputIndex, 0,
                                                   convertedSize, queuedPacket->packet->pts, 0);
                if (status != AMEDIA_OK) {
                    LOGE_HW("❌ Failed to queue input buffer: %d", status);
                    break;
                }

                LOGI_HW("📤 Input buffer queued successfully");

                // ===== MODE-SPECIFIC OUTPUT HANDLING =====
                if (m_surfaceMode) {
                    // ===== SURFACE MODE: Zero-copy GPU rendering =====
                    // MediaCodec renders directly to ANativeWindow surface
                    // NEVER call getOutputBuffer() or convertHardwareFrame() in surface mode

                    // Drain output buffers by rendering to surface (continuous draining)
                    AMediaCodecBufferInfo info;
                    bool frameRendered = false;

                    while (!frameRendered && !m_stopQueueProcessor) {
                        ssize_t outputIndex = AMediaCodec_dequeueOutputBuffer(m_codec, &info, 0); // Non-blocking

                        if (outputIndex >= 0) {
                            // Render directly to surface (true = render to surface)
                            AMediaCodec_releaseOutputBuffer(m_codec, outputIndex, true);
                            LOGI_HW("🎬 Frame rendered to surface: pts=%" PRId64, info.presentationTimeUs);

                            m_consecutiveFailures = 0; // Reset on success
                            frameRendered = true;

                            // ===== CREATE SYNTHETIC FRAME FOR TIMING =====
                            // In SURFACE mode, we don't have CPU-accessible frame data
                            // But we still need to maintain timing for A/V sync
                            AVFrame* syntheticFrame = av_frame_alloc();
                            if (syntheticFrame) {
                                syntheticFrame->pts = info.presentationTimeUs;
                                syntheticFrame->width = m_width;
                                syntheticFrame->height = m_height;
                                syntheticFrame->format = AV_PIX_FMT_NONE; // No actual data
                                syntheticFrame->pict_type = AV_PICTURE_TYPE_NONE;

                                // ===== DELIVER SYNTHETIC FRAME TO VIDEO PIPELINE =====
                                if (m_frameDeliveryCallback) {
                                    LOGI_HW("📤 Delivering synthetic surface frame to pipeline: pts=%" PRId64, syntheticFrame->pts);
                                    m_frameDeliveryCallback(syntheticFrame);
                                    LOGI_HW("✅ Synthetic frame delivered successfully");

                                    // ===== UPDATE WATCHDOG ON SUCCESSFUL FRAME DELIVERY =====
                                    updateWatchdogOnFrameReceived();
                                } else {
                                    LOGW_HW("⚠️ No frame delivery callback set - dropping synthetic frame");
                                    av_frame_free(&syntheticFrame);
                                }
                            } else {
                                LOGW_HW("⚠️ Failed to allocate synthetic frame for surface timing");
                            }

                        } else if (outputIndex == AMEDIACODEC_INFO_TRY_AGAIN_LATER) {
                            // No frame ready yet - exit loop and continue processing
                            break;
                        } else if (outputIndex == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED) {
                            LOGI_HW("📺 Output format changed in surface mode");
                            continue;
                        } else {
                            LOGW_HW("Unknown output index in surface mode: %zd", outputIndex);
                            break;
                        }
                    }

                } else {
                    // ===== BUFFER MODE: CPU buffer access + conversion =====
                    // Drain output buffers continuously to prevent input buffer starvation
                    // In BUFFER mode, we need to drain output buffers to prevent starvation
                    drainOutputNonBlocking();
                }

                // ===== SURFACE MODE SAFETY CHECK =====
                // In surface mode, frames are synthetic - no CPU data access needed
                // The GPU handles all rendering, so we skip CPU frame processing entirely

                // Update timing
                m_lastPacketTimeUs = currentTimeUs;

                // Remove from queue
                if (queuedPacket->packet) {
                    av_packet_free(&queuedPacket->packet);
                }
                m_packetQueue.pop();

                LOGI_HW("✅ Queued packet processed and removed from queue");

            } else {
                // Not ready yet, wait for timing
                lock.unlock();
                std::this_thread::sleep_for(std::chrono::microseconds(1000)); // 1ms
                lock.lock();
            }
        }
    }

    LOGI_HW("🔄 Queue processor thread exiting");
}

// ===== CONTINUOUS OUTPUT DRAINING =====
void MediaCodecHardwareDecoder::drainOutputNonBlocking() {
    AMediaCodecBufferInfo info;
    while (true) {
        ssize_t index = AMediaCodec_dequeueOutputBuffer(m_codec, &info, 0); // Non-blocking

        if (index >= 0) {
            // Got output buffer - release it immediately
            AMediaCodec_releaseOutputBuffer(m_codec, index, false);
            LOGD_HW("🗑️ Drained output buffer: index=%zd", index);
        } else if (index == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED) {
            // Format changed - handle if needed
            LOGI_HW("📺 Output format changed during draining");
            continue;
        } else {
            // No more output buffers available
            break;
        }
    }
}

void MediaCodecHardwareDecoder::queuePacketForProcessing(AVPacket* packet, bool isRateLimited, bool isCorrupted) {
    // Make a copy of the packet for queuing
    AVPacket* packetCopy = av_packet_alloc();
    if (!packetCopy) {
        LOGE_HW("Failed to allocate packet copy for queuing");
        return;
    }

    if (av_packet_ref(packetCopy, packet) < 0) {
        LOGE_HW("Failed to copy packet for queuing");
        av_packet_free(&packetCopy);
        return;
    }

    auto queueTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();

    auto queuedPacket = std::make_unique<QueuedPacket>(packetCopy);
    queuedPacket->queueTimeUs = queueTimeUs;

    if (isRateLimited) {
        LOGI_HW("📋 Queued packet for rate limiting: pts=%" PRId64, packet->pts);
    }

    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_packetQueue.push(std::move(queuedPacket));
    }

    m_queueCondition.notify_one();
    LOGD_HW("Packet queued successfully - queue size: %zu", m_packetQueue.size());
}

AVFrame* MediaCodecHardwareDecoder::tryProcessQueuedPacket() {
    std::lock_guard<std::mutex> lock(m_queueMutex);

    if (m_packetQueue.empty()) {
        return nullptr;
    }

    // Check if we can process the next packet
    auto currentTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();

    int64_t timeSinceLastPacket = currentTimeUs - m_lastPacketTimeUs;

    if (timeSinceLastPacket >= m_minPacketIntervalUs) {
        auto& queuedPacket = m_packetQueue.front();
        AVFrame* result = decodeQueuedPacket(queuedPacket->packet);

        if (result) {
            LOGI_HW("✅ Processed queued packet successfully");
            m_lastPacketTimeUs = currentTimeUs;

            // Remove from queue
            if (queuedPacket->packet) {
                av_packet_free(&queuedPacket->packet);
            }
            m_packetQueue.pop();

            return result;
        } else {
            LOGW_HW("❌ Failed to process queued packet");
            // Keep in queue for retry or redownload
        }
    }

    return nullptr;
}

bool MediaCodecHardwareDecoder::isPacketCorrupted(AVPacket* packet) {
    if (!packet || !packet->data || packet->size <= 0) {
        return true; // Null or empty packets are corrupted
    }

    // Check for common corruption indicators
    if (packet->size > 10 * 1024 * 1024) { // > 10MB is suspicious
        LOGW_HW("Packet size too large, possible corruption: %d bytes", packet->size);
        return true;
    }

    // Check for invalid PTS/DTS
    if (packet->pts < 0 && packet->pts != AV_NOPTS_VALUE) {
        LOGW_HW("Invalid PTS detected: %" PRId64, packet->pts);
        return true;
    }

    // Additional corruption checks can be added here
    // For now, basic validation is sufficient

    return false;
}

void MediaCodecHardwareDecoder::setLoadingState(bool loading) {
    if (m_isInLoadingState != loading) {
        m_isInLoadingState = loading;
        LOGI_HW("Loading state changed: %s", loading ? "ACTIVE" : "INACTIVE");
        // TODO: Notify UI to show/hide loading indicator
    }
}

bool MediaCodecHardwareDecoder::isInLoadingState() const {
    return m_isInLoadingState;
}

// ===== DECODER WATCHDOG SYSTEM =====

void MediaCodecHardwareDecoder::updateWatchdogOnFrameReceived() {
    m_lastOutputFrameTime = std::chrono::steady_clock::now();
    m_watchdogActive = true;
    LOGD_HW("🐕 Watchdog updated - frame received at steady_clock time");
}

void MediaCodecHardwareDecoder::checkWatchdogTimeout() {
    if (!m_watchdogActive) {
        return; // Watchdog not active yet
    }

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - m_lastOutputFrameTime).count();

    if (elapsed > m_watchdogTimeoutMs) {
        // Check if input buffers are full (indicating stalled decoding)
        bool inputBuffersFull = false;
        ssize_t testIndex = AMediaCodec_dequeueInputBuffer(m_codec, 0); // Non-blocking
        if (testIndex == AMEDIACODEC_INFO_TRY_AGAIN_LATER) {
            inputBuffersFull = true;
        } else if (testIndex >= 0) {
            // Put the buffer back since we're just testing
            AMediaCodec_queueInputBuffer(m_codec, testIndex, 0, 0, 0, 0);
        }

        if (inputBuffersFull) {
            LOGE_HW("🚨 DECODER WATCHDOG: Timeout after %dms with full input buffers - triggering recovery", m_watchdogTimeoutMs);
            triggerWatchdogRecovery();
        } else {
            LOGD_HW("🐕 Watchdog timeout (%dms) but input buffers not full - continuing", m_watchdogTimeoutMs);
        }
    }
}

void MediaCodecHardwareDecoder::triggerWatchdogRecovery() {
    LOGW_HW("🔧 WATCHDOG RECOVERY: Flushing codec and clearing packet queue");

    // 1. Flush the codec
    if (m_codec) {
        media_status_t flushStatus = AMediaCodec_flush(m_codec);
        if (flushStatus == AMEDIA_OK) {
            LOGI_HW("✅ Watchdog recovery: Codec flushed successfully");
        } else {
            LOGW_HW("❌ Watchdog recovery: Failed to flush codec: %d", flushStatus);
        }
    }

    // 2. Clear all pending packets
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        size_t clearedPackets = 0;
        while (!m_packetQueue.empty()) {
            auto& queuedPacket = m_packetQueue.front();
            if (queuedPacket->packet) {
                av_packet_free(&queuedPacket->packet);
            }
            m_packetQueue.pop();
            clearedPackets++;
        }
        LOGI_HW("✅ Watchdog recovery: Cleared %zu pending packets from queue", clearedPackets);
    }

    // 3. Re-inject SPS/PPS if available
    if (m_codec) {
        // TODO: Re-inject SPS/PPS data to reset decoder state
        LOGI_HW("ℹ️  Watchdog recovery: SPS/PPS re-injection not yet implemented");
    }

    // 4. Reset watchdog
    m_watchdogActive = false;

    LOGI_HW("✅ Watchdog recovery complete - decoder should resume normal operation");
}







// ===== SPS/PPS EXTRACTION FROM EXTRADATA =====

bool MediaCodecHardwareDecoder::extractSpsPpsFromExtradata(const uint8_t* extradata, size_t extradataSize,
                                                         uint8_t** spsData, size_t* spsSize,
                                                         uint8_t** ppsData, size_t* ppsSize) {
    LOGI_HW("🔍 Extracting SPS/PPS from codec extradata: %zu bytes", extradataSize);

    // Initialize output parameters
    *spsData = nullptr;
    *spsSize = 0;
    *ppsData = nullptr;
    *ppsSize = 0;

    // Log raw extradata for debugging
    LOGD_HW("📊 Raw extradata dump:");
    for (size_t i = 0; i < extradataSize && i < 32; i += 4) {
        LOGD_HW("   [%02zu]: %02x %02x %02x %02x", i,
                extradata[i], extradata[i+1], extradata[i+2],
                (i+3 < extradataSize) ? extradata[i+3] : 0x00);
    }

    if (!extradata || extradataSize < 8) {
        LOGE_HW("❌ Extradata validation failed: data=%p, size=%zu (minimum 8 bytes required)",
                extradata, extradataSize);
        return false;
    }

    // Parse avcC box format (ISO/IEC 14496-15)
    // [0] configurationVersion (always 1)
    // [1] AVCProfileIndication
    // [2] profile_compatibility
    // [3] AVCLevelIndication
    // [4] lengthSizeMinusOne (usually 3, for 4-byte NAL lengths)
    // [5] numOfSequenceParameterSets (usually 1)
    // [6-7] sequenceParameterSetLength
    // [8...] SPS data
    // ... then PPS data

    uint8_t version = extradata[0];
    if (version != 1) {
        LOGE_HW("❌ Unsupported avcC version: %d (expected 1)", version);
        return false;
    }

    uint8_t numSps = extradata[5] & 0x1F; // Bottom 5 bits
    LOGI_HW("📊 avcC parse: numSPS=%d", numSps);

    if (numSps == 0) {
        LOGE_HW("❌ No SPS found in avcC box");
        return false;
    }

    // Parse SPS
    size_t pos = 6; // Start after fixed header
    for (uint8_t i = 0; i < numSps; i++) {
        if (pos + 2 >= extradataSize) {
            LOGE_HW("❌ avcC truncated while reading SPS length");
            return false;
        }

        uint16_t spsLen = (extradata[pos] << 8) | extradata[pos + 1];
        pos += 2;

        if (pos + spsLen > extradataSize) {
            LOGE_HW("❌ SPS data extends beyond avcC box (len=%d, available=%zu)", spsLen, extradataSize - pos);
            return false;
        }

        // Allocate SPS buffer - RAW NAL unit only (no start codes for CSD)
        // MediaCodec csd-0 expects: [SPS NAL data] (raw payload only)
        *spsData = (uint8_t*)av_malloc(spsLen);
        if (!*spsData) {
            LOGE_HW("❌ Failed to allocate SPS buffer");
            return false;
        }

        // Copy SPS NAL data directly (raw NAL unit)
        memcpy(*spsData, extradata + pos, spsLen);
        *spsSize = spsLen;

        LOGI_HW("✅ Extracted raw SPS NAL unit: %zu bytes", *spsSize);
        pos += spsLen;
        break; // Use first SPS
    }

    // Parse PPS
    if (pos >= extradataSize) {
        LOGE_HW("❌ No PPS data in avcC box");
        if (*spsData) av_free(*spsData);
        *spsData = nullptr;
        *spsSize = 0;
        return false;
    }

    uint8_t numPps = extradata[pos++];
    LOGI_HW("📊 avcC parse: numPPS=%d", numPps);

    if (numPps == 0) {
        LOGE_HW("❌ No PPS found in avcC box");
        if (*spsData) av_free(*spsData);
        *spsData = nullptr;
        *spsSize = 0;
        return false;
    }

    for (uint8_t i = 0; i < numPps; i++) {
        if (pos + 2 > extradataSize) {
            LOGE_HW("❌ avcC truncated while reading PPS length");
            if (*spsData) av_free(*spsData);
            *spsData = nullptr;
            *spsSize = 0;
            return false;
        }

        uint16_t ppsLen = (extradata[pos] << 8) | extradata[pos + 1];
        pos += 2;

        if (pos + ppsLen > extradataSize) {
            LOGE_HW("❌ PPS data extends beyond avcC box (len=%d, available=%zu)", ppsLen, extradataSize - pos);
            if (*spsData) av_free(*spsData);
            *spsData = nullptr;
            *spsSize = 0;
            return false;
        }

        // Allocate PPS buffer - RAW NAL unit only (no start codes for CSD)
        // MediaCodec csd-1 expects: [PPS NAL data] (raw payload only)
        *ppsData = (uint8_t*)av_malloc(ppsLen);
        if (!*ppsData) {
            LOGE_HW("❌ Failed to allocate PPS buffer");
            if (*spsData) av_free(*spsData);
            *spsData = nullptr;
            *spsSize = 0;
            return false;
        }

        // Copy PPS NAL data directly (raw NAL unit)
        memcpy(*ppsData, extradata + pos, ppsLen);
        *ppsSize = ppsLen;

        LOGI_HW("✅ Extracted raw PPS NAL unit: %zu bytes", *ppsSize);
        pos += ppsLen;
        break; // Use first PPS
    }

    LOGI_HW("✅ avcC parsing complete: SPS=%zu bytes, PPS=%zu bytes", *spsSize, *ppsSize);
    return true;
}



// ===== CODEC ENUMERATION ENSURANCE =====
// Ensures that codec enumeration has been performed before CSD format selection
bool MediaCodecHardwareDecoder::ensureCodecEnumeration() {
    // Check if enumeration has already been completed
    if (isEnumerationComplete()) {
        LOGD_HW("✅ Codec enumeration already completed");
        return true;
    }

    LOGW_HW("⚠️ Codec enumeration not yet complete - waiting for Java enumeration");

    // Wait for enumeration to complete with timeout (up to 5 seconds)
    // This handles the case where enumeration is still running asynchronously
    const int MAX_WAIT_MS = 5000;
    const int CHECK_INTERVAL_MS = 100;
    int totalWaitMs = 0;

    while (totalWaitMs < MAX_WAIT_MS) {
        if (isEnumerationComplete()) {
            LOGI_HW("✅ Codec enumeration completed during wait");
            return true;
        }

        // Check enumeration data periodically
        auto enumeratedCodecs = getEnumeratedCodecs();
        if (!enumeratedCodecs.empty()) {
            LOGI_HW("✅ Found %zu enumerated codecs - enumeration appears to be working", enumeratedCodecs.size());
            // Give it a bit more time to complete
            std::this_thread::sleep_for(std::chrono::milliseconds(CHECK_INTERVAL_MS / 2));
            totalWaitMs += CHECK_INTERVAL_MS / 2;

            if (isEnumerationComplete()) {
                LOGI_HW("✅ Codec enumeration completed after partial data check");
                return true;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(CHECK_INTERVAL_MS));
        totalWaitMs += CHECK_INTERVAL_MS;

        LOGD_HW("⏳ Still waiting for codec enumeration... (%d/%d ms)", totalWaitMs, MAX_WAIT_MS);
    }

    LOGE_HW("❌ Codec enumeration timeout after %d ms - enumeration may have failed", MAX_WAIT_MS);

    // Even if enumeration didn't complete, check if we have any data we can use
    auto enumeratedCodecs = getEnumeratedCodecs();
    if (!enumeratedCodecs.empty()) {
        LOGW_HW("⚠️ Using partial enumeration data (%zu codecs) despite incomplete status", enumeratedCodecs.size());
        return true; // We have some data, might as well use it
    }

    LOGE_HW("❌ No enumeration data available - falling back to defaults");
    return false;
}

// ===== ENUMERATION-BASED CSD FORMAT SELECTION =====
// Determines optimal CSD format based on Java bridge enumeration results
bool MediaCodecHardwareDecoder::shouldUseAnnexBFormat(const char* mimeType) {
    LOGI_HW("🔍 Determining CSD format for MIME type: %s", mimeType);

    // Ensure enumeration has been performed with timeout
    if (!ensureCodecEnumeration()) {
        LOGW_HW("⚠️ Codec enumeration not available - falling back to raw NAL units");
        return false; // Default to raw NAL units
    }

    // Get enumeration results
    auto enumeratedCodecs = getEnumeratedCodecs();
    if (enumeratedCodecs.empty()) {
        LOGW_HW("⚠️ No enumerated codecs found - falling back to raw NAL units");
        return false;
    }

    LOGI_HW("📊 Checking %zu enumerated codecs for CSD format preferences", enumeratedCodecs.size());

    // Look for matching codecs and check their capabilities
    for (const auto& codec : enumeratedCodecs) {
        // Check if this codec matches our MIME type
        if (codec.mimeType == mimeType && codec.isHardware) {
            LOGI_HW("✅ Found hardware codec for %s - checking CSD requirements", mimeType);

            // Use enumeration data to determine format preference
            std::string codecName = codec.name;
            std::transform(codecName.begin(), codecName.end(), codecName.begin(), ::tolower);

            // Chipset-specific CSD format detection
            // Some chipsets require Annex B format (start codes) while others work with raw NAL units
            if (codecName.find("exynos") != std::string::npos ||
                codecName.find("samsung") != std::string::npos ||
                codecName.find("mali") != std::string::npos ||
                codecName.find("rk") != std::string::npos) { // Rockchip
                LOGI_HW("📋 Exynos/Mali/Rockchip chipset detected - using Annex B CSD format");
                return true;
            }

            // Qualcomm and MediaTek chipsets generally work with raw NAL units
            if (codecName.find("qcom") != std::string::npos ||
                codecName.find("qualcomm") != std::string::npos ||
                codecName.find("mt") != std::string::npos ||
                codecName.find("mediatek") != std::string::npos) {
                LOGI_HW("📋 Qualcomm/MediaTek chipset detected - using raw NAL units CSD format");
                return false;
            }

            // Default based on codec type for unknown chipsets
            if (mimeType == std::string("video/avc")) {
                LOGI_HW("📋 H.264 on unknown chipset - defaulting to raw NAL units");
                return false;
            } else {
                LOGI_HW("📋 Non-H.264 codec on unknown chipset - using Annex B format for safety");
                return true;
            }
        }
    }

    LOGW_HW("⚠️ No suitable hardware codec found in enumeration for %s - defaulting to raw NAL units", mimeType);
    return false;
}

// ===== AVCC TO ANNEX B CONVERSION =====
// Converts length-prefixed NAL units (AVCC format from MP4) to start-code prefixed (Annex B)
// MediaCodec expects Annex B format when SPS/PPS are provided as CSD parameters
bool MediaCodecHardwareDecoder::convertAvccToAnnexB(const uint8_t* inputData, size_t inputSize,
                                                   uint8_t** outputData, size_t* outputSize) {
    LOGD_HW("🔄 Converting AVCC packet (%zu bytes) to Annex B format", inputSize);

    if (!inputData || inputSize < 4) {
        LOGE_HW("❌ Invalid input data for AVCC conversion");
        return false;
    }

    // CRITICAL: Check for known crash addresses in input data
    uintptr_t inputDataPtr = reinterpret_cast<uintptr_t>(inputData);
#if UINTPTR_MAX == UINT64_MAX
    if (inputDataPtr == 0x100000000ULL) {
        LOGE_HW("❌ CRITICAL: Input data points to crash address 0x100000000!");
        return false;
    }
#endif

    // Calculate output size: each 4-byte length prefix becomes 4-byte start code
    // So output size equals input size (no size change)
    *outputSize = inputSize;

    // Allocate output buffer
    *outputData = (uint8_t*)av_malloc(inputSize);
    if (!*outputData) {
        LOGE_HW("❌ Failed to allocate output buffer for AVCC conversion");
        return false;
    }

    // Start code to replace length prefixes
    const uint8_t startCode[4] = {0x00, 0x00, 0x00, 0x01};

    size_t inputPos = 0;
    size_t outputPos = 0;
    int nalCount = 0;

    // Process all NAL units in the packet
    while (inputPos + 4 <= inputSize) {
        // Read NAL unit length (big-endian 32-bit)
        uint32_t nalLength = (inputData[inputPos] << 24) |
                            (inputData[inputPos + 1] << 16) |
                            (inputData[inputPos + 2] << 8) |
                            inputData[inputPos + 3];

        inputPos += 4; // Skip length prefix

        // Validate NAL length
        if (nalLength == 0 || inputPos + nalLength > inputSize) {
            LOGW_HW("⚠️ Invalid NAL length %u at position %zu", nalLength, inputPos - 4);
            av_free(*outputData);
            *outputData = nullptr;
            return false;
        }

        // Write start code instead of length prefix
        memcpy(*outputData + outputPos, startCode, 4);
        outputPos += 4;

        // Copy NAL unit data
        memcpy(*outputData + outputPos, inputData + inputPos, nalLength);
        outputPos += nalLength;
        inputPos += nalLength;

        nalCount++;

        LOGD_HW("📦 Converted NAL unit %d: length %u bytes", nalCount, nalLength);
    }

    // Verify we consumed all input data
    if (inputPos != inputSize) {
        LOGW_HW("⚠️ AVCC conversion didn't consume all input data: %zu/%zu bytes", inputPos, inputSize);
        // This might be okay if there are trailing bytes, but log it
    }

    LOGI_HW("✅ AVCC→Annex B conversion complete: %d NAL units, %zu bytes", nalCount, *outputSize);
    return true;
}