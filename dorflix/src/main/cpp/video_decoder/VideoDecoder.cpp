#include "VideoDecoder.h"
#include "AudioDecoder.h"
#include "VideoCacheManager.h"
#include "VideoDownloaderJNI.h"
#include "ExportedFunctions.h"
#include "FrameBufferManager.h"
#include "VideoRendererFactory.h"
#include "VideoDecoderFactory.h"
#include "VideoDecoderBase.h"
#include "MediaCodecHardwareDecoder.h"
#include "AddressSpaceDetector.h"
#include "MemoryConstrainedBufferManager.h"
#include "../av_sync/AVSyncController.h"
#include "../av_sync/ExceptionSafety.h"
#include "../video_preloader/Preloader.h"
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <thread>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <cstring>
#include <algorithm>
#include <android/log.h>
#include <queue>
#include <mutex>
#include <cinttypes>

// Define EAGAIN for Android NDK compatibility if not already defined
#ifndef EAGAIN
#define EAGAIN 11
#endif

// Logging macros
#define LOG_TAG "VideoDecoder"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

VideoDecoder::VideoDecoder() {
    LOGI("=== VIDEODECODER CONSTRUCTOR CALLED ===");

    // ===== 32-BIT ADDRESS SPACE DETECTION & MEMORY CONTAINMENT =====
    // Detect address space and apply memory constraints to prevent 0x100000000 crashes
    LOGI("Detecting address space for memory containment...");
    MemoryConstrainedBufferManager memoryManager;
    memoryManager.logMemoryConstraints();

    // Apply FFmpeg memory limits based on address space
    LOGI("Setting FFmpeg memory limits for address space safety...");
    size_t maxAllocMB = AddressSpaceDetector::getMaxSafeAllocationMB();
    av_max_alloc(maxAllocMB * 1024 * 1024); // Convert MB to bytes
    LOGI("FFmpeg memory limit set to %zu MB for %s-bit safety",
         maxAllocMB, AddressSpaceDetector::is32Bit() ? "32" : "64");

    // Initialize VideoCacheManager
    LOGI("Initializing VideoCacheManager...");
    m_cacheManager = new VideoCacheManager();
    LOGI("VideoCacheManager initialized");

    // Initialize Preloader for background video caching
    LOGI("Initializing Preloader...");
    m_preloader = new Preloader();
    LOGI("Preloader initialized");

    // Initialize AudioDecoder
    LOGI("Initializing AudioDecoder...");
    m_audioDecoder = new AudioDecoder();
    LOGI("AudioDecoder initialized");

    // Initialize A/V Sync Controller
    LOGI("Initializing AVSyncController...");
    m_syncController = new AVSyncController();

    // Set up sync controller callbacks
    m_syncController->setVideoRenderCallback([this](AVFrame* frame) {
        this->renderFrame(frame);
    });

    m_syncController->setAudioPlayCallback([this](AVPacket* packet) {
        if (m_audioDecoder) {
            m_audioDecoder->decodeAudioPacket(packet);
        }
    });

    m_syncController->startSync();
    LOGI("AVSyncController initialized and started");

    // Renderer will be selected hierarchically in setSurface()
    LOGI("Renderer will be selected hierarchically based on GPU capabilities");

    // FFmpeg 8+ auto-registration - no need for av_register_all()
    // Initialize network support with error checking
    LOGI("Initializing FFmpeg network support...");
    int result = avformat_network_init();
    if (result < 0) {
        LOGW("FFmpeg network initialization failed (error: %d), continuing anyway", result);
        // Log error but continue - network may not be available
        // This is not critical for local file playback
    } else {
        LOGI("FFmpeg network support initialized");
    }

    LOGI("VideoDecoder constructor completed successfully");
}

VideoDecoder::~VideoDecoder() {
    cleanup();

    // Clean up AudioDecoder
    if (m_audioDecoder) {
        delete m_audioDecoder;
        m_audioDecoder = nullptr;
    }

    // Clean up AV Sync Controller
    if (m_syncController) {
        m_syncController->stopSync();
        delete m_syncController;
        m_syncController = nullptr;
    }

    // Clean up hierarchical renderer (if any)
    if (m_renderer) {
        m_renderer->cleanup();
        m_renderer.reset();
    }
    m_rendererTypeSelected = false;
    m_rendererType = RendererType::GPU_OPENGL; // Reset to default

    // Clean up Preloader
    if (m_preloader) {
        m_preloader->stopPreloading();
        delete m_preloader;
        m_preloader = nullptr;
        LOGI("Destroyed preloader");
    }
}

bool VideoDecoder::loadVideo(const std::string& videoPath) {
    LOGI("=== LOADING NEW VIDEO ===");
    LOGI("Video path: %s", videoPath.c_str());

    // For TikTok-style app: Only cleanup if we already have a video loaded
    // Don't cleanup on first load - videos should persist until explicitly cleaned up
    if (m_formatContext != nullptr) {
        LOGI("Previous video loaded, cleaning up before loading new video");
        cleanup();
    } else {
        LOGI("No previous video loaded, proceeding with fresh load");
    }
    
    // Open video file with FFmpeg 8+
    LOGI("Calling openVideoFile()");
    if (!openVideoFile(videoPath)) {
        LOGE("openVideoFile() failed");
        notifyError(-1, "Failed to open video file");
        return false;
    }
    LOGI("openVideoFile() succeeded");

    // Find video stream
    LOGI("Calling findVideoStream()");
    if (!findVideoStream()) {
        LOGE("findVideoStream() failed");
        notifyError(-2, "No video stream found");
        return false;
    }
    LOGI("findVideoStream() succeeded");

    // Initialize video codec
    LOGI("Calling initializeVideoCodec()");
    if (!initializeVideoCodec()) {
        LOGE("initializeVideoCodec() failed");
        notifyError(-3, "Failed to initialize video codec");
        return false;
    }
    LOGI("initializeVideoCodec() succeeded");

    // Initialize audio decoder (optional)
    LOGI("Calling initializeAudioCodec()");
    if (m_audioDecoder && m_audioDecoder->initializeAudioCodec(m_formatContext)) {
        LOGI("AudioDecoder initialized successfully - audio will be available");
    } else {
        LOGW("AudioDecoder initialization failed - videos will play without audio");
    }
    LOGI("initializeAudioCodec() completed");

    // Set up scaling context for rendering
    // Get pixel format from codec parameters (hierarchical decoder system)
    AVCodecParameters* codecParams = m_formatContext->streams[m_videoStreamIndex]->codecpar;
    AVPixelFormat sourcePixFmt = (AVPixelFormat)codecParams->format;
    LOGI("Using pixel format from codec parameters: %d", sourcePixFmt);

    // If codec parameters don't have the format (common for H.264),
    // use YUV420P which is the most common pixel format for H.264 videos
    if (sourcePixFmt == AV_PIX_FMT_NONE || sourcePixFmt == -1) {
        sourcePixFmt = AV_PIX_FMT_YUV420P;
        LOGI("Using default YUV420P pixel format for H.264 video");
    }

    // For pixel-perfect rendering, prefer RGB565 for better performance on mobile
    // Fallback to RGBA_8888 for compatibility
    AVPixelFormat targetPixFmt = AV_PIX_FMT_RGB565;
    if (sourcePixFmt == AV_PIX_FMT_RGBA || sourcePixFmt == AV_PIX_FMT_BGRA ||
        sourcePixFmt == AV_PIX_FMT_ARGB || sourcePixFmt == AV_PIX_FMT_ABGR) {
        targetPixFmt = AV_PIX_FMT_RGBA;
        LOGI("Using RGBA_8888 target format for alpha channel support");
    }
    LOGI("Target pixel format for rendering: %s", targetPixFmt == AV_PIX_FMT_RGB565 ? "RGB565" : "RGBA_8888");

    // Video loaded - scaling context will be created dynamically in renderFrame()
    // based on actual device screen dimensions and aspect ratio
    LOGI("Video loaded at native dimensions: %dx%d (scaling context created in renderFrame)", m_videoWidth, m_videoHeight);

    // Don't create scaling context here - wait for renderFrame() to get device dimensions
    // Scaling context is intentionally set to nullptr and created dynamically later
    m_swsContext = nullptr;
    LOGI("Scaling context will be created dynamically in renderFrame()");

    // Allocate frame for decoding
    LOGI("Allocating frame for decoding");
    m_frame = av_frame_alloc();
    if (!m_frame) {
        LOGE("Failed to allocate frame");
        notifyError(-5, "Failed to allocate frame");
        return false;
    }
    LOGI("Frame allocated successfully");

    LOGI("Setting m_isPrepared = true");
    m_isPrepared = true;

    LOGI("=== VIDEO LOADING COMPLETED SUCCESSFULLY ===");
    LOGI("Video is now fully prepared and ready for playback (thread will start in play())");

    return true;
}

