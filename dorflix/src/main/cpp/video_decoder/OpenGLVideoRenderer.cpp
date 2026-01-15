#include "OpenGLVideoRenderer.h"
#include <android/log.h>
#include <cstring>
#include <cmath>

// Logging macros
#define LOG_TAG "OpenGLVideoRenderer"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

OpenGLVideoRenderer::OpenGLVideoRenderer()
    : m_eglDisplay(EGL_NO_DISPLAY)
    , m_eglContext(EGL_NO_CONTEXT)
    , m_eglSurface(EGL_NO_SURFACE)
    , m_videoTexture(0)
    , m_shaderProgram(0)
    , m_vertexBuffer(0)
    , m_uvBuffer(0)
    , m_videoWidth(0)
    , m_videoHeight(0)
    , m_surfaceWidth(0)
    , m_surfaceHeight(0)
    , m_inputFormat(AV_PIX_FMT_NONE)
    , m_matrixValid(false)
    , m_swsContext(nullptr)
    , m_rgbFrame(nullptr)
    , m_initialized(false)
    , m_hasError(false)
    , m_openglVersion(0)
{
    LOGI("OpenGLVideoRenderer created");
}

OpenGLVideoRenderer::~OpenGLVideoRenderer() {
    cleanup();
    LOGI("OpenGLVideoRenderer destroyed");
}

bool OpenGLVideoRenderer::initialize(ANativeWindow* window, int width, int height) {
    LOGI("Initializing OpenGL ES video renderer: %dx%d", width, height);

    if (m_initialized) {
        LOGW("Renderer already initialized");
        return true;
    }

    m_surfaceWidth = width;
    m_surfaceHeight = height;
    m_hasError = false;

    // 🔍 DETECT GPU CAPABILITIES BEFORE INITIALIZATION
    LOGI("🔍 Detecting GPU capabilities...");
    GPUCapabilitiesDetector gpuDetector;
    GPUCapabilities caps = gpuDetector.detectCapabilities();

    // Log GPU information
    LOGI("📊 GPU Detection Results:");
    LOGI("   GPU: %s %s", caps.gpuVendor.c_str(), caps.gpuRenderer.c_str());
    LOGI("   OpenGL ES: %s", caps.supportsOpenGLES3 ? "3.0" : (caps.supportsOpenGLES2 ? "2.0" : "None"));
    LOGI("   Performance Class: %d/4", (int)caps.performanceClass);
    LOGI("   Recommended Strategy: %d", (int)caps.recommendedStrategy);

    if (caps.hasKnownBugs) {
        LOGW("⚠️ GPU has known issues:");
        for (const auto& issue : caps.knownIssues) {
            LOGW("   - %s", issue.c_str());
        }
    }

    // Check if GPU supports any OpenGL ES rendering
    if (caps.recommendedStrategy == GPUCapabilities::STRATEGY_SOFTWARE_ONLY) {
        LOGE("❌ GPU does not support OpenGL ES rendering - software fallback needed");
        LOGE("   This device has very limited GPU capabilities");
        m_hasError = true;
        return false;
    }

    // Setup EGL context with GPU-aware configuration
    if (!setupEGL(window)) {
        LOGE("❌ Failed to setup EGL context");

        // Try to provide helpful error information based on GPU detection
        if (caps.hasKnownBugs) {
            LOGE("   GPU has known compatibility issues - this may be expected");
        }

        LOGE("   Recommended GPU strategy was: %d", (int)caps.recommendedStrategy);
        LOGE("   Consider falling back to software rendering");

        m_hasError = true;
        return false;
    }

    // Create shaders appropriate for detected GPU capabilities
    if (!createShaders()) {
        LOGE("❌ Failed to create shaders");

        if (caps.recommendedStrategy == GPUCapabilities::STRATEGY_OPENGLES3_FULL) {
            LOGW("   GPU was expected to support ES 3.0 but shader creation failed");
            LOGW("   Consider downgrading to ES 2.0 shaders");
        }

        m_hasError = true;
        return false;
    }

    // Create vertex buffers
    if (!createBuffers()) {
        LOGE("❌ Failed to create buffers");
        m_hasError = true;
        return false;
    }

    // Create RGB frame for FFmpeg conversion
    m_rgbFrame = av_frame_alloc();
    if (!m_rgbFrame) {
        LOGE("❌ Failed to allocate RGB frame");
        m_hasError = true;
        return false;
    }

    // Store GPU capabilities for runtime decisions
    m_gpuCaps = caps;

    m_initialized = true;
    LOGI("✅ OpenGL ES video renderer initialized successfully");
    LOGI("   GPU Strategy: %d, Performance: %d/4", (int)caps.recommendedStrategy, (int)caps.performanceClass);
    return true;
}

