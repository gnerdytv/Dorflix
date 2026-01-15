#pragma once

#include <memory>
#include <string>
#include "VideoRenderer.h"

enum class RendererType {
    GPU_OPENGL,      // OpenGL ES hardware acceleration
    GPU_VULKAN,      // Vulkan hardware acceleration (future)
    CPU_ANATIVE,     // ANativeWindow CPU rendering
    CPU_CANVAS       // Android Canvas CPU rendering
};

class VideoRendererFactory {
public:
    /**
     * Selects the best available renderer
     * Priority: GPU Hardware → CPU ANativeWindow → CPU Canvas
     */
    static RendererType selectBestRenderer();

    /**
     * Creates a renderer instance of the specified type
     */
    static std::unique_ptr<VideoRenderer> createRenderer(RendererType type);

    /**
     * Checks if GPU rendering is supported
     */
    static bool isGPURenderingSupported();

    /**
     * Checks if ANativeWindow rendering is supported
     */
    static bool isANativeWindowSupported();

private:
    VideoRendererFactory() = delete; // Static only class
};