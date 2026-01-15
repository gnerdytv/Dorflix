#pragma once

#include <memory>
#include <string>
#include <android/native_window.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>
}

// Forward declaration for DecoderType
enum class DecoderType {
    MEDIACODEC_HARDWARE,    // Android MediaCodec with hardware acceleration
    MEDIACODEC_SOFTWARE,    // Android MediaCodec software (OMX.google.*)
    FFMPEG_SOFTWARE,        // FFmpeg software decoder
    NONE                    // No decoder available
};

class VideoDecoderBase {
public:
    VideoDecoderBase() = default;
    virtual ~VideoDecoderBase() = default;

    /**
     * Initialize the decoder with format context and stream index
     */
    virtual bool initialize(AVFormatContext* formatContext, int streamIndex) = 0;

    /**
     * Decode a packet and return the resulting frame
     * Returns nullptr if no frame is available (EAGAIN)
     */
    virtual AVFrame* decodePacket(AVPacket* packet) = 0;

    /**
     * Get the current decoder type
     */
    virtual DecoderType getDecoderType() const = 0;

    /**
     * Get decoder name for logging
     */
    virtual const char* getDecoderName() const = 0;

    /**
     * Flush decoder buffers
     */
    virtual void flush() = 0;

    /**
     * Get codec context (for FFmpeg decoders)
     */
    virtual AVCodecContext* getCodecContext() { return nullptr; }

    /**
     * Check if decoder is ready
     */
    virtual bool isReady() const = 0;

    // Prevent copying
    VideoDecoderBase(const VideoDecoderBase&) = delete;
    VideoDecoderBase& operator=(const VideoDecoderBase&) = delete;
};