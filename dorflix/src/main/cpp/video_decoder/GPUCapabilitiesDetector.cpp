#include "GPUCapabilitiesDetector.h"
#include <android/log.h>
#include <cstring>
#include <algorithm>

GPUCapabilitiesDetector::GPUCapabilitiesDetector()
    : m_tempDisplay(EGL_NO_DISPLAY)
    , m_tempContext(EGL_NO_CONTEXT)
    , m_tempSurface(EGL_NO_SURFACE)
{
    LOGI_GPU("GPUCapabilitiesDetector created");
}

GPUCapabilitiesDetector::~GPUCapabilitiesDetector() {
    destroyTempEGLContext();
    LOGI_GPU("GPUCapabilitiesDetector destroyed");
}

GPUCapabilities GPUCapabilitiesDetector::detectCapabilities() {
    LOGI_GPU("=== STARTING GPU CAPABILITY DETECTION ===");

    GPUCapabilities caps = {};

    // Initialize with defaults
    caps.supportsOpenGLES1 = false;
    caps.supportsOpenGLES2 = false;
    caps.supportsOpenGLES3 = false;
    caps.maxTextureSize = 0;
    caps.maxViewportWidth = 0;
    caps.maxViewportHeight = 0;
    caps.performanceClass = GPUCapabilities::PERFORMANCE_UNKNOWN;
    caps.recommendedStrategy = GPUCapabilities::STRATEGY_SOFTWARE_ONLY;
    caps.hasKnownBugs = false;

    // First check if OpenGL ES is available at all
    if (!isOpenGLESAvailable()) {
        LOGW_GPU("OpenGL ES not available - falling back to software rendering");
        caps.recommendedStrategy = GPUCapabilities::STRATEGY_SOFTWARE_ONLY;
        return caps;
    }

    // Try to create temporary context for detailed detection
    if (!createTempEGLContext()) {
        LOGW_GPU("Cannot create temporary EGL context - basic detection only");
        // Still try basic EGL detection
        detectEGLCapabilities();
        return caps;
    }

    // Full capability detection
    if (detectOpenGLESCapabilities(caps)) {
        identifyGPU(caps);
        classifyPerformance(caps);
        determineStrategy(caps);

        LOGI_GPU("=== GPU DETECTION COMPLETE ===");
        LOGI_GPU("GPU: %s %s", caps.gpuVendor.c_str(), caps.gpuRenderer.c_str());
        LOGI_GPU("OpenGL ES: %s", caps.supportsOpenGLES3 ? "3.0" : (caps.supportsOpenGLES2 ? "2.0" : "None"));
        LOGI_GPU("Performance: %d/4", (int)caps.performanceClass);
        LOGI_GPU("Strategy: %d", (int)caps.recommendedStrategy);

        if (caps.hasKnownBugs) {
            LOGW_GPU("⚠️ Known GPU bugs detected:");
            for (const auto& issue : caps.knownIssues) {
                LOGW_GPU("  - %s", issue.c_str());
            }
        }
    }

    destroyTempEGLContext();
    return caps;
}

bool GPUCapabilitiesDetector::isOpenGLESAvailable() {
    LOGI_GPU("Checking OpenGL ES availability...");

    // Try to get EGL display
    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY) {
        LOGE_GPU("eglGetDisplay failed");
        return false;
    }

    // Try to initialize EGL
    EGLint major, minor;
    if (!eglInitialize(display, &major, &minor)) {
        LOGE_GPU("eglInitialize failed");
        return false;
    }

    LOGI_GPU("EGL %d.%d initialized successfully", major, minor);

    // Check for OpenGL ES support
    const char* extensions = eglQueryString(display, EGL_EXTENSIONS);
    if (extensions) {
        LOGI_GPU("EGL Extensions: %s", extensions);

        // Check for OpenGL ES support
        if (strstr(extensions, "EGL_OPENGL_ES3_BIT") ||
            strstr(extensions, "EGL_OPENGL_ES2_BIT") ||
            strstr(extensions, "EGL_OPENGL_ES_BIT")) {
            LOGI_GPU("✅ OpenGL ES supported by EGL");
            eglTerminate(display);
            return true;
        }
    }

    LOGW_GPU("❌ No OpenGL ES support detected in EGL extensions");
    eglTerminate(display);
    return false;
}