void OpenGLVideoRenderer::cleanup() {
    LOGI("Cleaning up OpenGL ES renderer");

    if (m_rgbFrame) {
        av_frame_free(&m_rgbFrame);
        m_rgbFrame = nullptr;
    }

    if (m_swsContext) {
        sws_freeContext(m_swsContext);
        m_swsContext = nullptr;
    }

    if (m_eglDisplay != EGL_NO_DISPLAY) {
        eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

        if (m_vertexBuffer) {
            glDeleteBuffers(1, &m_vertexBuffer);
            m_vertexBuffer = 0;
        }

        if (m_uvBuffer) {
            glDeleteBuffers(1, &m_uvBuffer);
            m_uvBuffer = 0;
        }

        if (m_videoTexture) {
            glDeleteTextures(1, &m_videoTexture);
            m_videoTexture = 0;
        }

        if (m_shaderProgram) {
            glDeleteProgram(m_shaderProgram);
            m_shaderProgram = 0;
        }

        if (m_eglSurface != EGL_NO_SURFACE) {
            eglDestroySurface(m_eglDisplay, m_eglSurface);
            m_eglSurface = EGL_NO_SURFACE;
        }

        if (m_eglContext != EGL_NO_CONTEXT) {
            eglDestroyContext(m_eglDisplay, m_eglContext);
            m_eglContext = EGL_NO_CONTEXT;
        }

        eglTerminate(m_eglDisplay);
        m_eglDisplay = EGL_NO_DISPLAY;
    }

    m_initialized = false;
    m_hasError = false;
    LOGI("OpenGL ES renderer cleanup completed");
}

bool OpenGLVideoRenderer::setupEGL(ANativeWindow* window) {
    LOGI("Setting up EGL context with enhanced config selection");

    // Get default display
    m_eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (m_eglDisplay == EGL_NO_DISPLAY) {
        LOGE("eglGetDisplay failed");
        return false;
    }

    // Initialize EGL
    EGLint major, minor;
    if (!eglInitialize(m_eglDisplay, &major, &minor)) {
        LOGE("eglInitialize failed");
        return false;
    }
    LOGI("EGL initialized: version %d.%d", major, minor);

    // ENHANCED: More comprehensive config selection with depth/stencil support
    const EGLint configAttrs[] = {
        // Color buffer requirements
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,

        // Depth and stencil buffers for advanced rendering
        EGL_DEPTH_SIZE, 16,
        EGL_STENCIL_SIZE, 8,

        // Rendering requirements
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,

        // Performance hints
        EGL_SAMPLE_BUFFERS, 0,  // No multisampling for video (performance)
        EGL_SAMPLES, 1,

        EGL_NONE
    };

    // Get all available configs
    EGLConfig configs[32];
    EGLint numConfigs;
    if (!eglChooseConfig(m_eglDisplay, configAttrs, configs, 32, &numConfigs) || numConfigs == 0) {
        LOGW("Preferred config not available, trying fallback config");

        // Fallback: Minimal requirements
        const EGLint fallbackAttrs[] = {
            EGL_RED_SIZE, 5,
            EGL_GREEN_SIZE, 6,
            EGL_BLUE_SIZE, 5,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_NONE
        };

        if (!eglChooseConfig(m_eglDisplay, fallbackAttrs, configs, 32, &numConfigs) || numConfigs == 0) {
            LOGE("No suitable EGL config found");
            return false;
        }

        LOGI("Using fallback EGL config (lower quality)");
    }

    // ENHANCED: Select best config based on quality metrics
    EGLConfig bestConfig = configs[0];
    int bestScore = 0;

    for (int i = 0; i < numConfigs; i++) {
        EGLint red, green, blue, alpha, depth, stencil;

        eglGetConfigAttrib(m_eglDisplay, configs[i], EGL_RED_SIZE, &red);
        eglGetConfigAttrib(m_eglDisplay, configs[i], EGL_GREEN_SIZE, &green);
        eglGetConfigAttrib(m_eglDisplay, configs[i], EGL_BLUE_SIZE, &blue);
        eglGetConfigAttrib(m_eglDisplay, configs[i], EGL_ALPHA_SIZE, &alpha);
        eglGetConfigAttrib(m_eglDisplay, configs[i], EGL_DEPTH_SIZE, &depth);
        eglGetConfigAttrib(m_eglDisplay, configs[i], EGL_STENCIL_SIZE, &stencil);

        // Score based on color depth and buffer sizes
        int score = (red + green + blue + alpha) * 10 + depth + stencil;

        if (score > bestScore) {
            bestScore = score;
            bestConfig = configs[i];
        }

        LOGI("Config %d: RGBA=%d,%d,%d,%d Depth=%d Stencil=%d Score=%d",
             i, red, green, blue, alpha, depth, stencil, score);
    }

    m_eglConfig = bestConfig;
    LOGI("Selected best EGL config with score: %d", bestScore);

    // Create context with basic attributes (KHR extensions not available in NDK)
    const EGLint contextAttrs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
    };

    m_eglContext = eglCreateContext(m_eglDisplay, m_eglConfig, EGL_NO_CONTEXT, contextAttrs);
    if (m_eglContext == EGL_NO_CONTEXT) {
        LOGW("OpenGL ES 3.0 context failed, trying ES 2.0");

        // Fallback to OpenGL ES 2.0
        const EGLint fallbackContextAttrs[] = {
            EGL_CONTEXT_CLIENT_VERSION, 2,
            EGL_NONE
        };

        m_eglContext = eglCreateContext(m_eglDisplay, m_eglConfig, EGL_NO_CONTEXT, fallbackContextAttrs);
        if (m_eglContext == EGL_NO_CONTEXT) {
            LOGE("eglCreateContext failed for both ES 3.0 and 2.0");
            return false;
        }

        LOGI("Using OpenGL ES 2.0 context (fallback)");
    } else {
        LOGI("✅ Created OpenGL ES 3.0 context");
    }

    // Create surface with enhanced attributes
    const EGLint surfaceAttrs[] = {
        EGL_RENDER_BUFFER, EGL_BACK_BUFFER,
        EGL_NONE
    };

    m_eglSurface = eglCreateWindowSurface(m_eglDisplay, m_eglConfig, window, surfaceAttrs);
    if (m_eglSurface == EGL_NO_SURFACE) {
        LOGE("eglCreateWindowSurface failed");
        return false;
    }

    // Make current
    if (!eglMakeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface, m_eglContext)) {
        LOGE("eglMakeCurrent failed");
        return false;
    }

    // Log final configuration
    EGLint surfaceWidth, surfaceHeight;
    eglQuerySurface(m_eglDisplay, m_eglSurface, EGL_WIDTH, &surfaceWidth);
    eglQuerySurface(m_eglDisplay, m_eglSurface, EGL_HEIGHT, &surfaceHeight);

    LOGI("✅ EGL setup completed: Surface %dx%d, Context ready", surfaceWidth, surfaceHeight);
    return true;
}