void VideoDecoder::play() {
    if (!m_isPrepared || m_isPlaying) {
        return;
    }

    // Safety check: ensure decoder is alive before starting threads
    if (!m_isAlive.load()) {
        LOGW("VideoDecoder m_isAlive is false, reviving decoder for playback");
        m_isAlive = true;
        LOGI("VideoDecoder revived, m_isAlive = true");
    }

    m_isPlaying = true;
    m_isDecoding = true;
    
    if (m_onVideoStarted) {
        m_onVideoStarted();
    }
    
    // Start decoding thread with proper error handling
    try {
        if (m_decodeThread.joinable()) {
            m_decodeThread.join();
        }
        m_decodeThread = std::thread(&VideoDecoder::decodeLoop, this);
    } catch (const std::exception& e) {
        LOGE("Failed to start decode thread: %s", e.what());
        m_isPlaying = false;
        m_isDecoding = false;
        notifyError(-10, "Failed to start decode thread");
    }
}

void VideoDecoder::pause() {
    if (!m_isPlaying) return;
    
    m_isPlaying = false;
    
    if (m_onVideoPaused) {
        m_onVideoPaused();
    }
}

void VideoDecoder::stop() {
    LOGI("=== STOPPING VIDEO DECODER ===");
    LOGI("Setting flags: isPlaying=false, isDecoding=false, isSeeking=false");

    m_isPlaying = false;
    m_isDecoding = false;
    m_isSeeking = false;

    if (m_decodeThread.joinable()) {
        LOGI("Waiting for decode thread to join...");
        m_decodeThread.join();
        LOGI("Decode thread successfully joined");
    } else {
        LOGI("Decode thread not joinable (already stopped or never started)");
    }

    LOGI("Calling onVideoStopped callback");
    if (m_onVideoStopped) {
        m_onVideoStopped();
    }

    LOGI("=== VIDEO DECODER STOPPED ===");
}

void VideoDecoder::seekTo(int position) {
    if (!m_isPrepared) return;
    
    m_isSeeking = true;
    m_currentPosition = position;
    m_isSeeking = false;
    
    if (m_onVideoSeekComplete) {
        m_onVideoSeekComplete();
    }
}

bool VideoDecoder::isPlaying() const { 
    return m_isPlaying; 
}

int VideoDecoder::getCurrentPosition() const { 
    return m_currentPosition; 
}

int VideoDecoder::getDuration() const { 
    return m_duration; 
}

bool VideoDecoder::isPrepared() const { 
    return m_isPrepared; 
}

void VideoDecoder::setSurface(void* surface) {
    LOGI("Setting surface: %p", surface);
    m_surface = surface;

    if (!surface) {
        LOGW("Surface is null");
        return;
    }

    // Select renderer hierarchically based on GPU capabilities (only once)
    if (!m_rendererTypeSelected) {
        m_rendererType = VideoRendererFactory::selectBestRenderer();
        m_renderer = VideoRendererFactory::createRenderer(m_rendererType);
        m_rendererTypeSelected = true;

        const char* rendererName = "Unknown";
        switch (m_rendererType) {
            case RendererType::GPU_OPENGL: rendererName = "OpenGL ES"; break;
            case RendererType::CPU_ANATIVE: rendererName = "ANativeWindow"; break;
            case RendererType::GPU_VULKAN: rendererName = "Vulkan"; break;
            case RendererType::CPU_CANVAS: rendererName = "Canvas"; break;
        }
        LOGI("Selected renderer: %s", rendererName);
    }

    // Initialize the selected renderer
    if (m_renderer) {
        ANativeWindow* window = static_cast<ANativeWindow*>(surface);
        int width = ANativeWindow_getWidth(window);
        int height = ANativeWindow_getHeight(window);

        // Cap surface dimensions to 720p (1280x720) for performance
        // Modern phones can have 1440x2560+ resolutions which use excessive memory
        const int MAX_WIDTH = 1280;
        const int MAX_HEIGHT = 720;

        if (width > MAX_WIDTH) {
            height = (height * MAX_WIDTH) / width;
            width = MAX_WIDTH;
            LOGI("Capping surface width to %d (was higher)", MAX_WIDTH);
        }
        if (height > MAX_HEIGHT) {
            width = (width * MAX_HEIGHT) / height;
            height = MAX_HEIGHT;
            LOGI("Capping surface height to %d (was higher)", MAX_HEIGHT);
        }

        if (width > 0 && height > 0) {
            LOGI("Initializing renderer with surface %dx%d", width, height);
            if (!m_renderer->initialize(window, width, height)) {
                LOGE("Failed to initialize selected renderer");
            } else {
                LOGI("Renderer initialized successfully");
            }
        } else {
            LOGW("Surface has invalid dimensions: %dx%d", width, height);
        }
    } else {
        LOGW("No renderer available");
    }

    // ===== PHASE 4: ADD MEDIACODEC FALLBACK MECHANISM =====
    // Try MediaCodec first, fall back to FFmpeg if it fails
    if (m_videoDecoder) {
        // Check if the decoder is MediaCodec-based
        auto* mediaCodecDecoder = dynamic_cast<MediaCodecHardwareDecoder*>(m_videoDecoder.get());
        if (mediaCodecDecoder) {
            LOGI("🎯 PHASE 4: Attempting MediaCodec hardware decoding with fallback");

            // Set surface on MediaCodec decoder
            LOGI("Setting output surface on MediaCodec decoder for SURFACE MODE");
            ANativeWindow* window = static_cast<ANativeWindow*>(surface);
            mediaCodecDecoder->setOutputSurface(window);
            LOGI("✅ Surface set on MediaCodec decoder");

            // Try to initialize MediaCodec decoder
            LOGI("🔄 Initializing MediaCodec decoder now that surface is available");
            if (!mediaCodecDecoder->initialize(m_formatContext, m_videoStreamIndex)) {
                LOGW("⚠️ PHASE 4: MediaCodec hardware decoding failed - attempting fallback to FFmpeg");

                // ===== FALLBACK: Create FFmpeg-based decoder =====
                LOGI("🎯 PHASE 4: Creating FFmpeg fallback decoder");

                // Clean up the failed MediaCodec decoder
                m_videoDecoder.reset();

                // Try to create an FFmpeg decoder instead
                DecoderType fallbackType = DecoderType::FFMPEG_SOFTWARE;
                m_videoDecoder = VideoDecoderFactory::createDecoder(fallbackType);

                if (!m_videoDecoder) {
                    LOGE("❌ PHASE 4: FFmpeg fallback decoder creation failed - no decoders available");

                    // ===== COMPLETE SHUTDOWN - NO DECODERS AVAILABLE =====
                    shutdownOnCriticalError(-101, "Both MediaCodec and FFmpeg decoders failed - no video decoding available");
                    return;
                }

                LOGI("✅ PHASE 4: FFmpeg fallback decoder created successfully");

                // Initialize the FFmpeg decoder immediately (no surface dependency)
                if (!m_videoDecoder->initialize(m_formatContext, m_videoStreamIndex)) {
                    LOGE("❌ PHASE 4: FFmpeg fallback decoder initialization failed");

                    // ===== COMPLETE SHUTDOWN - FFmpeg ALSO FAILED =====
                    shutdownOnCriticalError(-102, "FFmpeg fallback decoder initialization failed");
                    return;
                }

                LOGI("✅ PHASE 4: FFmpeg fallback decoder initialized successfully - using software decoding");
                LOGI("ℹ️  PHASE 4: Performance may be reduced compared to hardware decoding");

            } else {
                LOGI("✅ PHASE 4: MediaCodec hardware decoding initialized successfully");

                // ===== CRITICAL: Set frame delivery callback for asynchronous decoding =====
                // Hardware decoder returns nullptr immediately and delivers frames via callback
                LOGI("Setting frame delivery callback on MediaCodec decoder");
                mediaCodecDecoder->setFrameDeliveryCallback([this](AVFrame* frame) {
                    if (frame) {
                        LOGD("Received frame from hardware decoder: pts=%" PRId64, frame->pts);

                        // Deliver frame to sync controller for synchronized rendering
                        if (m_syncController) {
                            try {
                                m_syncController->processVideoFrame(frame);
                            } catch (const std::exception& e) {
                                LOGE("Failed to process video frame through sync controller: %s", e.what());
                            } catch (...) {
                                LOGE("Unknown error processing video frame through sync controller");
                            }
                        } else {
                            LOGW("Sync controller is null, dropping frame from hardware decoder");
                        }
                    } else {
                        LOGW("Received null frame from hardware decoder");
                    }
                });
                LOGI("✅ Frame delivery callback set on MediaCodec decoder");
            }
        } else {
            LOGI("🎯 PHASE 4: Using non-MediaCodec decoder (already initialized) - no fallback needed");
        }
    } else {
        LOGW("⚠️ PHASE 4: No video decoder available to set surface on");
    }

    LOGI("✅ PHASE 4: Fallback mechanism completed - decoder ready for playback");
}

