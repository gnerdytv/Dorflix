#include "VideoRendererFactory.h"
#include "OpenGLVideoRenderer.h"
#include "ANativeWindowRenderer.h"
#include "GPUCapabilitiesDetector.h"
#include <android/log.h>

// Logging macros
#define LOG_TAG_RENDERER "RendererFactory"
#define LOGE_RENDERER(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_RENDERER, __VA_ARGS__)
#define LOGI_RENDERER(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_RENDERER, __VA_ARGS__)
#define LOGW_RENDERER(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG_RENDERER, __VA_ARGS__)

RendererType VideoRendererFactory::selectBestRenderer() {
    LOGI_RENDERER("🎨 Selecting hardware-accelerated video renderer...");

    // Only use ANativeWindow renderer (works with MediaCodec hardware)
    // This provides direct hardware-to-hardware rendering pipeline
    if (isANativeWindowSupported()) {
        LOGI_RENDERER("✅ Hardware-accelerated rendering available (ANativeWindow)");
        return RendererType::CPU_ANATIVE;
    }

    // No fallback - hardware rendering is required
    LOGE_RENDERER("❌ Hardware rendering not available - this should not happen on modern Android");
    return RendererType::CPU_CANVAS; // Emergency fallback, though unlikely to work
}

std::unique_ptr<VideoRenderer> VideoRendererFactory::createRenderer(RendererType type) {
    LOGI_RENDERER("Creating renderer of type: %d", static_cast<int>(type));

    switch (type) {
        case RendererType::GPU_OPENGL:
            return std::make_unique<OpenGLVideoRenderer>();

        case RendererType::CPU_ANATIVE:
            return std::make_unique<ANativeWindowRenderer>();

        case RendererType::GPU_VULKAN:
            LOGW_RENDERER("Vulkan renderer not implemented yet, using OpenGL");
            return std::make_unique<OpenGLVideoRenderer>();

        case RendererType::CPU_CANVAS:
            LOGW_RENDERER("Canvas renderer not implemented yet, using ANativeWindow");
            return std::make_unique<ANativeWindowRenderer>();

        default:
            LOGE_RENDERER("Unknown renderer type requested: %d", static_cast<int>(type));
            return nullptr;
    }
}

bool VideoRendererFactory::isGPURenderingSupported() {
    LOGI_RENDERER("Checking GPU rendering support...");

    // Use GPU capabilities detector
    GPUCapabilitiesDetector detector;
    GPUCapabilities caps = detector.detectCapabilities();

    // Check if OpenGL ES is supported and recommended
    bool hasOpenGLES = caps.supportsOpenGLES2 || caps.supportsOpenGLES3;
    bool isRecommended = caps.recommendedStrategy != GPUCapabilities::STRATEGY_SOFTWARE_ONLY;

    if (hasOpenGLES && isRecommended) {
        LOGI_RENDERER("GPU supports OpenGL ES rendering (recommended)");
        return true;
    }

    LOGW_RENDERER("GPU does not support OpenGL ES rendering");
    return false;
}

bool VideoRendererFactory::isANativeWindowSupported() {
    LOGI_RENDERER("Checking ANativeWindow rendering support...");

    // ANativeWindow is always available on Android API 9+
    // We could add more sophisticated checks here if needed
    LOGI_RENDERER("ANativeWindow rendering is supported");
    return true;
}