bool OpenGLVideoRenderer::createShaders() {
    LOGI("Creating OpenGL ES shaders");

    const char* vertexSource = getVertexShader();
    const char* fragmentSource = getFragmentShader();

    // Create vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, nullptr);
    glCompileShader(vertexShader);

    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        LOGE("Vertex shader compilation failed: %s", infoLog);
        return false;
    }

    // Create fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, nullptr);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        LOGE("Fragment shader compilation failed: %s", infoLog);
        return false;
    }

    // Create program
    m_shaderProgram = glCreateProgram();
    glAttachShader(m_shaderProgram, vertexShader);
    glAttachShader(m_shaderProgram, fragmentShader);
    glLinkProgram(m_shaderProgram);

    glGetProgramiv(m_shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetProgramInfoLog(m_shaderProgram, 512, nullptr, infoLog);
        LOGE("Shader program linking failed: %s", infoLog);
        return false;
    }

    // Clean up shaders
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Get attribute locations
    m_positionLocation = glGetAttribLocation(m_shaderProgram, "aPosition");
    m_texCoordLocation = glGetAttribLocation(m_shaderProgram, "aTexCoord");
    m_textureLocation = glGetUniformLocation(m_shaderProgram, "uTexture");
    m_matrixLocation = glGetUniformLocation(m_shaderProgram, "uMatrix");

    LOGI("✅ Shaders created successfully");
    return true;
}