bool GPUCapabilitiesDetector::isOpenGLES1Supported() {
    // Try to create ES 1.1 context (version 1)
    const EGLint contextAttrs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 1,
        EGL_NONE
    };

    EGLContext context = eglCreateContext(m_tempDisplay, nullptr, EGL_NO_CONTEXT, contextAttrs);
    if (context != EGL_NO_CONTEXT) {
        eglDestroyContext(m_tempDisplay, context);
        return true;
    }

    return false;
}

bool GPUCapabilitiesDetector::isOpenGLES2Supported() {
    // Try to create ES 2.0 context
    const EGLint contextAttrs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    EGLContext context = eglCreateContext(m_tempDisplay, nullptr, EGL_NO_CONTEXT, contextAttrs);
    if (context != EGL_NO_CONTEXT) {
        eglDestroyContext(m_tempDisplay, context);
        return true;
    }

    return false;
}

bool GPUCapabilitiesDetector::isOpenGLES3Supported() {
    // Try to create ES 3.0 context
    const EGLint contextAttrs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
    };

    EGLContext context = eglCreateContext(m_tempDisplay, nullptr, EGL_NO_CONTEXT, contextAttrs);
    if (context != EGL_NO_CONTEXT) {
        eglDestroyContext(m_tempDisplay, context);
        return true;
    }

    return false;
}

bool GPUCapabilitiesDetector::detectEGLCapabilities() {
    LOGI_GPU("Detecting basic EGL capabilities...");

    // This would detect EGL-level capabilities without creating a context
    // For now, we rely on the context-based detection
    return true;
}

bool GPUCapabilitiesDetector::detectOpenGLESCapabilities(GPUCapabilities& caps) {
    LOGI_GPU("Detecting OpenGL ES capabilities...");

    // Make our temporary context current
    if (!eglMakeCurrent(m_tempDisplay, m_tempSurface, m_tempSurface, m_tempContext)) {
        LOGE_GPU("Failed to make temporary context current");
        return false;
    }

    // Get basic info
    caps.gpuVendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    caps.gpuRenderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    caps.glVersion = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    caps.glslVersion = reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION));

    LOGI_GPU("GPU Vendor: %s", caps.gpuVendor.c_str());
    LOGI_GPU("GPU Renderer: %s", caps.gpuRenderer.c_str());
    LOGI_GPU("OpenGL ES Version: %s", caps.glVersion.c_str());
    LOGI_GPU("GLSL Version: %s", caps.glslVersion.c_str());

    // Check version support
    caps.supportsOpenGLES1 = isOpenGLES1Supported();
    caps.supportsOpenGLES2 = isOpenGLES2Supported();
    caps.supportsOpenGLES3 = isOpenGLES3Supported();

    // Get limits
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &caps.maxTextureSize);
    glGetIntegerv(GL_MAX_VIEWPORT_DIMS, &caps.maxViewportWidth);
    caps.maxViewportHeight = caps.maxViewportWidth; // GL_MAX_VIEWPORT_DIMS returns array

    LOGI_GPU("Max texture size: %d", caps.maxTextureSize);
    LOGI_GPU("Max viewport: %dx%d", caps.maxViewportWidth, caps.maxViewportHeight);

    // Get extensions
    const char* extensions = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
    std::string extStr = extensions ? extensions : "";

    // Check specific extensions
    caps.supportsNPOTTextures = checkExtension("GL_OES_texture_npot") ||
                               checkExtension("GL_ARB_texture_non_power_of_two");

    caps.supportsETC1Compression = checkExtension("GL_OES_compressed_ETC1_RGB8_texture");
    caps.supportsETC2Compression = checkExtension("GL_OES_compressed_ETC2_RGB8_texture") ||
                                  checkExtension("GL_OES_compressed_ETC2_RGBA8_texture");
    caps.supportsASTCCompression = checkExtension("GL_KHR_texture_compression_astc_ldr");
    caps.supportsS3TCCompression = checkExtension("GL_EXT_texture_compression_s3tc");

    caps.supportsFramebuffers = checkExtension("GL_OES_framebuffer_object") ||
                               hasExtension(extStr, "GL_ARB_framebuffer_object");

    caps.supportsVAO = checkExtension("GL_OES_vertex_array_object") ||
                      hasExtension(extStr, "GL_ARB_vertex_array_object");

    // Get shader limits
    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_VECTORS, &caps.maxVertexUniformVectors);
    glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_VECTORS, &caps.maxFragmentUniformVectors);
    glGetIntegerv(GL_MAX_VARYING_VECTORS, &caps.maxVaryingVectors);

    LOGI_GPU("Extensions detected: NPOT=%d, ETC1=%d, ETC2=%d, ASTC=%d, S3TC=%d",
             caps.supportsNPOTTextures, caps.supportsETC1Compression,
             caps.supportsETC2Compression, caps.supportsASTCCompression,
             caps.supportsS3TCCompression);

    return true;
}

