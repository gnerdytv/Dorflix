#ifndef GPU_CAPABILITIES_DETECTOR_H
#define GPU_CAPABILITIES_DETECTOR_H

#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <string>
#include <vector>

// Logging macros
#define LOG_TAG_GPU "GPUCapabilities"
#define LOGE_GPU(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_GPU, __VA_ARGS__)
#define LOGI_GPU(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_GPU, __VA_ARGS__)
#define LOGW_GPU(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG_GPU, __VA_ARGS__)

struct GPUCapabilities {
    // Basic info
    std::string gpuVendor;
    std::string gpuRenderer;
    std::string glVersion;
    std::string glslVersion;

    // Version support
    bool supportsOpenGLES1;
    bool supportsOpenGLES2;
    bool supportsOpenGLES3;
    int maxTextureSize;
    int maxViewportWidth;
    int maxViewportHeight;

    // Extension support
    bool supportsNPOTTextures;
    bool supportsETC1Compression;
    bool supportsETC2Compression;
    bool supportsASTCCompression;
    bool supportsS3TCCompression;
    bool supportsFramebuffers;
    bool supportsVAO;

    // Shader capabilities
    int maxVertexUniformVectors;
    int maxFragmentUniformVectors;
    int maxVaryingVectors;

    // Performance rating
    enum PerformanceClass {
        PERFORMANCE_UNKNOWN,
        PERFORMANCE_LOW_END,
        PERFORMANCE_MID_RANGE,
        PERFORMANCE_HIGH_END,
        PERFORMANCE_FLAGship
    };
    PerformanceClass performanceClass;

    // Recommendation
    enum RenderingStrategy {
        STRATEGY_SOFTWARE_ONLY,
        STRATEGY_OPENGLES1_BASIC,
        STRATEGY_OPENGLES2_BASIC,
        STRATEGY_OPENGLES2_OPTIMIZED,
        STRATEGY_OPENGLES3_FULL
    };
    RenderingStrategy recommendedStrategy;

    // Known issues
    bool hasKnownBugs;
    std::vector<std::string> knownIssues;
};

class GPUCapabilitiesDetector {
public:
    GPUCapabilitiesDetector();
    ~GPUCapabilitiesDetector();

    // Main detection method
    GPUCapabilities detectCapabilities();

    // Individual capability checks
    bool isOpenGLESAvailable();
    bool isOpenGLES1Supported();
    bool isOpenGLES2Supported();
    bool isOpenGLES3Supported();
    void identifyGPU(GPUCapabilities& caps);

private:
    // EGL detection (no context needed)
    bool detectEGLCapabilities();

    // OpenGL ES detection (requires context)
    bool detectOpenGLESCapabilities(GPUCapabilities& caps);

    // Extension checking
    bool checkExtension(const char* extension);
    bool hasExtension(const std::string& extensions, const std::string& ext);

    // GPU identification
    void identifyGPUVendor(GPUCapabilities& caps);
    void classifyPerformance(GPUCapabilities& caps);
    void determineStrategy(GPUCapabilities& caps);

    // Temporary EGL context for detection
    EGLDisplay m_tempDisplay;
    EGLContext m_tempContext;
    EGLSurface m_tempSurface;

    bool createTempEGLContext();
    void destroyTempEGLContext();
};

#endif // GPU_CAPABILITIES_DETECTOR_H