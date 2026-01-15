#pragma once

#include "VideoDecoderBase.h"
#include <memory>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/frame.h>
#include <libswscale/swscale.h>
}

class FFmpegDecoder : public VideoDecoderBase {
public:
    FFmpegDecoder();
    ~FFmpegDecoder() override;

    bool initialize(AVFormatContext* formatContext, int streamIndex) override;
    AVFrame* decodePacket(AVPacket* packet) override;
    DecoderType getDecoderType() const override { return DecoderType::FFMPEG_SOFTWARE; }
    const char* getDecoderName() const override { return "FFmpeg Software"; }
    void flush() override;
    AVCodecContext* getCodecContext() override { return m_codecContext; }
    bool isReady() const override { return m_codecContext != nullptr && m_initialized; }

private:
    AVCodecContext* m_codecContext;
    AVFrame* m_frame;
    bool m_initialized;
    int m_streamIndex;
};