bool GPUCapabilitiesDetector::checkExtension(const char* extension) {
    return eglGetProcAddress(extension) != nullptr;
}

bool GPUCapabilitiesDetector::hasExtension(const std::string& extensions, const std::string& ext) {
    return extensions.find(ext) != std::string::npos;
}

void GPUCapabilitiesDetector::identifyGPU(GPUCapabilities& caps) {
    std::string vendor = caps.gpuVendor;
    std::string renderer = caps.gpuRenderer;

    // Convert to lowercase for case-insensitive matching
    std::transform(vendor.begin(), vendor.end(), vendor.begin(), ::tolower);
    std::transform(renderer.begin(), renderer.end(), renderer.begin(), ::tolower);

    // Identify known GPU families and check for known issues
    if (vendor.find("qualcomm") != std::string::npos || vendor.find("qcom") != std::string::npos) {
        // Qualcomm Adreno GPUs
        if (renderer.find("adreno") != std::string::npos) {
            LOGI_GPU("📱 Detected Qualcomm Adreno GPU");

            // Check for known Adreno issues
            if (renderer.find("2") != std::string::npos || renderer.find("3") != std::string::npos) {
                // Older Adreno GPUs (2xx, 3xx series)
                caps.hasKnownBugs = true;
                caps.knownIssues.push_back("Older Adreno GPU - may have shader precision issues");
            }
        }
    }
    else if (vendor.find("arm") != std::string::npos || vendor.find("mali") != std::string::npos) {
        // ARM Mali GPUs
        LOGI_GPU("📱 Detected ARM Mali GPU");

        if (renderer.find("mali") != std::string::npos) {
            // Some Mali GPUs have issues with certain shader operations
            if (renderer.find("t6") != std::string::npos || renderer.find("t7") != std::string::npos) {
                caps.hasKnownBugs = true;
                caps.knownIssues.push_back("Mali T6xx/T7xx - potential shader compilation issues");
            }
        }
    }
    else if (vendor.find("imagination") != std::string::npos || vendor.find("powervr") != std::string::npos) {
        // Imagination PowerVR GPUs
        LOGI_GPU("📱 Detected Imagination PowerVR GPU");

        // PowerVR generally has good OpenGL ES support
        // Some older versions might have VAO issues
    }
    else if (vendor.find("nvidia") != std::string::npos) {
        // NVIDIA GPUs (less common in mobile)
        LOGI_GPU("📱 Detected NVIDIA GPU");
    }
    else if (vendor.find("intel") != std::string::npos) {
        // Intel GPUs (rare in mobile)
        LOGI_GPU("📱 Detected Intel GPU");
    }
    else {
        LOGI_GPU("📱 Unknown GPU vendor: %s", caps.gpuVendor.c_str());
    }
}

