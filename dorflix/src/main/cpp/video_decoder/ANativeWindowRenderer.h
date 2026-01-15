#pragma once

#include "VideoRenderer.h"

class ANativeWindowRenderer : public VideoRenderer {
public:
    ANativeWindowRenderer();
    ~ANativeWindowRenderer() override;

    bool initialize(ANativeWindow* window, int width, int height) override;
    bool renderFrame(AVFrame* frame) override;
    void setViewport(int width, int height) override;
    bool isInitialized() const override { return m_window != nullptr && m_initialized; }
    void cleanup() override;

private:
    ANativeWindow* m_window;
    bool m_initialized;
    int m_surfaceWidth;
    int m_surfaceHeight;

    // Frame conversion for YUV→RGB
    struct SwsContext* m_swsContext;
    uint8_t* m_rgbBuffer;
    int m_rgbBufferSize;

    bool setupScalingContext(int frameWidth, int frameHeight, AVPixelFormat format);
    bool ensureRGBBuffer(int requiredSize);
    bool validateFrameIntegrity(AVFrame* frame);
    void renderLetterbox(uint8_t* buffer, int bufferWidth, int bufferHeight,
                        int videoWidth, int videoHeight);
};