void VideoDecoder::setVolume(float volume) {
    if (m_audioDecoder) {
        m_audioDecoder->setVolume(volume);
    }
    m_volume = volume;
}

void VideoDecoder::setMute(bool mute) {
    if (m_audioDecoder) {
        m_audioDecoder->setMute(mute);
    }
    m_isMuted = mute;
}

void VideoDecoder::setVideoPreparedCallback(VideoPreparedCallback callback) { 
    m_onVideoPrepared = callback; 
}

void VideoDecoder::setVideoStartedCallback(VideoStartedCallback callback) { 
    m_onVideoStarted = callback; 
}

void VideoDecoder::setVideoPausedCallback(VideoPausedCallback callback) { 
    m_onVideoPaused = callback; 
}

void VideoDecoder::setVideoStoppedCallback(VideoStoppedCallback callback) { 
    m_onVideoStopped = callback; 
}

void VideoDecoder::setVideoCompletedCallback(VideoCompletedCallback callback) { 
    m_onVideoCompleted = callback; 
}

void VideoDecoder::setVideoErrorCallback(VideoErrorCallback callback) { 
    m_onVideoError = callback; 
}

void VideoDecoder::setVideoProgressCallback(VideoProgressCallback callback) { 
    m_onVideoProgress = callback; 
}

void VideoDecoder::setVideoBufferingStartedCallback(VideoBufferingCallback callback) { 
    m_onVideoBufferingStarted = callback; 
}

void VideoDecoder::setVideoBufferingEndedCallback(VideoBufferingCallback callback) { 
    m_onVideoBufferingEnded = callback; 
}

void VideoDecoder::setVideoSeekCompleteCallback(VideoSeekCallback callback) { 
    m_onVideoSeekComplete = callback; 
}

void VideoDecoder::setVideoSizeChangedCallback(VideoSizeCallback callback) { 
    m_onVideoSizeChanged = callback; 
}

