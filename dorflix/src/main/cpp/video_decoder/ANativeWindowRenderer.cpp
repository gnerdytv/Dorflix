#include "ANativeWindowRenderer.h"
#include <cstring>
#include <algorithm>
#include <android/log.h>

extern "C" {
#include <libswscale/swscale.h>
#include <libavutil/pixdesc.h>
}

// Logging macros
#define LOG_TAG_AN "ANativeWin"
#define LOGE_AN(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_AN, __VA_ARGS__)
#define LOGI_AN(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_AN, __VA_ARGS__)
#define LOGW_AN(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG_AN, __VA_ARGS__)

ANativeWindowRenderer::ANativeWindowRenderer()
    : m_window(nullptr)
    , m_initialized(false)
    , m_surfaceWidth(0)
    , m_surfaceHeight(0)
    , m_swsContext(nullptr)
    , m_rgbBuffer(nullptr)
    , m_rgbBufferSize(0)
{
    LOGI_AN("ANativeWindowRenderer created");
}

ANativeWindowRenderer::~ANativeWindowRenderer() {
    cleanup();
    LOGI_AN("ANativeWindowRenderer destroyed");
}

bool ANativeWindowRenderer::initialize(ANativeWindow* window, int width, int height) {
    LOGI_AN("Initializing ANativeWindow renderer: %dx%d", width, height);

    if (!window) {
        LOGE_AN("No window provided");
        return false;
    }

    m_window = window;
    m_surfaceWidth = width;
    m_surfaceHeight = height;

    // Try different formats in order of preference
    int32_t formats[] = {
        WINDOW_FORMAT_RGBA_8888,
        WINDOW_FORMAT_RGBX_8888,
        WINDOW_FORMAT_RGB_565,
        ANativeWindow_getFormat(window)  // Use current format as fallback
    };

    bool formatSet = false;
    for (int32_t format : formats) {
        if (ANativeWindow_setBuffersGeometry(window, width, height, format) == 0) {
            LOGI_AN("Set window format to %d", format);
            formatSet = true;
            break;
        }
    }

    if (!formatSet) {
        LOGW_AN("Could not set preferred format, using system default");
        // Continue anyway - the system will choose a compatible format
    }

    m_initialized = true;
    LOGI_AN("✅ ANativeWindow renderer initialized successfully");
    return true;
}