void GPUCapabilitiesDetector::classifyPerformance(GPUCapabilities& caps) {
    // Classify GPU performance based on renderer string and capabilities

    std::string renderer = caps.gpuRenderer;
    std::transform(renderer.begin(), renderer.end(), renderer.begin(), ::tolower);

    if (renderer.find("adreno") != std::string::npos) {
        // Qualcomm Adreno classification
        if (renderer.find("740") != std::string::npos || renderer.find("750") != std::string::npos) {
            caps.performanceClass = GPUCapabilities::PERFORMANCE_FLAGship;
        } else if (renderer.find("730") != std::string::npos || renderer.find("735") != std::string::npos ||
                   renderer.find("650") != std::string::npos || renderer.find("660") != std::string::npos) {
            caps.performanceClass = GPUCapabilities::PERFORMANCE_HIGH_END;
        } else if (renderer.find("630") != std::string::npos || renderer.find("640") != std::string::npos ||
                   renderer.find("540") != std::string::npos || renderer.find("610") != std::string::npos) {
            caps.performanceClass = GPUCapabilities::PERFORMANCE_MID_RANGE;
        } else {
            caps.performanceClass = GPUCapabilities::PERFORMANCE_LOW_END;
        }
    }
    else if (renderer.find("mali") != std::string::npos) {
        // ARM Mali classification
        if (renderer.find("g710") != std::string::npos || renderer.find("g715") != std::string::npos ||
            renderer.find("g720") != std::string::npos) {
            caps.performanceClass = GPUCapabilities::PERFORMANCE_HIGH_END;
        } else if (renderer.find("g610") != std::string::npos || renderer.find("g615") != std::string::npos ||
                   renderer.find("g620") != std::string::npos || renderer.find("g510") != std::string::npos) {
            caps.performanceClass = GPUCapabilities::PERFORMANCE_MID_RANGE;
        } else {
            caps.performanceClass = GPUCapabilities::PERFORMANCE_LOW_END;
        }
    }
    else if (renderer.find("powervr") != std::string::npos) {
        // PowerVR classification (generally mid-range)
        caps.performanceClass = GPUCapabilities::PERFORMANCE_MID_RANGE;
    }
    else {
        // Unknown GPU - assume mid-range
        caps.performanceClass = GPUCapabilities::PERFORMANCE_MID_RANGE;
    }

    // Adjust based on capabilities
    if (caps.maxTextureSize < 2048) {
        caps.performanceClass = GPUCapabilities::PERFORMANCE_LOW_END;
    }
}