bool OpenGLVideoRenderer::createBuffers() {
    LOGI("Creating vertex buffers");

    // Vertex positions (quad covering screen)
    const GLfloat vertices[] = {
        -1.0f, -1.0f,  // bottom left
         1.0f, -1.0f,  // bottom right
        -1.0f,  1.0f,  // top left
         1.0f,  1.0f   // top right
    };

    // Texture coordinates
    const GLfloat texCoords[] = {
        0.0f, 1.0f,  // bottom left
        1.0f, 1.0f,  // bottom right
        0.0f, 0.0f,  // top left
        1.0f, 0.0f   // top right
    };

    // Create vertex buffer
    glGenBuffers(1, &m_vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Create UV buffer
    glGenBuffers(1, &m_uvBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_uvBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(texCoords), texCoords, GL_STATIC_DRAW);

    // Create texture
    glGenTextures(1, &m_videoTexture);
    glBindTexture(GL_TEXTURE_2D, m_videoTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    LOGI("✅ Buffers created successfully");
    return true;
}

bool OpenGLVideoRenderer::renderFrame(AVFrame* frame) {
    if (!m_initialized || !frame) {
        LOGE("Render failed: not initialized or null frame");
        return false;
    }

    if (m_hasError) {
        LOGW("Render skipped: renderer in error state");
        return false;
    }

    // CRITICAL: Validate frame data
    if (!frame->data[0] || frame->width <= 0 || frame->height <= 0) {
        LOGE("Render failed: invalid frame data");
        return false;
    }

    // Convert frame to RGB with DYNAMIC pixel format detection
    AVPixelFormat inputFormat = static_cast<AVPixelFormat>(frame->format);
    updateScalingContext(frame->width, frame->height, inputFormat);

    if (m_hasError || !m_swsContext || !m_rgbFrame) {
        LOGE("Render failed: scaling context error");
        return false;
    }

    // Convert YUV to RGB with error checking
    int scaleResult = sws_scale(m_swsContext, frame->data, frame->linesize, 0, frame->height,
                                m_rgbFrame->data, m_rgbFrame->linesize);

    if (scaleResult < 0) {
        LOGE("YUV→RGB conversion failed: %d", scaleResult);
        m_hasError = true;
        return false;
    }

    // OPTIMIZED: Use glTexSubImage2D for updates instead of glTexImage2D
    glBindTexture(GL_TEXTURE_2D, m_videoTexture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_rgbFrame->width, m_rgbFrame->height,
                    GL_RGBA, GL_UNSIGNED_BYTE, m_rgbFrame->data[0]);

    checkGLError("glTexSubImage2D");
    if (m_hasError) {
        return false;
    }

    // Render quad with aspect ratio correction
    renderQuad();

    // Swap buffers with ERROR RECOVERY
    if (!eglSwapBuffers(m_eglDisplay, m_eglSurface)) {
        LOGE("eglSwapBuffers failed - attempting recovery");

        // Try to recover from context loss
        if (!eglMakeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface, m_eglContext)) {
            LOGE("Failed to recover EGL context");
            m_hasError = true;
            return false;
        }

        // Try swap again
        if (!eglSwapBuffers(m_eglDisplay, m_eglSurface)) {
            LOGE("Recovery swap also failed");
            m_hasError = true;
            return false;
        }

        LOGI("✅ Successfully recovered from EGL error");
    }

    checkGLError("renderFrame");
    return true;
}

void OpenGLVideoRenderer::setViewport(int width, int height) {
    if (width != m_surfaceWidth || height != m_surfaceHeight) {
        m_surfaceWidth = width;
        m_surfaceHeight = height;

        // Invalidate cached matrix when surface dimensions change
        m_matrixValid = false;

        glViewport(0, 0, width, height);
        LOGI("Viewport updated to %dx%d, matrix cache invalidated", width, height);
    }
}

void OpenGLVideoRenderer::updateScalingContext(int frameWidth, int frameHeight, AVPixelFormat inputFormat) {
    if (frameWidth != m_videoWidth || frameHeight != m_videoHeight || inputFormat != m_inputFormat) {
        m_videoWidth = frameWidth;
        m_videoHeight = frameHeight;
        m_inputFormat = inputFormat;

        // Free old context
        if (m_swsContext) {
            sws_freeContext(m_swsContext);
            m_swsContext = nullptr;
        }

        // Allocate RGB frame
        if (m_rgbFrame) {
            av_frame_free(&m_rgbFrame);
        }
        m_rgbFrame = av_frame_alloc();

        if (!m_rgbFrame) {
            LOGE("Failed to allocate RGB frame");
            m_hasError = true;
            return;
        }

        // Set up frame properties
        m_rgbFrame->format = AV_PIX_FMT_RGBA;
        m_rgbFrame->width = frameWidth;
        m_rgbFrame->height = frameHeight;

        int ret = av_frame_get_buffer(m_rgbFrame, 0);
        if (ret < 0) {
            LOGE("Failed to allocate RGB frame buffer: %d", ret);
            av_frame_free(&m_rgbFrame);
            m_rgbFrame = nullptr;
            m_hasError = true;
            return;
        }

        // Create scaling context with DYNAMIC input format
        m_swsContext = sws_getContext(frameWidth, frameHeight, inputFormat,
                                     frameWidth, frameHeight, AV_PIX_FMT_RGBA,
                                     SWS_BILINEAR, nullptr, nullptr, nullptr);

        if (!m_swsContext) {
            LOGE("Failed to create scaling context for format %d", inputFormat);
            m_hasError = true;
            return;
        }

        // Update texture size if needed
        if (m_videoTexture) {
            glBindTexture(GL_TEXTURE_2D, m_videoTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, frameWidth, frameHeight, 0,
                         GL_RGBA, GL_UNSIGNED_BYTE, nullptr); // Allocate without data
            checkGLError("glTexImage2D allocation");
        }

        LOGI("✅ Updated scaling context: %dx%d format=%d → RGBA",
             frameWidth, frameHeight, inputFormat);
    }
}

void OpenGLVideoRenderer::renderQuad() {
    glUseProgram(m_shaderProgram);

    // OPTIMIZED: Cache transformation matrix - only recalculate when dimensions change
    if (!m_matrixValid) {
        // Calculate ASPECT RATIO CORRECTION matrix
        float videoAspect = static_cast<float>(m_videoWidth) / m_videoHeight;
        float surfaceAspect = static_cast<float>(m_surfaceWidth) / m_surfaceHeight;

        float scaleX = 1.0f;
        float scaleY = 1.0f;

        if (videoAspect > surfaceAspect) {
            // Video is wider - fit to width, add letterbox top/bottom
            scaleY = surfaceAspect / videoAspect;
        } else {
            // Video is taller - fit to height, add letterbox sides
            scaleX = videoAspect / surfaceAspect;
        }

        // Create transformation matrix with aspect ratio correction
        m_cachedMatrix[0] = scaleX;    m_cachedMatrix[1] = 0.0f;     m_cachedMatrix[2] = 0.0f;     m_cachedMatrix[3] = 0.0f;
        m_cachedMatrix[4] = 0.0f;      m_cachedMatrix[5] = scaleY;   m_cachedMatrix[6] = 0.0f;     m_cachedMatrix[7] = 0.0f;
        m_cachedMatrix[8] = 0.0f;      m_cachedMatrix[9] = 0.0f;     m_cachedMatrix[10] = 1.0f;    m_cachedMatrix[11] = 0.0f;
        m_cachedMatrix[12] = 0.0f;     m_cachedMatrix[13] = 0.0f;    m_cachedMatrix[14] = 0.0f;    m_cachedMatrix[15] = 1.0f;

        m_matrixValid = true;
        LOGI("✅ Matrix cached for video %dx%d on surface %dx%d",
             m_videoWidth, m_videoHeight, m_surfaceWidth, m_surfaceHeight);
    }

    // Use cached matrix for performance
    glUniformMatrix4fv(m_matrixLocation, 1, GL_FALSE, m_cachedMatrix);
    glUniform1i(m_textureLocation, 0);

    // Bind texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_videoTexture);

    // Bind vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
    glEnableVertexAttribArray(m_positionLocation);
    glVertexAttribPointer(m_positionLocation, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    // Bind UV buffer
    glBindBuffer(GL_ARRAY_BUFFER, m_uvBuffer);
    glEnableVertexAttribArray(m_texCoordLocation);
    glVertexAttribPointer(m_texCoordLocation, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    // Draw with cached aspect ratio correction
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // Cleanup
    glDisableVertexAttribArray(m_positionLocation);
    glDisableVertexAttribArray(m_texCoordLocation);
}

const char* OpenGLVideoRenderer::getVertexShader() {
    return R"glsl(
        #version 300 es
        precision mediump float;

        layout(location = 0) in vec2 aPosition;
        layout(location = 1) in vec2 aTexCoord;

        uniform mat4 uMatrix;

        out vec2 vTexCoord;

        void main() {
            gl_Position = uMatrix * vec4(aPosition, 0.0, 1.0);
            vTexCoord = aTexCoord;
        }
    )glsl";
}

const char* OpenGLVideoRenderer::getFragmentShader() {
    return R"glsl(
        #version 300 es
        precision mediump float;

        uniform sampler2D uTexture;

        in vec2 vTexCoord;

        out vec4 fragColor;

        void main() {
            fragColor = texture(uTexture, vTexCoord);
        }
    )glsl";
}

void OpenGLVideoRenderer::checkGLError(const char* operation) {
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        LOGE("OpenGL error after %s: 0x%x", operation, error);
        m_hasError = true;
    }
}

void OpenGLVideoRenderer::logError(const char* message) {
    LOGE("OpenGLVideoRenderer Error: %s", message);
    m_hasError = true;
}