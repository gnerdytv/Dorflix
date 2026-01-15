#pragma once

#include <memory>
#include <string>
#include <vector>
#include <tuple>
#include "VideoDecoderBase.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

// ===== ENUMERATED CODEC INFO STRUCTURE =====
// Mirrors the JNI bridge tuple structure for codec information
struct EnumeratedCodecInfo {
    std::string name;
    std::string mimeType;
    bool isEncoder;
    bool isHardware;
    int maxWidth;
    int maxHeight;
    int profile;
    int level;
    long performanceScore;
    bool isValid;
    std::vector<std::string> hdrSupport;
    std::vector<std::string> colorFormats;
    int maxBitrate;

    EnumeratedCodecInfo() : isValid(false), performanceScore(-1), maxBitrate(0) {}
};

// Logging macros
#define LOG_TAG_DECODER "VideoDecoderFactory"
#define LOGE_DECODER(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_DECODER, __VA_ARGS__)
#define LOGI_DECODER(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_DECODER, __VA_ARGS__)
#define LOGW_DECODER(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG_DECODER, __VA_ARGS__)
#define LOGD_DECODER(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_DECODER, __VA_ARGS__)

class VideoDecoderFactory {
public:
    /**
     * Selects the best available decoder for the given codec
     * Priority: MediaCodec HW → MediaCodec SW → FFmpeg SW
     */
    static DecoderType selectBestDecoder(const std::string& codecName, AVCodecID codecId);

    /**
     * Creates a decoder instance of the specified type
     */
    static std::unique_ptr<VideoDecoderBase> createDecoder(DecoderType type);

    /**
     * Checks if MediaCodec hardware decoding is supported for a codec
     */
    static bool isMediaCodecHardwareSupported(const std::string& codecName, AVCodecID codecId);

    /**
     * Checks if MediaCodec software decoding is supported for a codec
     */
    static bool isMediaCodecSoftwareSupported(const std::string& codecName, AVCodecID codecId);

    /**
     * Checks if FFmpeg decoding is supported for a codec
     */
    static bool isFFmpegSupported(AVCodecID codecId);

    /**
     * Converts AVCodecID to MIME type for MediaCodec
     */
    static const char* getMimeType(AVCodecID codecId);

    /**
     * Selects the best enumerated codec for the given requirements
     */
    static EnumeratedCodecInfo selectBestEnumeratedCodec(const char* mimeType, int requiredWidth, int requiredHeight);

    /**
     * Checks if an enumerated codec is actually supported
     */
    static bool isEnumeratedCodecSupported(const EnumeratedCodecInfo& codec, const char* mimeType);

    /**
     * Gets the currently selected codec information
     */
    static const EnumeratedCodecInfo& getSelectedCodec();

private:
    VideoDecoderFactory() = delete; // Static only class
};