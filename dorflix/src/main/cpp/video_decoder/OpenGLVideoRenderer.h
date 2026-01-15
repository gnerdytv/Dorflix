#ifndef OPENGL_VIDEO_RENDERER_H
#define OPENGL_VIDEO_RENDERER_H

#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <android/native_window.h>
#include <string>

// Local headers
#include "VideoRenderer.h"
#include "GPUCapabilitiesDetector.h"

// FFmpeg headers
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>
#include <libswscale/swscale.h>
}

class OpenGLVideoRenderer : public VideoRenderer {
public:
    OpenGLVideoRenderer();
    ~OpenGLVideoRenderer();

    // Initialization
    bool initialize(ANativeWindow* window, int width, int height);
    void cleanup();

    // Rendering
    bool renderFrame(AVFrame* frame);
    void setViewport(int width, int height);

    // State queries
    bool isInitialized() const { return m_initialized; }
    bool hasError() const { return m_hasError; }

private:
    // EGL context
    EGLDisplay m_eglDisplay;
    EGLContext m_eglContext;
    EGLSurface m_eglSurface;
    EGLConfig m_eglConfig;

    // OpenGL resources
    GLuint m_videoTexture;
    GLuint m_shaderProgram;
    GLuint m_vertexBuffer;
    GLuint m_uvBuffer;

    // Shader locations
    GLint m_positionLocation;
    GLint m_texCoordLocation;
    GLint m_textureLocation;
    GLint m_matrixLocation;

    // Video properties
    int m_videoWidth;
    int m_videoHeight;
    int m_surfaceWidth;
    int m_surfaceHeight;
    AVPixelFormat m_inputFormat;

    // Cached transformation matrix for performance
    float m_cachedMatrix[16];
    bool m_matrixValid;

    // GPU capabilities for runtime decisions
    GPUCapabilities m_gpuCaps;

    // FFmpeg scaling context
    SwsContext* m_swsContext;
    AVFrame* m_rgbFrame;

    // State
    bool m_initialized;
    bool m_hasError;

    // OpenGL ES version being used
    int m_openglVersion;  // 1 for ES 1.1, 2 for ES 2.0, 3 for ES 3.0

    // Private methods
    bool setupEGL(ANativeWindow* window);
    bool createShaders();
    bool createBuffers();
    void updateScalingContext(int frameWidth, int frameHeight, AVPixelFormat inputFormat);
    void renderQuad();

    // Shader sources
    const char* getVertexShader();
    const char* getFragmentShader();

    // Utility
    void checkGLError(const char* operation);
    void logError(const char* message);
};

#endif // OPENGL_VIDEO_RENDERER_H