bool ANativeWindowRenderer::renderFrame(AVFrame* frame) {
    LOGI_AN("=== RENDER FRAME START ===");
    LOGI_AN("Frame object: %p", frame);

    if (!m_initialized || !frame || !m_window) {
        LOGE_AN("Render failed: not initialized or invalid parameters");
        return false;
    }

    // ===== SURFACE MODE DETECTION =====
    // In surface mode, MediaCodec renders directly to ANativeWindow
    // Frames are synthetic (just timing, no pixel data)
    bool isSyntheticFrame = (frame->format == AV_PIX_FMT_NONE ||
                            frame->format < 0 ||
                            !frame->data[0] ||
                            frame->width == 0 || frame->height == 0);

    if (isSyntheticFrame) {
        LOGI_AN("🎬 Detected synthetic frame (surface mode) - skipping pixel processing");
        LOGI_AN("   → MediaCodec handles direct surface rendering");
        LOGI_AN("   → Frame timing: pts=%" PRId64 ", dimensions=%dx%d, format=%d",
                frame->pts, frame->width, frame->height, frame->format);
        // In surface mode, MediaCodec already rendered to surface - just return success
        return true;
    }

    // Log incoming frame details before validation
    LOGI_AN("Incoming frame details: %dx%d format=%s",
            frame->width, frame->height,
            frame->format >= 0 && frame->format < AV_PIX_FMT_NB ?
            av_get_pix_fmt_name((AVPixelFormat)frame->format) : "INVALID");

    // Log frame data pointers with memory addresses
    // Note: frame->data is always valid (array), check individual pointers
    for (int i = 0; i < 4 && frame->data[i]; i++) {
        LOGI_AN("Frame data[%d]: %p (uintptr=%" PRIuPTR ")", i, frame->data[i],
                reinterpret_cast<uintptr_t>(frame->data[i]));
    }

    // CRITICAL: Comprehensive frame validation before rendering
    if (!validateFrameIntegrity(frame)) {
        LOGE_AN("Render failed: frame integrity validation failed");
        return false;
    }

    LOGI_AN("✅ Frame passed integrity validation - proceeding with rendering");
    LOGI_AN("Rendering frame: %dx%d format=%s data[0]=%p linesize[0]=%d",
            frame->width, frame->height, av_get_pix_fmt_name((AVPixelFormat)frame->format),
            frame->data[0], frame->linesize[0]);

    // Setup scaling context if needed
    if (!setupScalingContext(frame->width, frame->height, (AVPixelFormat)frame->format)) {
        LOGE_AN("Failed to setup scaling context");
        return false;
    }

    // Scale to fit surface dimensions
    int outputWidth = m_surfaceWidth;
    int outputHeight = m_surfaceHeight;

    // Ensure RGB buffer is large enough
    int requiredSize = outputWidth * outputHeight * 4;
    if (!ensureRGBBuffer(requiredSize)) {
        LOGE_AN("Failed to allocate RGB buffer");
        return false;
    }

    // Convert frame format to RGB (handles both YUV420P and BGRA input)
    uint8_t* dstData[4] = {m_rgbBuffer, nullptr, nullptr, nullptr};
    int dstLinesize[4] = {outputWidth * 4, 0, 0, 0};

    LOGI_AN("=== SWS_SCALE OPERATION START ===");
    LOGI_AN("Source: format=%s, dimensions=%dx%d",
            av_get_pix_fmt_name((AVPixelFormat)frame->format), frame->width, frame->height);
    LOGI_AN("Destination: format=RGBA, dimensions=%dx%d, buffer=%p size=%d",
            outputWidth, outputHeight, m_rgbBuffer, m_rgbBufferSize);

    // Validate inputs before sws_scale
    if (!frame->data[0]) {
        LOGE_AN("CRITICAL: frame->data[0] is null before sws_scale!");
        return false;
    }
    if (!m_rgbBuffer) {
        LOGE_AN("CRITICAL: m_rgbBuffer is null before sws_scale!");
        return false;
    }

    LOGI_AN("Pre-sws_scale validation: frame->data[0]=%p, m_rgbBuffer=%p",
            frame->data[0], m_rgbBuffer);

    // Check for crash addresses in frame data
    uintptr_t frameDataPtr = reinterpret_cast<uintptr_t>(frame->data[0]);
#if UINTPTR_MAX == UINT64_MAX
    // 64-bit architecture
    if (frameDataPtr == 0x100000000ULL) {
        LOGE_AN("CRITICAL: Frame data[0] contains crash address 0x100000000!");
        return false;
    }
#else
    // 32-bit architecture
    if (frameDataPtr == 0x10000000) {
        LOGE_AN("CRITICAL: Frame data[0] contains crash address 0x10000000!");
        return false;
    }
#endif

    sws_scale(m_swsContext, frame->data, frame->linesize, 0, frame->height,
              dstData, dstLinesize);

    LOGI_AN("✅ sws_scale completed successfully");

    // Lock window buffer
    ANativeWindow_Buffer buffer;
    if (ANativeWindow_lock(m_window, &buffer, nullptr) != 0) {
        LOGE_AN("Failed to lock window buffer");
        return false;
    }

    LOGI_AN("Buffer info: %dx%d stride=%d format=%d",
            buffer.width, buffer.height, buffer.stride, buffer.format);

    // Safe buffer copy with proper bounds checking
    int copyHeight = std::min(outputHeight, buffer.height);
    int copyWidth = std::min(outputWidth, buffer.width);

    // Ensure buffer dimensions are valid
    if (copyWidth <= 0 || copyHeight <= 0) {
        LOGE_AN("Invalid buffer dimensions: %dx%d", copyWidth, copyHeight);
        ANativeWindow_unlockAndPost(m_window);
        return false;
    }

    uint8_t* dstBuffer = reinterpret_cast<uint8_t*>(buffer.bits);
    if (!dstBuffer) {
        LOGE_AN("Buffer bits is null");
        ANativeWindow_unlockAndPost(m_window);
        return false;
    }

    // Calculate strides in bytes
    int dstStrideBytes = buffer.stride * 4; // RGBA
    int srcStrideBytes = outputWidth * 4;   // RGBA

    // Copy row by row with bounds checking
    for (int y = 0; y < copyHeight; y++) {
        uint8_t* dstRow = dstBuffer + (y * dstStrideBytes);
        uint8_t* srcRow = m_rgbBuffer + (y * srcStrideBytes);

        // Ensure we don't exceed buffer bounds
        size_t bytesToCopy = copyWidth * 4;
        if (dstRow + bytesToCopy > dstBuffer + (buffer.height * dstStrideBytes)) {
            LOGE_AN("Row copy would exceed buffer bounds at y=%d", y);
            break;
        }
        if (srcRow + bytesToCopy > m_rgbBuffer + m_rgbBufferSize) {
            LOGE_AN("Row copy would exceed source buffer bounds at y=%d", y);
            break;
        }

        memcpy(dstRow, srcRow, bytesToCopy);
    }

    // Unlock and post buffer
    if (ANativeWindow_unlockAndPost(m_window) != 0) {
        LOGE_AN("Failed to unlock and post window buffer");
        return false;
    }

    LOGI_AN("✅ Frame rendered successfully via ANativeWindow");
    return true;
}