bool VideoDecoder::openVideoFile(const std::string& videoPath) {
    LOGI("=== OPENING VIDEO FILE ===");
    LOGI("File path: %s", videoPath.c_str());

    // Validate input path
    if (videoPath.empty()) {
        LOGE("Empty video path provided");
        return false;
    }

    // Check if this is a network URL (starts with http/https)
    bool isHttpsUrl = (videoPath.find("https://") == 0);
    bool isNetworkUrl = (videoPath.find("http://") == 0 ||
                        videoPath.find("https://") == 0 ||
                        videoPath.find("file://") == 0);

    // For HTTPS URLs, check cache first, then download if needed
    std::string finalPath = videoPath;
    if (isHttpsUrl) {
        LOGI("HTTPS URL detected - checking cache first");

        // Check if video is cached using JNI call
        JNIEnv* env = nullptr;
        JavaVM* jvm = nullptr;

        // Get JVM from exported function
        jvm = getGlobalJVM();

        if (!jvm) {
            LOGE("JVM not available for cache check");
            return false;
        }

        // Attach current thread to JVM
        int attachResult = jvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
        bool attached = false;
        if (attachResult == JNI_EDETACHED) {
            if (jvm->AttachCurrentThread(&env, nullptr) == JNI_OK) {
                attached = true;
            } else {
                LOGE("Failed to attach thread for cache check");
                return false;
            }
        } else if (attachResult != JNI_OK) {
            LOGE("Failed to get JNI environment for cache check");
            return false;
        }

        // Find VideoDownloader class
        jclass downloaderClass = env->FindClass("com/dorflix/app/video/VideoDownloader");
        if (!downloaderClass) {
            LOGE("Could not find VideoDownloader class for cache check");
            if (attached) jvm->DetachCurrentThread();
            return false;
        }

        // Check if video is cached first
        jmethodID isCachedMethod = env->GetStaticMethodID(downloaderClass, "nativeIsVideoCached",
                                                         "(Ljava/lang/String;)Z");
        if (!isCachedMethod) {
            LOGE("Could not find nativeIsVideoCached method");
            if (attached) jvm->DetachCurrentThread();
            return false;
        }

        // Get cached path
        jmethodID getCachedPathMethod = env->GetStaticMethodID(downloaderClass, "nativeGetCachedPath",
                                                              "(Ljava/lang/String;)Ljava/lang/String;");
        if (!getCachedPathMethod) {
            LOGE("Could not find nativeGetCachedPath method");
            if (attached) jvm->DetachCurrentThread();
            return false;
        }

        // Create Java string for URL
        jstring jUrl = env->NewStringUTF(videoPath.c_str());
        if (!jUrl) {
            LOGE("Failed to create Java string for URL");
            if (attached) jvm->DetachCurrentThread();
            return false;
        }

        // Check if cached
        jboolean isCached = env->CallStaticBooleanMethod(downloaderClass, isCachedMethod, jUrl);
        if (env->ExceptionCheck()) {
            LOGE("Exception occurred during cache check");
            env->ExceptionDescribe();
            env->ExceptionClear();
            env->DeleteLocalRef(jUrl);
            if (attached) jvm->DetachCurrentThread();
            return false;
        }

        if (isCached == JNI_TRUE) {
            LOGI("✅ Video found in cache - using cached version");

            // Get cached path
            jstring jCachedPath = static_cast<jstring>(env->CallStaticObjectMethod(downloaderClass, getCachedPathMethod, jUrl));
            if (env->ExceptionCheck()) {
                LOGE("Exception occurred getting cached path");
                env->ExceptionDescribe();
                env->ExceptionClear();
                env->DeleteLocalRef(jUrl);
                if (attached) jvm->DetachCurrentThread();
                return false;
            }

            if (jCachedPath) {
                const char* cachedPathChars = env->GetStringUTFChars(jCachedPath, nullptr);
                if (cachedPathChars) {
                    finalPath = cachedPathChars;
                    env->ReleaseStringUTFChars(jCachedPath, cachedPathChars);
                    LOGI("Using cached video path: %s", finalPath.c_str());
                }
                env->DeleteLocalRef(jCachedPath);
            }

            env->DeleteLocalRef(jUrl);
            if (attached) jvm->DetachCurrentThread();
        } else {
            LOGI("❌ Video not in cache - cannot play without download");

            // Clean up and return error - don't attempt synchronous download
            env->DeleteLocalRef(jUrl);
            if (attached) jvm->DetachCurrentThread();

            LOGE("Video not cached: %s - refusing to block main thread with download", videoPath.c_str());
            return false;
        }

        // Detach thread if we attached it
        if (attached) {
            if (jvm->DetachCurrentThread() != JNI_OK) {
                LOGW("Failed to detach thread after cache check");
            }
        }
    }

    // For network URLs, skip local file existence check
        if (!isNetworkUrl) {
            // Check if file exists (basic validation for local files)
            FILE* testFile = fopen(finalPath.c_str(), "rb");
            if (!testFile) {
                LOGE("Video file does not exist or cannot be accessed: %s", finalPath.c_str());
                return false;
            }
            fclose(testFile);
        }
    
    // Set format options for network streaming
    AVDictionary* options = nullptr;
    if (isNetworkUrl) {
        LOGI("=== CONFIGURING NETWORK OPTIONS FOR HTTPS STREAMING ===");

        // Basic network protocol support
        av_dict_set(&options, "protocol_whitelist", "file,http,https,tcp,tls,crypto,data", 0);
        av_dict_set(&options, "user_agent", "Dorflix/1.0 (Android)", 0);
        av_dict_set(&options, "timeout", "15000000", 0); // 15 seconds timeout
        av_dict_set(&options, "reconnect", "1", 0);
        av_dict_set(&options, "reconnect_at_eof", "1", 0);
        av_dict_set(&options, "reconnect_streamed", "1", 0);
        av_dict_set(&options, "reconnect_delay_max", "30", 0);

        // HTTPS/SSL specific options
        av_dict_set(&options, "tls_verify", "0", 0); // Disable SSL certificate verification for development
        av_dict_set(&options, "ssl_verifyhost", "0", 0); // Don't verify hostname matches certificate
        av_dict_set(&options, "ssl_verifypeer", "0", 0); // Don't verify peer certificate
        av_dict_set(&options, "http_proxy", "", 0); // Ensure no proxy interference
        av_dict_set(&options, "seekable", "0", 0); // Cloudinary URLs may not be seekable

        // Additional network options for better compatibility
        av_dict_set(&options, "rw_timeout", "15000000", 0); // 15 second read/write timeout
        av_dict_set(&options, "buffer_size", "65536", 0); // 64KB buffer
        av_dict_set(&options, "max_delay", "500000", 0); // Max delay for timestamp correction

        LOGI("Network options configured for HTTPS streaming");
    }

    // Open input file with FFmpeg 8+ - use finalPath (cached file for HTTPS, original for local files)
    int result = avformat_open_input(&m_formatContext, finalPath.c_str(), nullptr, &options);

    // Clean up options dictionary
    if (options) {
        av_dict_free(&options);
    }

    if (result < 0) {
        char error_buffer[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_strerror(result, error_buffer, sizeof(error_buffer));
        LOGE("Failed to open video file '%s': %s (error code: %d)",
             videoPath.c_str(), error_buffer, result);

        // Enhanced error logging for network URLs
        if (isNetworkUrl) {
            LOGE("NETWORK URL DEBUGGING:");
            LOGE("  -> URL starts with: %s", videoPath.substr(0, 10).c_str());
            LOGE("  -> Is HTTPS: %s", (videoPath.find("https://") == 0) ? "YES" : "NO");
            LOGE("  -> Network options were configured: YES");

            // Specific network error codes
            if (result == AVERROR(-2)) {  // ENOENT
                LOGE("  -> NETWORK ERROR: URL not accessible or protocol not supported");
                LOGE("  -> POSSIBLE CAUSES:");
                LOGE("    • HTTPS protocol not compiled into FFmpeg");
                LOGE("    • SSL/TLS library missing (OpenSSL)");
                LOGE("    • Firewall blocking HTTPS requests");
                LOGE("    • DNS resolution failed");
            } else if (result == AVERROR_INVALIDDATA) {
                LOGE("  -> NETWORK ERROR: Server response invalid or not MP4");
                LOGE("  -> POSSIBLE CAUSES:");
                LOGE("    • Cloudinary URL returns HTML error page");
                LOGE("    • Content-Type not video/mp4");
                LOGE("    • CORS or authentication issues");
            } else if (result == AVERROR(-5)) {  // EIO
                LOGE("  -> NETWORK ERROR: I/O failure during connection");
                LOGE("  -> POSSIBLE CAUSES:");
                LOGE("    • SSL handshake failed");
                LOGE("    • Connection timeout");
                LOGE("    • Server rejected request");
            } else if (result == AVERROR(-13)) {  // EACCES
                LOGE("  -> NETWORK ERROR: Access denied");
                LOGE("  -> POSSIBLE CAUSES:");
                LOGE("    • SSL certificate validation failed");
                LOGE("    • Server requires authentication");
            }
        } else {
            // Local file error codes
            if (result == AVERROR(-13)) {  // EACCES equivalent
                LOGE("  -> Permission denied - check file permissions");
            } else if (result == AVERROR(-2)) {  // ENOENT equivalent
                LOGE("  -> File not found - verify path is correct");
            } else if (result == AVERROR_INVALIDDATA) {
                LOGE("  -> Invalid file format - file may be corrupted");
            } else if (result == AVERROR(-5)) {  // EIO equivalent
                LOGE("  -> I/O error - disk issue");
            }
        }

        return false;
    }
    
    LOGI("Video file opened successfully");
    
    // Retrieve stream information with timeout and error handling
    result = avformat_find_stream_info(m_formatContext, nullptr);
    if (result < 0) {
        char error_buffer[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_strerror(result, error_buffer, sizeof(error_buffer));
        LOGE("Failed to retrieve stream info: %s (error code: %d)", error_buffer, result);

        // Clean up on failure
        avformat_close_input(&m_formatContext);
        m_formatContext = nullptr;

        if (result == AVERROR_INVALIDDATA) {
            LOGE("  -> File format not supported or corrupted");
        }

        return false;
    }
    
    LOGI("Stream information retrieved successfully");
    
    // Set interrupt callback for graceful shutdown
    m_formatContext->interrupt_callback.callback = interruptCallback;
    m_formatContext->interrupt_callback.opaque = this;
    
    // Get duration with bounds checking and validation
    if (m_formatContext->duration != AV_NOPTS_VALUE) {
        m_duration = static_cast<int>(m_formatContext->duration / AV_TIME_BASE);
        if (m_duration < 0) {
            m_duration = 0;
            LOGW("Duration was negative, set to 0");
        } else {
            LOGI("Video duration: %d seconds", m_duration);
        }
    } else {
        m_duration = 0;
        LOGW("Video duration is unknown (AV_NOPTS_VALUE)");
    }

    // Additional stream validation
    if (m_formatContext->nb_streams == 0) {
        LOGE("No streams found in video file");
        avformat_close_input(&m_formatContext);
        m_formatContext = nullptr;
        return false;
    }

    LOGI("Found %d streams in video file", m_formatContext->nb_streams);
    LOGI("=== VIDEO FILE OPENED SUCCESSFULLY ===");
    
    return true;
}

bool VideoDecoder::findVideoStream() {
    LOGI("=== FINDING VIDEO STREAM ===");

    // Find video stream
    for (unsigned int i = 0; i < m_formatContext->nb_streams; i++) {
        AVStream* stream = m_formatContext->streams[i];
        AVCodecParameters* codecParams = stream->codecpar;

        LOGI("Checking stream %d: codec_type=%d", i, codecParams->codec_type);

        if (codecParams->codec_type == AVMEDIA_TYPE_VIDEO) {
            LOGI("Found video stream at index %d", i);
            m_videoStreamIndex = i;
            m_videoWidth = codecParams->width;
            m_videoHeight = codecParams->height;

            LOGI("Video dimensions: %dx%d", m_videoWidth, m_videoHeight);

    // ===== PHASE 1: ENSURE ENUMERATION COMPLETES BEFORE DECODER SELECTION =====
    LOGI("🎯 WAITING FOR CODEC ENUMERATION: Ensuring enumeration completes before decoder selection");
    const int MAX_ENUMERATION_WAIT_MS = 5000; // 5 second timeout
    const int ENUMERATION_CHECK_INTERVAL_MS = 100;
    int totalWaitMs = 0;

    while (!isEnumerationComplete() && totalWaitMs < MAX_ENUMERATION_WAIT_MS) {
        LOGI("⏳ Waiting for codec enumeration to complete... (%d/%d ms)", totalWaitMs, MAX_ENUMERATION_WAIT_MS);
        std::this_thread::sleep_for(std::chrono::milliseconds(ENUMERATION_CHECK_INTERVAL_MS));
        totalWaitMs += ENUMERATION_CHECK_INTERVAL_MS;
    }

    if (!isEnumerationComplete()) {
        LOGW("⚠️ Codec enumeration did not complete within timeout - proceeding with available data");
        // Continue anyway - we might have partial enumeration data
    } else {
        LOGI("✅ Codec enumeration completed successfully");
    }

    // Check if we have any enumerated codecs
    auto enumeratedCodecs = getEnumeratedCodecs();
    if (enumeratedCodecs.empty()) {
        LOGW("⚠️ No enumerated codecs available - MediaCodec may not work properly");
    } else {
        LOGI("📊 Found %zu enumerated codecs for selection", enumeratedCodecs.size());
    }

    // ===== HARDWARE SURFACE MODE ONLY - WITH ENUMERATION-BASED SELECTION =====
    LOGI("🎯 HARDWARE SURFACE MODE WITH ENUMERATION: Using enumerated codec data for selection");
    const char* codecName = avcodec_get_name(codecParams->codec_id);
    LOGI("🎯 HARDWARE SURFACE MODE WITH ENUMERATION: Codec name: %s, codec_id: %d", codecName, codecParams->codec_id);

    // Use VideoDecoderFactory to intelligently select decoder based on enumerated codecs
    DecoderType selectedType = VideoDecoderFactory::selectBestDecoder(codecName, codecParams->codec_id);
    LOGI("🎯 HARDWARE SURFACE MODE WITH ENUMERATION: Factory selected decoder type: %d", static_cast<int>(selectedType));

    if (selectedType == DecoderType::NONE) {
        LOGE("❌ CRITICAL: No suitable decoder available for %s (codec_id: %d)", codecName, codecParams->codec_id);
        LOGE("   → Enumeration may have failed or no hardware codecs available");
        return false; // Fail completely - no decoders available
    }

    m_videoDecoder = VideoDecoderFactory::createDecoder(selectedType);

    if (!m_videoDecoder) {
        LOGE("❌ CRITICAL: Decoder creation failed for type %d", static_cast<int>(selectedType));
        LOGE("   → Factory selected unsupported decoder type");
        return false; // Fail completely - decoder creation failed
    }

    LOGI("✅ Decoder created successfully: type=%d", static_cast<int>(selectedType));

    // ===== PASS SELECTED CODEC INFORMATION TO DECODER =====
    // Provide the decoder with information about the selected codec for optimal configuration
    auto* mediaCodecDecoder = dynamic_cast<MediaCodecHardwareDecoder*>(m_videoDecoder.get());
    if (mediaCodecDecoder) {
        const EnumeratedCodecInfo& selectedCodec = VideoDecoderFactory::getSelectedCodec();
        if (selectedCodec.isValid) {
            LOGI("🎯 Setting selected codec info on MediaCodec decoder: %s", selectedCodec.name.c_str());
            mediaCodecDecoder->setSelectedCodecInfo(selectedCodec);
            LOGI("✅ Selected codec info set on MediaCodec decoder");
        } else {
            LOGW("⚠️ No valid selected codec info available - decoder will use enumeration-based selection");
        }
    } else {
        LOGW("⚠️ Created decoder is not MediaCodecHardwareDecoder - cannot set codec info");
    }

    // ===== DEFER INITIALIZATION UNTIL SURFACE IS AVAILABLE =====
    // Don't initialize yet - wait for setSurface() to be called first
    // This ensures the surface is available during MediaCodec configuration
    LOGI("⏳ Deferring decoder initialization until surface is available");
    LOGI("🎯 HARDWARE SURFACE MODE ONLY: Decoder created but not initialized (waiting for surface)");
    LOGI("=== VIDEO STREAM FOUND (WAITING FOR SURFACE) ===");
    return true;
        }
    }

    LOGE("No video stream found in %d streams", m_formatContext->nb_streams);
    return false;
}

bool VideoDecoder::initializeVideoCodec() {
    // Video codec is now initialized by the hierarchical decoder in findVideoStream
    return m_videoDecoder != nullptr;
}



void VideoDecoder::decodeLoop() {
    LOGI("=== VIDEO DECODER THREAD STARTED ===");
    LOGI("Initial thread state - decoding: %d, playing: %d, alive: %d",
         static_cast<int>(m_isDecoding), static_cast<int>(m_isPlaying), static_cast<int>(m_isAlive.load()));

    AVPacket* packet = av_packet_alloc();
    if (!packet) {
        LOGE("Failed to allocate packet");
        return;
    }

    int loop_iterations = 0;

    while (m_isDecoding && m_isPlaying && m_isAlive.load()) {
        loop_iterations++;
        LOGI("Decode loop iteration %d - flags: decoding=%d, playing=%d, alive=%d",
             loop_iterations, static_cast<int>(m_isDecoding), static_cast<int>(m_isPlaying), static_cast<int>(m_isAlive.load()));

        if (shouldStopDecoding() || !m_isAlive.load()) {
            LOGI("Decode loop stopping - shouldStopDecoding: %d, alive: %d",
                 shouldStopDecoding(), static_cast<int>(m_isAlive.load()));
            break;
        }

        // Read frame from file
        int result = av_read_frame(m_formatContext, packet);
        if (result < 0) {
            if (result == AVERROR_EOF) {
                // End of file reached
                if (m_onVideoCompleted) {
                    m_onVideoCompleted();
                }
                break;
            }
            // Log error but continue
            continue;
        }

        // 🎯 TIKTOK-STYLE: Pre-Decode Packet Validation
        if (!isValidPacket(packet)) {
            LOGW("❌ REJECTING corrupted packet: size=%d, pts=%" PRId64 ", dts=%" PRId64,
                 packet->size, packet->pts, packet->dts);
            av_packet_unref(packet);
            continue; // Skip corrupted packet, don't crash
        }

        // ===== ADDITIONAL SAFETY: Check for crash addresses in packet data =====
        // Even if validation passes, some packets may have corrupted data pointers
        if (packet->data) {
            uintptr_t dataPtr = reinterpret_cast<uintptr_t>(packet->data);
#if UINTPTR_MAX == UINT64_MAX
            if (dataPtr == 0x100000000ULL) {
                LOGE("❌ CRITICAL: Packet data points to crash address 0x100000000 - rejecting!");
                av_packet_unref(packet);
                continue;
            }
#endif
        }

        // Process video packets
        if (packet->stream_index == m_videoStreamIndex) {
            // ===== PHASE 3: CONNECT ENUMERATED CODECS TO ACTUAL DECODING =====
            // Use hierarchical decoder to decode the frame
            if (m_videoDecoder) {
                LOGI("🎯 PHASE 3: Sending packet to hierarchical decoder (enumerated codec integration)");

                AVFrame* decodedFrame = m_videoDecoder->decodePacket(packet);

                // ===== SURFACE MODE HANDLING =====
                // In surface mode, MediaCodec returns nullptr immediately (asynchronous)
                // Frames are delivered via callback instead of synchronously
                auto* mediaCodecDecoder = dynamic_cast<MediaCodecHardwareDecoder*>(m_videoDecoder.get());
                bool isSurfaceMode = mediaCodecDecoder && mediaCodecDecoder->isReady(); // Surface mode if decoder is ready and has surface

                if (isSurfaceMode) {
                    LOGI("🎯 PHASE 3: Operating in surface mode - frames delivered asynchronously via callback");
                    // In surface mode, decodePacket returns nullptr immediately
                    // Frames arrive asynchronously through the frame delivery callback
                    // No further processing needed here for video frames
                } else if (decodedFrame) {
                    // ===== BUFFER MODE: Process frame synchronously =====
                    LOGI("🎯 PHASE 3: Buffer mode - processing frame synchronously");
                    LOGI("Frame decoded successfully: %dx%d format=%d pts=%" PRId64,
                         decodedFrame->width, decodedFrame->height, decodedFrame->format, decodedFrame->pts);

                    // Send frame to sync controller for synchronized rendering
                    if (m_syncController) {
                        LOGI("Sending frame %p to sync controller (pts=%" PRId64 ")", decodedFrame, decodedFrame->pts);
                        try {
                            m_syncController->processVideoFrame(decodedFrame);
                            LOGD("processVideoFrame completed successfully");
                        } catch (const std::exception& e) {
                            LOGE("Failed to process video frame through sync controller: %s", e.what());
                        } catch (...) {
                            LOGE("Unknown error processing video frame through sync controller");
                        }
                    } else {
                        LOGW("Sync controller is null, skipping video frame processing");
                    }

                    // Calculate current position for progress updates
                    if (decodedFrame->pts != AV_NOPTS_VALUE) {
                        int64_t pts = decodedFrame->best_effort_timestamp;
                        double pts_seconds = pts * av_q2d(m_formatContext->streams[m_videoStreamIndex]->time_base);
                        m_currentPosition = static_cast<int>(pts_seconds);

                        // Update progress
                        if (m_onVideoProgress) {
                            m_onVideoProgress(m_currentPosition);
                        }
                    }

                    // Note: The hierarchical decoder manages frame lifecycle
                    // Don't free the frame here - the decoder handles it
                } else {
                    LOGW("🎯 PHASE 3: Decoder returned null frame (expected in surface mode, unexpected in buffer mode)");
                }

                LOGI("✅ PHASE 3: Packet processing completed for enumerated codec integration");
            } else {
                LOGE("❌ PHASE 3: No hierarchical decoder available - enumerated codec integration failed");
            }
        }
        // Process audio packets through sync controller
        else if (m_audioDecoder && m_audioDecoder->hasAudioStream() &&
                 packet->stream_index == m_audioDecoder->getAudioStreamIndex()) {
            // ===== ADDITIONAL VALIDATION FOR AUDIO PACKETS =====
            // Even though we validated earlier, double-check for corrupted audio packets
            if (packet->data) {
                uintptr_t dataPtr = reinterpret_cast<uintptr_t>(packet->data);
#if UINTPTR_MAX == UINT64_MAX
                if (dataPtr == 0x100000000ULL) {
                    LOGE("❌ CRITICAL: Audio packet data points to crash address 0x100000000 - rejecting!");
                    av_packet_unref(packet);
                    continue;
                }
#endif
            }

            // Send audio packet to sync controller for synchronized playback
            if (m_syncController && packet) {
                try {
                    m_syncController->processAudioPacket(packet);
                } catch (const std::exception& e) {
                    LOGE("Failed to process audio packet through sync controller: %s", e.what());
                } catch (...) {
                    LOGE("Unknown error processing audio packet through sync controller");
                }
            } else {
                LOGW("Sync controller or packet is null, skipping audio packet processing");
            }
        }

        av_packet_unref(packet);

        // Small delay to prevent busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    av_packet_free(&packet);
    m_isDecoding = false;

    LOGI("=== VIDEO DECODER THREAD EXITING ===");
}

void VideoDecoder::renderFrame(AVFrame* frame) {
    LOGI("=== RENDER FRAME START ===");
    LOGI("Frame: %p, Renderer initialized: %d", frame, m_renderer ? m_renderer->isInitialized() : 0);

    // ===== SURFACE MODE DETECTION =====
    // In surface mode, MediaCodec renders directly - frames are synthetic (just timing)
    bool isSyntheticFrame = (frame && (frame->format == AV_PIX_FMT_NONE ||
                              frame->format < 0 ||
                              !frame->data[0] ||
                              frame->width == 0 || frame->height == 0));

    if (isSyntheticFrame) {
        LOGI("🎬 Received synthetic frame (surface mode) - MediaCodec handles direct rendering");
        LOGI("   → Frame timing: pts=%" PRId64 ", dimensions=%dx%d, format=%d",
             frame ? frame->pts : -1, frame ? frame->width : 0, frame ? frame->height : 0,
             frame ? frame->format : -1);
        // In surface mode, no additional rendering needed - MediaCodec already rendered
        return;
    }

    // 🎭 TIKTOK-STYLE: Frame Error Concealment
    AVFrame* frameToRender = frame;

    // Check if frame is corrupted or null
    if (!frame || !isFrameValid(frame)) {
        LOGW("Frame is corrupted or null - applying error concealment");
        frameToRender = concealCorruptedFrame(frame); // Returns valid frame or nullptr

        if (!frameToRender) {
            LOGW("No concealment frame available - skipping render");
            return; // Skip this frame, don't crash
        }
    }

    if (!m_renderer || !m_renderer->isInitialized()) {
        LOGE("RENDER ERROR: Renderer not initialized!");
        // Don't free frameToRender if it was allocated by concealment
        if (frameToRender != frame) {
            av_frame_free(&frameToRender);
        }
        return;
    }

    LOGI("Attempting render for frame %p (%dx%d format=%d)", frameToRender,
         frameToRender->width, frameToRender->height, frameToRender->format);

    // Use the selected renderer (hierarchical system)
    // Note: Renderer should not free the frame - sync system manages lifecycle
    if (!m_renderer->renderFrame(frameToRender)) {
        LOGE("RENDER ERROR: renderFrame failed!");
        // Don't free the frame - let sync system handle cleanup
    } else {
        LOGI("Frame rendered successfully");
    }

    // Clean up concealment frame if it was allocated
    if (frameToRender != frame) {
        av_frame_free(&frameToRender);
    }

    LOGI("=== RENDER FRAME COMPLETE ===\n");
}

void VideoDecoder::notifyError(int errorCode, const std::string& message) {
    if (m_onVideoError) {
        m_onVideoError(errorCode, message);
    }
}

// ===== CRITICAL ERROR HANDLING: COMPLETE SHUTDOWN ON MEDIACODEC FAILURE =====
void VideoDecoder::shutdownOnCriticalError(int errorCode, const std::string& message) {
    LOGE("🚨 CRITICAL ERROR - SHUTTING DOWN VIDEO DECODER COMPLETELY: %s", message.c_str());

    // ===== IMMEDIATE COMPLETE SHUTDOWN =====
    // Stop all flags and activity
    m_isAlive = false;
    m_isPlaying = false;
    m_isDecoding = false;
    m_isPrepared = false;
    m_isSeeking = false;

    // ===== STOP ALL THREADS =====
    // Stop decode thread immediately
    if (m_decodeThread.joinable()) {
        LOGI("🛑 Joining decode thread during critical shutdown...");
        m_decodeThread.join();
        LOGI("✅ Decode thread joined and stopped");
    }

    // ===== STOP SYNC CONTROLLER =====
    if (m_syncController) {
        LOGI("🛑 Stopping A/V sync controller during critical shutdown...");
        m_syncController->stopSync();
        LOGI("✅ A/V sync controller stopped");
    }

    // ===== STOP AUDIO DECODER =====
    if (m_audioDecoder) {
        LOGI("🛑 Stopping audio decoder during critical shutdown...");
        m_audioDecoder->cleanup();
        LOGI("✅ Audio decoder stopped");
    }

    // ===== STOP PRELOADER =====
    if (m_preloader) {
        LOGI("🛑 Stopping preloader during critical shutdown...");
        m_preloader->stopPreloading();
        LOGI("✅ Preloader stopped");
    }

    // ===== COMPLETE RESOURCE CLEANUP =====
    LOGI("🧹 Performing complete resource cleanup during critical shutdown...");
    cleanup();
    LOGI("✅ All resources cleaned up");

    // ===== NOTIFY ERROR =====
    LOGI("📢 Notifying error callback: code=%d, message='%s'", errorCode, message.c_str());
    notifyError(errorCode, message);

    LOGE("✅ VIDEO DECODER COMPLETE CRITICAL SHUTDOWN FINISHED");
    LOGE("   → All threads stopped");
    LOGE("   → All resources cleaned up");
    LOGE("   → Error notification sent");
    LOGE("   → No further activity will occur");
}

void VideoDecoder::cleanup() {
    LOGI("=== STARTING VIDEO DECODER CLEANUP ===");
    LOGI("Note: cleanup() cleans resources but keeps decoder alive for reuse");

    // Stop threads and wait for them to exit
    stop();

    // Clean up AudioDecoder
    if (m_audioDecoder) {
        LOGI("Cleaning up AudioDecoder");
        m_audioDecoder->cleanup();
    }

    LOGI("Freeing FFmpeg resources after threads stopped");

    // Clean up FFmpeg 8+ resources using proper cleanup functions
    // Each cleanup is protected against double-free
    if (m_frame) {
        LOGI("Freeing AVFrame");
        av_frame_free(&m_frame);
        m_frame = nullptr;
    }

    if (m_swsContext) {
        LOGI("Freeing SWS scaling context");
        sws_freeContext(m_swsContext);
        m_swsContext = nullptr;
    }

    // Video codec context is managed by hierarchical decoder - no cleanup needed here
    LOGI("Video codec context managed by hierarchical decoder");

    if (m_formatContext) {
        LOGI("Closing format context");
        avformat_close_input(&m_formatContext);
        m_formatContext = nullptr;
    }

    // Reset state
    m_isPrepared = false;
    m_duration = 0;
    m_currentPosition = 0;
    m_videoWidth = 0;
    m_videoHeight = 0;
    m_videoStreamIndex = -1;

    // Clear surface reference (don't own it)
    m_surface = nullptr;

    // Reset renderer selection (will be re-selected on next setSurface)
    m_rendererTypeSelected = false;
    if (m_renderer) {
        m_renderer->cleanup();
        m_renderer.reset();
    }

    LOGI("=== VIDEO DECODER CLEANUP COMPLETED SUCCESSFULLY ===");
}





void VideoDecoder::destroy() {
    LOGI("=== DESTROYING VIDEO DECODER (TIKTOK-STYLE) ===");

    // Set destroy flag - this decoder is no longer usable
    m_isDestroyed = true;

    // Stop all operations immediately
    m_isAlive = false;
    m_isPlaying = false;
    m_isDecoding = false;
    m_isPrepared = false;

    // Stop and join decode thread
    if (m_decodeThread.joinable()) {
        LOGI("Waiting for decode thread to join in destroy...");
        m_decodeThread.join();
        LOGI("Decode thread joined in destroy");
    }

    // Clean up AudioDecoder
    if (m_audioDecoder) {
        LOGI("Destroying AudioDecoder");
        delete m_audioDecoder;
        m_audioDecoder = nullptr;
    }

    // Aggressive cleanup - free all resources immediately
    if (m_frame) {
        av_frame_free(&m_frame);
        m_frame = nullptr;
        LOGI("Destroyed AVFrame");
    }

    if (m_swsContext) {
        sws_freeContext(m_swsContext);
        m_swsContext = nullptr;
        LOGI("Destroyed SWS context");
    }

    // Video codec context is managed by hierarchical decoder - no cleanup needed here
    LOGI("Video codec context managed by hierarchical decoder");

    if (m_formatContext) {
        avformat_close_input(&m_formatContext);
        m_formatContext = nullptr;
        LOGI("Destroyed format context");
    }

    if (m_cacheManager) {
        delete m_cacheManager;
        m_cacheManager = nullptr;
        LOGI("Destroyed cache manager");
    }

    // Clear all callbacks to prevent dangling references
    m_onVideoPrepared = nullptr;
    m_onVideoStarted = nullptr;
    m_onVideoPaused = nullptr;
    m_onVideoStopped = nullptr;
    m_onVideoCompleted = nullptr;
    m_onVideoError = nullptr;
    m_onVideoProgress = nullptr;
    m_onVideoBufferingStarted = nullptr;
    m_onVideoBufferingEnded = nullptr;
    m_onVideoSeekComplete = nullptr;
    m_onVideoSizeChanged = nullptr;

    // Clear surface reference
    m_surface = nullptr;

    // Reset all state
    m_duration = 0;
    m_currentPosition = 0;
    m_videoWidth = 0;
    m_videoHeight = 0;
    m_videoStreamIndex = -1;
    m_volume = 1.0f;
    m_isMuted = false;

    LOGI("=== VIDEO DECODER DESTROYED COMPLETELY ===");
}

bool VideoDecoder::shouldStopDecoding() const {
    return !m_isDecoding;
}

bool VideoDecoder::isDestroyed() const {
    return m_isDestroyed;
}

// 🎯 TIKTOK-STYLE: Pre-Decode Packet Validation
bool VideoDecoder::isValidPacket(AVPacket* packet) {
    if (!packet) {
        LOGE("Packet is null");
        return false;
    }

    // Size validation - reject empty or oversized packets
    if (packet->size <= 0) {
        LOGW("Packet has invalid size: %d", packet->size);
        return false;
    }

    if (packet->size > MAX_PACKET_SIZE) {
        LOGW("Packet too large: %d bytes (max: %d)", packet->size, MAX_PACKET_SIZE);
        return false;
    }

    // PTS validation - reject invalid timestamps
    if (packet->pts == AV_NOPTS_VALUE) {
        LOGW("Packet has invalid PTS (AV_NOPTS_VALUE)");
        return false;
    }

    if (packet->pts < 0) {
        LOGW("Packet has negative PTS: %" PRId64, packet->pts);
        return false;
    }

    // DTS validation (if present)
    if (packet->dts != AV_NOPTS_VALUE && packet->dts < 0) {
        LOGW("Packet has negative DTS: %" PRId64, packet->dts);
        return false;
    }

    // Stream index validation
    if (packet->stream_index < 0 || packet->stream_index >= (int)m_formatContext->nb_streams) {
        LOGW("Packet has invalid stream index: %d", packet->stream_index);
        return false;
    }

    return true;
}

// 🎭 TIKTOK-STYLE: Frame Error Concealment
bool VideoDecoder::isFrameValid(AVFrame* frame) {
    if (!frame) {
        LOGE("Frame is null");
        return false;
    }

    // Basic frame properties validation
    if (frame->width <= 0 || frame->height <= 0 || frame->width > 4096 || frame->height > 4096) {
        LOGE("Invalid frame dimensions: %dx%d", frame->width, frame->height);
        return false;
    }

    if (frame->format < 0 || frame->format >= AV_PIX_FMT_NB) {
        LOGE("Invalid pixel format: %d", frame->format);
        return false;
    }

    // Note: frame->data and frame->linesize are arrays in AVFrame, so they're always valid
    // Individual data pointers will be validated in the loop below

    // Get number of planes for this pixel format
    int numPlanes = av_pix_fmt_count_planes(static_cast<AVPixelFormat>(frame->format));
    if (numPlanes <= 0) {
        LOGE("Invalid number of planes for format %d: %d", frame->format, numPlanes);
        return false;
    }

    // Validate ALL data planes, not just data[0]
    for (int plane = 0; plane < numPlanes && plane < AV_NUM_DATA_POINTERS; plane++) {
        if (!frame->data[plane]) {
            LOGE("Frame data[%d] is null (format=%d, expected %d planes)",
                 plane, frame->format, numPlanes);
            return false;
        }

        // Validate pointer is in valid memory range (not corrupted)
        uintptr_t dataPtr = reinterpret_cast<uintptr_t>(frame->data[plane]);
#if UINTPTR_MAX == UINT64_MAX
        // 64-bit architecture
        if (dataPtr < 0x1000 || dataPtr >= 0x8000000000000000ULL) {
            LOGE("Frame data[%d] pointer appears corrupted: %p (format=%d)",
                 plane, frame->data[plane], frame->format);
            return false;
        }
        // Check for the specific crash address (64-bit)
        if (dataPtr == 0x100000000ULL) {
            LOGE("CRITICAL: Frame data[%d] contains crash address 0x100000000! (format=%d)",
                 plane, frame->format);
            return false;
        }
#else
        // 32-bit architecture
        if (dataPtr < 0x1000 || dataPtr >= 0xC0000000) {  // Check against typical 32-bit address space limit
            LOGE("Frame data[%d] pointer appears corrupted: %p (format=%d)",
                 plane, frame->data[plane], frame->format);
            return false;
        }
        // Check for the specific crash address (32-bit)
        if (dataPtr == 0x10000000) {
            LOGE("CRITICAL: Frame data[%d] contains crash address 0x10000000! (format=%d)",
                 plane, frame->format);
            return false;
        }
#endif

        LOGD("Validated data[%d] = %p for format %d", plane, frame->data[plane], frame->format);
    }

    LOGD("Frame validation passed: %dx%d format=%d planes=%d",
         frame->width, frame->height, frame->format, numPlanes);
    return true;
}

AVFrame* VideoDecoder::concealCorruptedFrame(AVFrame* previousFrame) {
    LOGI("🎭 Applying frame error concealment");

    // Method 1: Use previous frame if available (freeze-frame)
    if (previousFrame && isFrameValid(previousFrame)) {
        LOGI("Using freeze-frame concealment from previous valid frame");
        // Create a copy of the previous frame
        AVFrame* concealed = av_frame_clone(previousFrame);
        if (concealed) {
            LOGI("✅ Freeze-frame concealment successful");
            return concealed;
        }
    }

    // Method 2: Create a black frame as last resort
    LOGI("Creating black frame for concealment");
    AVFrame* blackFrame = av_frame_alloc();
    if (!blackFrame) {
        LOGE("Failed to allocate black frame for concealment");
        return nullptr;
    }

    // Set up black frame with current video dimensions
    blackFrame->format = AV_PIX_FMT_RGB24; // Safe fallback format
    blackFrame->width = m_videoWidth > 0 ? m_videoWidth : 640;
    blackFrame->height = m_videoHeight > 0 ? m_videoHeight : 480;

    // Allocate buffer
    int ret = av_frame_get_buffer(blackFrame, 32);
    if (ret < 0) {
        LOGE("Failed to allocate buffer for black frame: %d", ret);
        av_frame_free(&blackFrame);
        return nullptr;
    }

    // Fill with black (RGB24 = 3 bytes per pixel)
    size_t frameSize = blackFrame->width * blackFrame->height * 3;
    if (blackFrame->data[0]) {
        memset(blackFrame->data[0], 0, frameSize); // Black = RGB(0,0,0)
    }

    blackFrame->pts = 0; // Will be set by caller
    blackFrame->pict_type = AV_PICTURE_TYPE_NONE;

    LOGI("✅ Black frame concealment successful (%dx%d)", blackFrame->width, blackFrame->height);
    return blackFrame;
}

// Static interrupt callback for FFmpeg operations
int VideoDecoder::interruptCallback(void* ctx) {
    VideoDecoder* decoder = static_cast<VideoDecoder*>(ctx);
    if (decoder && !decoder->m_isAlive.load()) {
        LOGI("FFmpeg operation interrupted (decoder not alive)");
        return 1; // Interrupt the operation
    }
    return 0; // Continue the operation
}