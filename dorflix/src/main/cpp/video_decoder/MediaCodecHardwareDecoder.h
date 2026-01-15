#pragma once

#include "VideoDecoderBase.h"
#include "VideoDecoderFactory.h"
#include <media/NdkMediaCodec.h>
#include <media/NdkMediaFormat.h>
#include <android/native_window.h>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>
#include <tuple>
#include <vector>
#include <string>

// Forward declarations for Java bridge enumeration functions
// These are defined in jni_interface.cpp but used in MediaCodecHardwareDecoder.cpp
std::vector<EnumeratedCodecInfo> getEnumeratedCodecs();
bool isEnumerationComplete();

class MediaCodecHardwareDecoder : public VideoDecoderBase {
public:
    // CSD format enumeration for dynamic device adaptation
    enum class CsdFormat {
        RAW_AVC,           // Raw avcC extradata as csd-0
        RAW_NAL_UNITS,     // SPS/PPS as raw NAL units (no start codes)
        ANNEX_B_NAL_UNITS, // SPS/PPS with start codes (Annex B)
        SPS_PPS_CONCAT,    // SPS+PPS concatenated in csd-0 only
        NO_CSD             // No CSD - let codec parse from packets
    };

    MediaCodecHardwareDecoder();
    ~MediaCodecHardwareDecoder() override;

    bool initialize(AVFormatContext* formatContext, int streamIndex) override;
    AVFrame* decodePacket(AVPacket* packet) override;
    DecoderType getDecoderType() const override { return DecoderType::MEDIACODEC_HARDWARE; }
    const char* getDecoderName() const override { return "MediaCodec Hardware"; }
    void flush() override;
    bool isReady() const override { return m_codec != nullptr && m_initialized; }

    /**
     * Set output surface for hardware decoding
     */
    void setOutputSurface(ANativeWindow* surface);

    /**
     * Set frame delivery callback for decoded frames
     */
    void setFrameDeliveryCallback(std::function<void(AVFrame* frame)> callback);

    /**
     * Set the selected codec information for optimal configuration
     */
    void setSelectedCodecInfo(const EnumeratedCodecInfo& codecInfo);

private:
    AMediaCodec* m_codec;
    ANativeWindow* m_surface;
    bool m_initialized;

    int m_width;
    int m_height;
    AVCodecID m_codecId;

    // Frame conversion for FFmpeg compatibility
    struct SwsContext* m_swsContext;
    AVFrame* m_outputFrame;

    // Failure tracking and recovery - separate counters for different failure types
    int m_consecutiveFailures;
    int m_maxConsecutiveFailures;
    int m_consecutiveTryAgainLater;  // Persistent TRY_AGAIN_LATER failures
    int m_maxPersistentTryAgainLater;  // Threshold for codec flush
    int m_exponentialBackoffUs;       // Exponential backoff for TRY_AGAIN_LATER

    // Dynamic buffer capacity detection and adaptive rate limiting
    int m_inputBufferCount;
    size_t m_inputBufferSize;
    int m_maxPacketsPerSecond;
    int64_t m_minPacketIntervalUs;
    int64_t m_lastPacketTimeUs;

    // Advanced packet queuing system
    struct QueuedPacket {
        AVPacket* packet;
        int64_t queueTimeUs;

        QueuedPacket(AVPacket* pkt)
            : packet(pkt), queueTimeUs(0) {}
    };

    std::queue<std::unique_ptr<QueuedPacket>> m_packetQueue;
    std::mutex m_queueMutex;
    std::condition_variable m_queueCondition;
    std::thread m_queueProcessorThread;
    bool m_stopQueueProcessor;
    bool m_isInLoadingState;

    // Frame delivery callback mechanism
    std::function<void(AVFrame* frame)> m_frameDeliveryCallback;

    // Redownload callback mechanism - REMOVED
    // Corrupted packets are now logged and dropped immediately

    // Surface mode detection - critical for correct operation
    bool m_surfaceMode;

    // ===== DECODER WATCHDOG SYSTEM =====
    // Monitors for stalled decoding and triggers recovery
    std::chrono::steady_clock::time_point m_lastOutputFrameTime;
    int m_watchdogTimeoutMs;
    bool m_watchdogActive;

    // ===== ADAPTIVE STARTUP RATE LIMITING =====
    // Allows burst during startup, then clamps to steady-state FPS
    enum class RateLimitState { STARTUP, STEADY_STATE };
    RateLimitState m_rateLimitState;
    int m_startupBurstCount;
    int m_maxStartupBurst;
    bool m_firstFrameReceived;

    // ===== SELECTED CODEC INFORMATION =====
    // Codec selected by VideoDecoderFactory for optimal configuration
    EnumeratedCodecInfo m_selectedCodec;

    // ===== DECODER WATCHDOG METHODS =====
    void updateWatchdogOnFrameReceived();
    void checkWatchdogTimeout();
    void triggerWatchdogRecovery();

    // ===== SPS/PPS EXTRACTION FROM EXTRADATA =====
    bool extractSpsPpsFromExtradata(const uint8_t* extradata, size_t extradataSize,
                                   uint8_t** spsData, size_t* spsSize,
                                   uint8_t** ppsData, size_t* ppsSize);

    // ===== AVCC TO ANNEX B CONVERSION =====
    // Converts length-prefixed NAL units (AVCC) to start-code prefixed (Annex B)
    // Required because MediaCodec expects Annex B format when CSD is provided separately
    bool convertAvccToAnnexB(const uint8_t* inputData, size_t inputSize,
                            uint8_t** outputData, size_t* outputSize);

    // ===== ADD START CODE TO RAW NAL UNIT =====
    // Adds Annex B start code (0x00000001) to raw NAL unit data
    uint8_t* addStartCode(const uint8_t* nalData, size_t nalSize, size_t* outputSize);

    // ===== ENUMERATION-BASED CSD FORMAT SELECTION =====
    // Determines optimal CSD format based on Java bridge enumeration results
    bool shouldUseAnnexBFormat(const char* mimeType);

    // ===== CODEC ENUMERATION ENSURANCE =====
    // Ensures that codec enumeration has been performed before CSD format selection
    bool ensureCodecEnumeration();



    // Private methods for buffer capability detection
    bool queryBufferCapabilities();
    void calculateAdaptiveLimits(int bufferCount, size_t bufferSize);
    int estimateInputBufferCount();
    size_t estimateInputBufferSize();

    bool configureCodec(const char* mimeType, int width, int height, AVCodecParameters* codecParams, AVFormatContext* formatContext);
    bool setupFrameConversion(int width, int height);
    AVFrame* convertHardwareFrame(const uint8_t* data, size_t size, int width, int height);

    // Helper methods
    bool dequeueInputBuffer(ssize_t* index, int64_t timeoutUs = 10000);
    bool dequeueOutputBuffer(AMediaCodecBufferInfo* info, ssize_t* index, int64_t timeoutUs = 10000);
    void checkFailureRecovery();

    // ===== CONTINUOUS OUTPUT DRAINING =====
    void drainOutputNonBlocking();



    // Advanced queuing and corruption handling methods
    void startQueueProcessor();
    void stopQueueProcessor();
    void queueProcessorThread();
    void queuePacketForProcessing(AVPacket* packet, bool isRateLimited = false, bool isCorrupted = false);
    AVFrame* tryProcessQueuedPacket();
    AVFrame* decodeQueuedPacket(AVPacket* packet);
    bool isPacketCorrupted(AVPacket* packet);
    void setLoadingState(bool loading);
    bool isInLoadingState() const;
};