void ANativeWindowRenderer::setViewport(int width, int height) {
    if (width != m_surfaceWidth || height != m_surfaceHeight) {
        m_surfaceWidth = width;
        m_surfaceHeight = height;

        if (m_window) {
            ANativeWindow_setBuffersGeometry(m_window, width, height, WINDOW_FORMAT_RGBX_8888);
        }

        LOGI_AN("Viewport updated to %dx%d", width, height);
    }
}

void ANativeWindowRenderer::cleanup() {
    LOGI_AN("Cleaning up ANativeWindow renderer");

    if (m_swsContext) {
        sws_freeContext(m_swsContext);
        m_swsContext = nullptr;
    }

    if (m_rgbBuffer) {
        delete[] m_rgbBuffer;
        m_rgbBuffer = nullptr;
        m_rgbBufferSize = 0;
    }

    m_window = nullptr;
    m_initialized = false;
    LOGI_AN("ANativeWindow renderer cleanup completed");
}

bool ANativeWindowRenderer::setupScalingContext(int frameWidth, int frameHeight, AVPixelFormat format) {
    // Check if we need to recreate the context
    // Accept both YUV420P (software decoder) and BGRA (hardware decoder) formats
    if (m_swsContext && frameWidth == m_surfaceWidth && frameHeight == m_surfaceHeight &&
        (format == AV_PIX_FMT_YUV420P || format == AV_PIX_FMT_BGRA)) {
        return true; // Context is still valid
    }

    // Free old context
    if (m_swsContext) {
        sws_freeContext(m_swsContext);
        m_swsContext = nullptr;
    }

    // Create new scaling context
    m_swsContext = sws_getContext(frameWidth, frameHeight, format,
                                  m_surfaceWidth, m_surfaceHeight, AV_PIX_FMT_RGBA,
                                  SWS_BILINEAR, nullptr, nullptr, nullptr);

    if (!m_swsContext) {
        LOGE_AN("Failed to create scaling context: %dx%d %s → %dx%d RGBA",
                frameWidth, frameHeight, av_get_pix_fmt_name(format),
                m_surfaceWidth, m_surfaceHeight);
        return false;
    }

    LOGI_AN("✅ Scaling context created: %dx%d %s → %dx%d RGBA",
            frameWidth, frameHeight, av_get_pix_fmt_name(format),
            m_surfaceWidth, m_surfaceHeight);
    return true;
}

bool ANativeWindowRenderer::ensureRGBBuffer(int requiredSize) {
    if (m_rgbBufferSize >= requiredSize) {
        return true; // Buffer is large enough
    }

    // Free old buffer
    if (m_rgbBuffer) {
        delete[] m_rgbBuffer;
    }

    // Allocate new buffer
    m_rgbBuffer = new (std::nothrow) uint8_t[requiredSize];
    if (!m_rgbBuffer) {
        LOGE_AN("Failed to allocate RGB buffer of size %d", requiredSize);
        m_rgbBufferSize = 0;
        return false;
    }

    m_rgbBufferSize = requiredSize;
    LOGI_AN("✅ RGB buffer allocated: %d bytes", requiredSize);
    return true;
}