void GPUCapabilitiesDetector::determineStrategy(GPUCapabilities& caps) {
    // Determine recommended rendering strategy based on capabilities and performance

    if (!caps.supportsOpenGLES1 && !caps.supportsOpenGLES2 && !caps.supportsOpenGLES3) {
        caps.recommendedStrategy = GPUCapabilities::STRATEGY_SOFTWARE_ONLY;
        LOGI_GPU("No OpenGL ES support - software rendering only");
        return;
    }

    // Check for minimum requirements
    bool hasBasicSupport = caps.supportsFramebuffers && caps.maxTextureSize >= 1024;

    if (!hasBasicSupport && !caps.supportsOpenGLES1) {
        caps.recommendedStrategy = GPUCapabilities::STRATEGY_SOFTWARE_ONLY;
        LOGI_GPU("Insufficient GPU capabilities - software rendering recommended");
        return;
    }

    // Determine strategy based on performance and features
    if (caps.supportsOpenGLES3 && caps.performanceClass >= GPUCapabilities::PERFORMANCE_MID_RANGE) {
        caps.recommendedStrategy = GPUCapabilities::STRATEGY_OPENGLES3_FULL;
        LOGI_GPU("Full OpenGL ES 3.0 rendering recommended");
    }
    else if (caps.supportsOpenGLES2 && caps.performanceClass >= GPUCapabilities::PERFORMANCE_LOW_END) {
        if (caps.performanceClass >= GPUCapabilities::PERFORMANCE_HIGH_END) {
            caps.recommendedStrategy = GPUCapabilities::STRATEGY_OPENGLES2_OPTIMIZED;
            LOGI_GPU("Optimized OpenGL ES 2.0 rendering recommended");
        } else {
            caps.recommendedStrategy = GPUCapabilities::STRATEGY_OPENGLES2_BASIC;
            LOGI_GPU("Basic OpenGL ES 2.0 rendering recommended");
        }
    }
    else if (caps.supportsOpenGLES1) {
        caps.recommendedStrategy = GPUCapabilities::STRATEGY_OPENGLES1_BASIC;
        LOGI_GPU("Basic OpenGL ES 1.1 rendering recommended");
    }
    else {
        caps.recommendedStrategy = GPUCapabilities::STRATEGY_SOFTWARE_ONLY;
        LOGI_GPU("GPU too limited - software rendering recommended");
    }
}

bool GPUCapabilitiesDetector::createTempEGLContext() {
    LOGI_GPU("Creating temporary EGL context for capability detection...");

    // Get display
    m_tempDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (m_tempDisplay == EGL_NO_DISPLAY) {
        LOGE_GPU("Failed to get EGL display");
        return false;
    }

    // Initialize
    EGLint major, minor;
    if (!eglInitialize(m_tempDisplay, &major, &minor)) {
        LOGE_GPU("Failed to initialize EGL");
        return false;
    }

    // Choose a basic config
    const EGLint configAttrs[] = {
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,  // Use pbuffer for offscreen
        EGL_NONE
    };

    EGLConfig config;
    EGLint numConfigs;
    if (!eglChooseConfig(m_tempDisplay, configAttrs, &config, 1, &numConfigs) || numConfigs == 0) {
        LOGE_GPU("Failed to choose EGL config");
        return false;
    }

    // Create context
    const EGLint contextAttrs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    m_tempContext = eglCreateContext(m_tempDisplay, config, EGL_NO_CONTEXT, contextAttrs);
    if (m_tempContext == EGL_NO_CONTEXT) {
        LOGE_GPU("Failed to create EGL context");
        return false;
    }

    // Create pbuffer surface (offscreen)
    const EGLint surfaceAttrs[] = {
        EGL_WIDTH, 1,
        EGL_HEIGHT, 1,
        EGL_NONE
    };

    m_tempSurface = eglCreatePbufferSurface(m_tempDisplay, config, surfaceAttrs);
    if (m_tempSurface == EGL_NO_SURFACE) {
        LOGE_GPU("Failed to create EGL surface");
        eglDestroyContext(m_tempDisplay, m_tempContext);
        m_tempContext = EGL_NO_CONTEXT;
        return false;
    }

    LOGI_GPU("✅ Temporary EGL context created successfully");
    return true;
}

void GPUCapabilitiesDetector::destroyTempEGLContext() {
    if (m_tempDisplay != EGL_NO_DISPLAY) {
        eglMakeCurrent(m_tempDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

        if (m_tempSurface != EGL_NO_SURFACE) {
            eglDestroySurface(m_tempDisplay, m_tempSurface);
            m_tempSurface = EGL_NO_SURFACE;
        }

        if (m_tempContext != EGL_NO_CONTEXT) {
            eglDestroyContext(m_tempDisplay, m_tempContext);
            m_tempContext = EGL_NO_CONTEXT;
        }

        eglTerminate(m_tempDisplay);
        m_tempDisplay = EGL_NO_DISPLAY;
    }

    LOGI_GPU("Temporary EGL context destroyed");
}