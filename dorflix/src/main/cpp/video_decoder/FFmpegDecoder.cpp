#include "FFmpegDecoder.h"
#include <android/log.h>

#define LOG_TAG_FFMPEG "FFmpegDecoder"
#define LOGE_FFMPEG(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_FFMPEG, __VA_ARGS__)
#define LOGI_FFMPEG(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_FFMPEG, __VA_ARGS__)
#define LOGW_FFMPEG(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG_FFMPEG, __VA_ARGS__)

FFmpegDecoder::FFmpegDecoder()
    : m_codecContext(nullptr)
    , m_frame(nullptr)
    , m_initialized(false)
    , m_streamIndex(-1)
{
    LOGI_FFMPEG("FFmpegDecoder created");
}

FFmpegDecoder::~FFmpegDecoder() {
    LOGI_FFMPEG("Destroying FFmpegDecoder");

    if (m_frame) {
        av_frame_free(&m_frame);
        m_frame = nullptr;
    }

    if (m_codecContext) {
        avcodec_free_context(&m_codecContext);
        m_codecContext = nullptr;
    }

    m_initialized = false;
    LOGI_FFMPEG("FFmpegDecoder destroyed");
}

bool FFmpegDecoder::initialize(AVFormatContext* formatContext, int streamIndex) {
    LOGI_FFMPEG("=== INITIALIZING FFMPEG SOFTWARE DECODER ===");

    if (!formatContext || streamIndex < 0 || streamIndex >= (int)formatContext->nb_streams) {
        LOGE_FFMPEG("Invalid format context or stream index");
        return false;
    }

    AVStream* stream = formatContext->streams[streamIndex];
    AVCodecParameters* codecParams = stream->codecpar;

    // Find decoder
    const AVCodec* codec = avcodec_find_decoder(codecParams->codec_id);
    if (!codec) {
        LOGE_FFMPEG("No decoder found for codec_id %d", codecParams->codec_id);
        return false;
    }

    // Allocate codec context
    m_codecContext = avcodec_alloc_context3(codec);
    if (!m_codecContext) {
        LOGE_FFMPEG("Failed to allocate codec context");
        return false;
    }

    // Copy codec parameters
    int ret = avcodec_parameters_to_context(m_codecContext, codecParams);
    if (ret < 0) {
        LOGE_FFMPEG("Failed to copy codec parameters: %d", ret);
        avcodec_free_context(&m_codecContext);
        m_codecContext = nullptr;
        return false;
    }

    // Initialize decoder
    ret = avcodec_open2(m_codecContext, codec, nullptr);
    if (ret < 0) {
        LOGE_FFMPEG("Failed to open codec: %d", ret);
        avcodec_free_context(&m_codecContext);
        m_codecContext = nullptr;
        return false;
    }

    // Allocate frame
    m_frame = av_frame_alloc();
    if (!m_frame) {
        LOGE_FFMPEG("Failed to allocate frame");
        avcodec_free_context(&m_codecContext);
        m_codecContext = nullptr;
        return false;
    }

    m_streamIndex = streamIndex;
    m_initialized = true;

    LOGI_FFMPEG("✅ FFmpeg software decoder initialized successfully");
    LOGI_FFMPEG("   → Codec: %s, Resolution: %dx%d",
            avcodec_get_name(m_codecContext->codec_id),
            m_codecContext->width, m_codecContext->height);

    return true;
}

AVFrame* FFmpegDecoder::decodePacket(AVPacket* packet) {
    if (!m_initialized || !packet) {
        LOGW_FFMPEG("Decoder not ready or invalid packet");
        return nullptr;
    }

    // Send packet to decoder
    int ret = avcodec_send_packet(m_codecContext, packet);
    if (ret < 0) {
        LOGE_FFMPEG("Failed to send packet to decoder: %d", ret);
        return nullptr;
    }

    // Receive frame from decoder
    ret = avcodec_receive_frame(m_codecContext, m_frame);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
        // No frame available yet
        return nullptr;
    } else if (ret < 0) {
        LOGE_FFMPEG("Failed to receive frame from decoder: %d", ret);
        return nullptr;
    }

    // Clone the frame for the caller (since m_frame will be reused)
    AVFrame* outputFrame = av_frame_clone(m_frame);
    if (!outputFrame) {
        LOGE_FFMPEG("Failed to clone decoded frame");
        return nullptr;
    }

    LOGI_FFMPEG("✅ Frame decoded successfully: %dx%d format=%d pts=%" PRId64,
            outputFrame->width, outputFrame->height, outputFrame->format, outputFrame->pts);

    return outputFrame;
}

void FFmpegDecoder::flush() {
    if (m_codecContext) {
        avcodec_flush_buffers(m_codecContext);
        LOGI_FFMPEG("FFmpeg decoder flushed");
    }
}