bool ANativeWindowRenderer::validateFrameIntegrity(AVFrame* frame) {
    if (!frame) {
        LOGE_AN("Frame is null");
        return false;
    }

    // Basic frame properties validation
    if (frame->width <= 0 || frame->height <= 0 || frame->width > 4096 || frame->height > 4096) {
        LOGE_AN("Invalid frame dimensions: %dx%d", frame->width, frame->height);
        return false;
    }

    if (frame->format < 0 || frame->format >= AV_PIX_FMT_NB) {
        LOGE_AN("Invalid pixel format: %d", frame->format);
        return false;
    }

    // Note: frame->data and frame->linesize are arrays in AVFrame, so they're always valid
    // Individual data pointers will be validated below

    // For BGRA format (hardware decoded), validate data[0]
    if (frame->format == AV_PIX_FMT_BGRA) {
        if (!frame->data[0]) {
            LOGE_AN("Frame data[0] is null for BGRA format");
            return false;
        }

        // Validate pointer is in valid memory range
        uintptr_t dataPtr = reinterpret_cast<uintptr_t>(frame->data[0]);
#if UINTPTR_MAX == UINT64_MAX
        // 64-bit architecture
        if (dataPtr < 0x1000 || dataPtr >= 0x8000000000000000ULL) {
            LOGE_AN("Frame data[0] pointer appears corrupted: %p", frame->data[0]);
            return false;
        }
#else
        // 32-bit architecture
        if (dataPtr < 0x1000 || dataPtr >= 0xC0000000) {  // Check against typical 32-bit address space limit
            LOGE_AN("Frame data[0] pointer appears corrupted: %p", frame->data[0]);
            return false;
        }
#endif

        if (frame->linesize[0] <= 0 || frame->linesize[0] > frame->width * 4 * 2) {
            LOGE_AN("Invalid linesize[0] for BGRA: %d (expected <= %d)",
                    frame->linesize[0], frame->width * 4 * 2);
            return false;
        }

        // Validate buffer size is reasonable
        if (frame->buf[0]) {
            size_t expectedMinSize = (size_t)frame->width * frame->height * 4;
            if (frame->buf[0]->size < expectedMinSize) {
                LOGE_AN("Frame buffer too small: %zu < %zu", frame->buf[0]->size, expectedMinSize);
                return false;
            }
        }
    }

    // Additional validation for other planes if they exist
    for (int i = 1; i < AV_NUM_DATA_POINTERS; i++) {
        if (frame->data[i] && frame->linesize[i] <= 0) {
            LOGE_AN("Invalid linesize[%d]: %d for non-null data", i, frame->linesize[i]);
            return false;
        }
    }

    LOGI_AN("✅ Frame integrity validation passed");
    return true;
}

void ANativeWindowRenderer::renderLetterbox(uint8_t* buffer, int bufferWidth, int bufferHeight,
                                           int videoWidth, int videoHeight) {
    // Calculate aspect ratios
    float videoAspect = static_cast<float>(videoWidth) / videoHeight;
    float bufferAspect = static_cast<float>(bufferWidth) / bufferHeight;

    int letterboxWidth = bufferWidth;
    int letterboxHeight = bufferHeight;
    int offsetX = 0;
    int offsetY = 0;

    if (videoAspect > bufferAspect) {
        // Video is wider - add letterbox top/bottom
        letterboxHeight = static_cast<int>(bufferWidth / videoAspect);
        offsetY = (bufferHeight - letterboxHeight) / 2;
    } else {
        // Video is taller - add letterbox sides
        letterboxWidth = static_cast<int>(bufferHeight * videoAspect);
        offsetX = (bufferWidth - letterboxWidth) / 2;
    }

    // If letterboxing is needed, fill letterbox areas with black
    if (offsetX > 0 || offsetY > 0) {
        // Clear entire buffer first
        memset(buffer, 0, bufferWidth * bufferHeight * 4);

        // Move video content to center (simplified - in practice you'd need to rescale)
        // For now, just log that letterboxing would be applied
        LOGI_AN("Letterboxing: video %dx%d in buffer %dx%d (offset %d,%d)",
                videoWidth, videoHeight, bufferWidth, bufferHeight, offsetX, offsetY);
    }
}