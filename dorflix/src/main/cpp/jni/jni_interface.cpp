#include "jni_interface.h"
#include "jni_logging.h"
#include "../video_decoder/VideoDecoder.h"
#include "../video_decoder/VideoCacheManager.h"
#include "../video_decoder/VideoDecoderFactory.h"
#include "../video_preloader/Preloader.h"
#include "../memory/MemoryPool.h"
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <jni.h>
#include <string>
#include <memory>
#include <unordered_map>
#include <ctime>
#include <android/log.h>

#define LOG_TAG "DorflixJNI"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

// Global JNI readiness flag - set to true when JNI_OnLoad completes successfully
static bool g_jniReady = false;

// Global JVM reference for thread attachment
static JavaVM* g_jvm = nullptr;

// Global references for callbacks
static jobject g_callbackObject = nullptr;

// ===== MEDIACODEC ENUMERATION BRIDGE GLOBALS =====
// Global storage for enumerated codecs with thread safety
static std::vector<EnumeratedCodecInfo> g_enumeratedCodecs;
static bool g_enumerationComplete = false;
static std::mutex g_enumerationMutex; // Thread safety for enumeration data

// Video downloader implementation
#include "../video_decoder/VideoDownloaderJNI.h"

class JNI_VideoDownloader : public VideoDownloaderInterface {
public:
    std::string downloadVideo(const std::string& url) override {
        if (!g_jvm) {
            return "";
        }

        JNIEnv* env = nullptr;
        bool attached = false;

        // Attach current thread to JVM
        int attachResult = g_jvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
        if (attachResult == JNI_EDETACHED) {
            if (g_jvm->AttachCurrentThread(&env, nullptr) == JNI_OK) {
                attached = true;
            } else {
                return "";
            }
        } else if (attachResult != JNI_OK) {
            return "";
        }

        // Find VideoDownloader class
        jclass downloaderClass = env->FindClass("com/dorflix/app/video/VideoDownloader");
        if (!downloaderClass) {
            if (attached) g_jvm->DetachCurrentThread();
            return "";
        }

        // Get download method
        jmethodID downloadMethod = env->GetStaticMethodID(downloaderClass, "nativeDownloadVideo",
                                                         "(Ljava/lang/String;)Ljava/lang/String;");
        if (!downloadMethod) {
            if (attached) g_jvm->DetachCurrentThread();
            return "";
        }

        // Create Java string for URL
        jstring jUrl = env->NewStringUTF(url.c_str());
        if (!jUrl) {
            if (attached) g_jvm->DetachCurrentThread();
            return "";
        }

        // Call the download method
        jstring jResult = static_cast<jstring>(env->CallStaticObjectMethod(downloaderClass, downloadMethod, jUrl));

        // Clean up
        env->DeleteLocalRef(jUrl);

        if (env->ExceptionCheck()) {
            env->ExceptionDescribe();
            env->ExceptionClear();
            if (attached) g_jvm->DetachCurrentThread();
            return "";
        }

        if (!jResult) {
            if (attached) g_jvm->DetachCurrentThread();
            return "";
        }

        // Get the result string
        const char* resultChars = env->GetStringUTFChars(jResult, nullptr);
        if (!resultChars) {
            env->DeleteLocalRef(jResult);
            if (attached) g_jvm->DetachCurrentThread();
            return "";
        }

        std::string finalPath = resultChars;
        env->ReleaseStringUTFChars(jResult, resultChars);
        env->DeleteLocalRef(jResult);

        // Detach thread if we attached it
        if (attached) {
            if (g_jvm->DetachCurrentThread() != JNI_OK) {
                // Log but continue
            }
        }

        return finalPath;
    }
};

// Global downloader instance
static JNI_VideoDownloader g_jniVideoDownloader;
VideoDownloaderInterface* g_videoDownloader = &g_jniVideoDownloader;

// Export JVM getter function so video_decoder can access it
extern "C" JNIEXPORT JavaVM* JNICALL getGlobalJVM() {
    return g_jvm;
}

// Forward declarations for FFmpeg verification functions
static bool verifyFFmpegFunctions();
static bool verifyFFmpegVersions();

// Runtime library verification function with enhanced error handling
static bool verifyFFmpegLibraries() {
    LOGI("=== Starting FFmpeg library verification ===");

    // First, verify that FFmpeg functions are available (this is the most important check)
    if (!verifyFFmpegFunctions()) {
        LOGE("FFmpeg function verification failed - libraries not loaded");
        return false;
    }

    LOGI(" FFmpeg functions are available");

    // Test basic FFmpeg functionality with a dummy operation
    // Instead of trying to open a file, test core functions directly
    bool basicTestPassed = false;
    try {
        // Test format context creation/destruction
        AVFormatContext* ctx = avformat_alloc_context();
        if (ctx) {
            avformat_free_context(ctx);
            basicTestPassed = true;
            LOGI(" FFmpeg format context operations work");
        } else {
            LOGE("Failed to allocate FFmpeg format context");
        }

        // Test codec operations
        const AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_H264);
        if (codec) {
            LOGI(" FFmpeg codec operations work");
        } else {
            LOGW("� H.264 decoder not found, but this may be normal");
        }

    } catch (const std::exception& e) {
        LOGE("L Exception during FFmpeg basic test: %s", e.what());
        return false;
    } catch (...) {
        LOGE("L Unknown exception during FFmpeg basic test");
        return false;
    }

    if (!basicTestPassed) {
        LOGE("L Basic FFmpeg operations failed");
        return false;
    }

    // Additional verification: check library versions
    if (!verifyFFmpegVersions()) {
        LOGW("� FFmpeg version verification failed, but continuing...");
    }

    LOGI(" FFmpeg libraries verified successfully");
    return true;
}

// Helper function to verify FFmpeg functions are available
static bool verifyFFmpegFunctions() {
    // Check core codec functions by calling them
    const AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec) {
        LOGE("FFmpeg codec functions not available (avcodec_find_decoder failed)");
        return false;
    }
    
    AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        LOGE("FFmpeg codec allocation functions not available");
        return false;
    }
    avcodec_free_context(&codec_ctx);
    
    // Check scaling functions by calling them
    AVFrame* src_frame = av_frame_alloc();
    AVFrame* dst_frame = av_frame_alloc();
    if (!src_frame || !dst_frame) {
        LOGE("FFmpeg frame allocation failed for scaling test");
        av_frame_free(&src_frame);
        av_frame_free(&dst_frame);
        return false;
    }
    
    struct SwsContext* sws_ctx = sws_getContext(
        640, 480, AV_PIX_FMT_YUV420P,
        640, 480, AV_PIX_FMT_YUV420P,
        SWS_BILINEAR, nullptr, nullptr, nullptr
    );
    
    av_frame_free(&src_frame);
    av_frame_free(&dst_frame);
    
    if (!sws_ctx) {
        LOGE("FFmpeg scaling functions not available (sws_getContext failed)");
        return false;
    }
    sws_freeContext(sws_ctx);
    
    // Check utility functions by calling them
    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        LOGE("FFmpeg frame allocation functions not available");
        return false;
    }
    av_frame_free(&frame);
    
    // Test basic function calls
    try {
        // Test codec finding
        const AVCodec* codec2 = avcodec_find_decoder(AV_CODEC_ID_H264);
        if (!codec2) {
            LOGW("H.264 decoder not found, but this may be normal");
        }
        
        // Test frame allocation
        AVFrame* frame2 = av_frame_alloc();
        if (!frame2) {
            LOGE("Failed to allocate test frame");
            return false;
        }
        av_frame_free(&frame2);
        
        LOGI("FFmpeg functions verification passed");
        return true;
        
    } catch (const std::exception& e) {
        LOGE("Exception during FFmpeg function verification: %s", e.what());
        return false;
    } catch (...) {
        LOGE("Unknown exception during FFmpeg function verification");
        return false;
    }
}

// Helper function to verify FFmpeg library versions
static bool verifyFFmpegVersions() {
    try {
        int libavutil_version = avutil_version();
        int libavcodec_version = avcodec_version();
        int libavformat_version = avformat_version();
        int libswscale_version = swscale_version();

        // Decode FFmpeg version numbers: (major << 16) | (minor << 8) | micro
        int avutil_major = (libavutil_version >> 16) & 0xFF;
        int avutil_minor = (libavutil_version >> 8) & 0xFF;
        int avutil_micro = libavutil_version & 0xFF;

        int avcodec_major = (libavcodec_version >> 16) & 0xFF;
        int avcodec_minor = (libavcodec_version >> 8) & 0xFF;
        int avcodec_micro = libavcodec_version & 0xFF;

        int avformat_major = (libavformat_version >> 16) & 0xFF;
        int avformat_minor = (libavformat_version >> 8) & 0xFF;
        int avformat_micro = libavformat_version & 0xFF;

        int swscale_major = (libswscale_version >> 16) & 0xFF;
        int swscale_minor = (libswscale_version >> 8) & 0xFF;
        int swscale_micro = libswscale_version & 0xFF;

        LOGI("FFmpeg library versions:");
        LOGI("  libavutil: %d.%d.%d (raw: %d)", avutil_major, avutil_minor, avutil_micro, libavutil_version);
        LOGI("  libavcodec: %d.%d.%d (raw: %d)", avcodec_major, avcodec_minor, avcodec_micro, libavcodec_version);
        LOGI("  libavformat: %d.%d.%d (raw: %d)", avformat_major, avformat_minor, avformat_micro, libavformat_version);
        LOGI("  libswscale: %d.%d.%d (raw: %d)", swscale_major, swscale_minor, swscale_micro, libswscale_version);

        // Check for minimum version requirements (FFmpeg 4.0+)
        const int MIN_MAJOR = 4;

        if (avutil_major < MIN_MAJOR) {
            LOGW("libavutil version %d.%d.%d is below minimum required %d.0.0",
                 avutil_major, avutil_minor, avutil_micro, MIN_MAJOR);
            return false;
        }

        if (avcodec_major < MIN_MAJOR) {
            LOGW("libavcodec version %d.%d.%d is below minimum required %d.0.0",
                 avcodec_major, avcodec_minor, avcodec_micro, MIN_MAJOR);
            return false;
        }

        if (avformat_major < MIN_MAJOR) {
            LOGW("libavformat version %d.%d.%d is below minimum required %d.0.0",
                 avformat_major, avformat_minor, avformat_micro, MIN_MAJOR);
            return false;
        }

        if (swscale_major < MIN_MAJOR) {
            LOGW("libswscale version %d.%d.%d is below minimum required %d.0.0",
                 swscale_major, swscale_minor, swscale_micro, MIN_MAJOR);
            return false;
        }

        LOGI("FFmpeg version verification passed");
        return true;

    } catch (const std::exception& e) {
        LOGE("Exception during FFmpeg version verification: %s", e.what());
        return false;
    } catch (...) {
        LOGE("Unknown exception during FFmpeg version verification");
        return false;
    }
}

/**
 * JNI implementation for C++ video processing
 */

// Global references for callbacks
static jmethodID onVideoPreparedMethod = nullptr;
static jmethodID onVideoStartedMethod = nullptr;
static jmethodID onVideoPausedMethod = nullptr;
static jmethodID onVideoStoppedMethod = nullptr;
static jmethodID onVideoCompletedMethod = nullptr;
static jmethodID onVideoErrorMethod = nullptr;
static jmethodID onVideoProgressChangedMethod = nullptr;
static jmethodID onVideoBufferingStartedMethod = nullptr;
static jmethodID onVideoBufferingEndedMethod = nullptr;
static jmethodID onVideoSeekCompleteMethod = nullptr;
static jmethodID onVideoSizeChangedMethod = nullptr;

// Callback validation and retry mechanism
struct CallbackAttempt {
    std::string methodName;
    int attemptCount;
    clock_t lastAttempt;
};

static std::unordered_map<std::string, CallbackAttempt> callbackAttempts;
static const int MAX_CALLBACK_ATTEMPTS = 5;
static const int CALLBACK_RETRY_DELAY_MS = 500; // 500ms between retries

// Helper function to validate and retry callbacks
bool shouldRetryCallback(const std::string& methodName) {
    clock_t now = clock();
    auto& attempt = callbackAttempts[methodName];

    if (attempt.attemptCount >= MAX_CALLBACK_ATTEMPTS) {
        LOGE("Max callback attempts (%d) exceeded for %s", MAX_CALLBACK_ATTEMPTS, methodName.c_str());
        return false;
    }

    // Calculate time difference in milliseconds
    double timeSinceLastAttempt = (double)(now - attempt.lastAttempt) / CLOCKS_PER_SEC * 1000.0;
    if (timeSinceLastAttempt < CALLBACK_RETRY_DELAY_MS) {
        LOGW("Callback retry too soon for %s, waiting %dms more", methodName.c_str(),
             CALLBACK_RETRY_DELAY_MS - static_cast<int>(timeSinceLastAttempt));
        return false;
    }

    attempt.attemptCount++;
    attempt.lastAttempt = now;
    LOGI("Retrying callback %s (attempt %d/%d)", methodName.c_str(), attempt.attemptCount, MAX_CALLBACK_ATTEMPTS);
    return true;
}

// Helper function to reset callback attempts on success
void resetCallbackAttempts(const std::string& methodName) {
    auto it = callbackAttempts.find(methodName);
    if (it != callbackAttempts.end()) {
        callbackAttempts.erase(it);
        LOGI("Reset callback attempts for %s after successful call", methodName.c_str());
    }
}

// Helper function to convert jstring to std::string
std::string jstringToString(JNIEnv *env, jstring jstr) {
    if (!jstr) return "";
    
    const char *chars = env->GetStringUTFChars(jstr, nullptr);
    std::string result(chars);
    env->ReleaseStringUTFChars(jstr, chars);
    return result;
}

// Helper function to get callback object
jobject getCallbackObject(JNIEnv *env, jobject thiz) {
    jclass clazz = env->GetObjectClass(thiz);
    jfieldID callbackField = env->GetFieldID(clazz, "callback", "Lcom/dorflix/app/video/VideoPlayerListener;");
    return env->GetObjectField(thiz, callbackField);
}

// Video player JNI functions

JNIEXPORT jlong JNICALL
Java_com_dorflix_app_video_VideoPlayerController_nativeCreatePlayer(JNIEnv *env, jobject thiz) {
    LOGI("=== JNI: nativeCreatePlayer CALLED ===");
    LOGI("Creating new VideoDecoder instance...");

    try {
        auto player = std::make_unique<VideoDecoder>();
        jlong playerPtr = reinterpret_cast<jlong>(player.release());

        LOGI(" VideoDecoder created successfully, pointer: %p", (void*)playerPtr);
        LOGI("=== JNI: nativeCreatePlayer COMPLETED ===");

        return playerPtr;
    } catch (const std::exception& e) {
        LOGE("L Exception in nativeCreatePlayer: %s", e.what());
        return 0;
    } catch (...) {
        LOGE("L Unknown exception in nativeCreatePlayer");
        return 0;
    }
}

JNIEXPORT void JNICALL
Java_com_dorflix_app_video_VideoPlayerController_nativeDestroyPlayer(JNIEnv *env, jobject thiz, jlong player_ptr) {
    if (player_ptr != 0) {
        auto* player = reinterpret_cast<VideoDecoder*>(player_ptr);
        delete player;
    }
}

JNIEXPORT jboolean JNICALL
Java_com_dorflix_app_video_VideoPlayerController_nativeSetSurface(JNIEnv *env, jobject thiz, jlong player_ptr, jobject surface) {
    LOGI("=== JNI: nativeSetSurface CALLED ===");
    LOGI("Player pointer: %p, Surface object: %p", (void*)player_ptr, (void*)surface);

    if (player_ptr != 0) {
        auto* player = reinterpret_cast<VideoDecoder*>(player_ptr);

        // Handle nullable Surface parameter
        ANativeWindow* window = nullptr;
        if (surface != nullptr) {
            window = ANativeWindow_fromSurface(env, surface);
            LOGI(" ANativeWindow created from Surface: %p", (void*)window);
        } else {
            LOGW("� Surface object is null - setting null window");
        }

        player->setSurface(window);
        LOGI(" Surface set on VideoDecoder");
        return JNI_TRUE;
    }
    LOGE("L Invalid player pointer in nativeSetSurface");
    return JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_dorflix_app_video_VideoPlayerController_nativeLoadVideo(JNIEnv *env, jobject thiz, jlong player_ptr, jstring video_path) {
    if (player_ptr == 0 || video_path == nullptr) {
        LOGE("Invalid parameters in nativeLoadVideo: player_ptr=%p, video_path=%p",
             (void*)player_ptr, (void*)video_path);
        return JNI_FALSE;
    }

    try {
        auto* player = reinterpret_cast<VideoDecoder*>(player_ptr);
        std::string path = jstringToString(env, video_path);

        LOGI("=== VIDEO LOADING START ===");
        LOGI("Video path received: %s", path.c_str());
        LOGI("Player pointer: %p", (void*)player);

        if (path.empty()) {
            LOGE("Empty video path provided");
            return JNI_FALSE;
        }

        // Load video directly without blocking verification
        LOGI("Attempting to load video...");
        bool success = player->loadVideo(path);

        if (success) {
            LOGI("=== VIDEO LOADING SUCCESS ===");
            LOGI("Video loaded successfully: %s", path.c_str());

            // Log the player state after successful load
            LOGI("Player state after load: isPrepared=%d, duration=%d",
                 player->isPrepared(), player->getDuration());
        } else {
            LOGE("=== VIDEO LOADING FAILED ===");
            LOGE("Failed to load video: %s", path.c_str());

            // Log additional error information
            LOGI("Player state after failed load: isPrepared=%d, duration=%d",
                 player->isPrepared(), player->getDuration());
        }

        return success ? JNI_TRUE : JNI_FALSE;
    } catch (const std::exception& e) {
        LOGE("=== VIDEO LOADING EXCEPTION ===");
        LOGE("Exception in nativeLoadVideo: %s", e.what());
        return JNI_FALSE;
    } catch (...) {
        LOGE("=== VIDEO LOADING UNKNOWN EXCEPTION ===");
        LOGE("Unknown exception in nativeLoadVideo");
        return JNI_FALSE;
    }
}

// Thread-safe callback helper functions with retry mechanism
void callJavaVoidMethod(const char* methodName, jmethodID methodId) {
    if (!g_jvm || !g_callbackObject || !methodId) {
        LOGE("Cannot call Java method %s: JVM=%p, callbackObject=%p, methodId=%p",
             methodName, g_jvm, g_callbackObject, methodId);
        return;
    }

    std::string methodNameStr(methodName);
    bool success = false;

    JNIEnv* env = nullptr;
    bool attached = false;

    // Attach current thread to JVM if not already attached
    int attachResult = g_jvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
    if (attachResult == JNI_EDETACHED) {
        if (g_jvm->AttachCurrentThread(&env, nullptr) == JNI_OK) {
            attached = true;
        } else {
            LOGE("Failed to attach thread for callback %s", methodName);
            return;
        }
    } else if (attachResult != JNI_OK) {
        LOGE("Failed to get JNI environment for callback %s", methodName);
        return;
    }

    // Call the Java method with exception handling
    jthrowable exception = nullptr;
    env->CallVoidMethod(g_callbackObject, methodId);

    // Check for exceptions
    if (env->ExceptionCheck()) {
        exception = env->ExceptionOccurred();
        if (exception) {
            env->ExceptionDescribe();
            env->ExceptionClear();
            LOGE("Java exception occurred in callback %s", methodName);
        }
    } else {
        success = true;
        LOGI("Callback %s executed successfully", methodName);
    }

    // Detach thread if we attached it
    if (attached) {
        if (g_jvm->DetachCurrentThread() != JNI_OK) {
            LOGE("Failed to detach thread after callback %s", methodName);
        }
    }

    // Handle retry logic
    if (success) {
        resetCallbackAttempts(methodNameStr);
    } else if (shouldRetryCallback(methodNameStr)) {
        // Retry the callback
        LOGI("Retrying failed callback %s", methodName);
        callJavaVoidMethod(methodName, methodId);
    } else {
        LOGE("Callback %s failed after max retries", methodName);
    }
}

void callJavaIntMethod(const char* methodName, jmethodID methodId, int param) {
    if (!g_jvm || !g_callbackObject || !methodId) {
        LOGE("Cannot call Java method %s: JVM=%p, callbackObject=%p, methodId=%p",
             methodName, g_jvm, g_callbackObject, methodId);
        return;
    }

    std::string methodNameStr(methodName);
    bool success = false;

    JNIEnv* env = nullptr;
    bool attached = false;

    // Attach current thread to JVM if not already attached
    int attachResult = g_jvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
    if (attachResult == JNI_EDETACHED) {
        if (g_jvm->AttachCurrentThread(&env, nullptr) == JNI_OK) {
            attached = true;
        } else {
            LOGE("Failed to attach thread for callback %s", methodName);
            return;
        }
    } else if (attachResult != JNI_OK) {
        LOGE("Failed to get JNI environment for callback %s", methodName);
        return;
    }

    // Call the Java method with exception handling
    jthrowable exception = nullptr;
    env->CallVoidMethod(g_callbackObject, methodId, param);

    // Check for exceptions
    if (env->ExceptionCheck()) {
        exception = env->ExceptionOccurred();
        if (exception) {
            env->ExceptionDescribe();
            env->ExceptionClear();
            LOGE("Java exception occurred in callback %s with param %d", methodName, param);
        }
    } else {
        success = true;
        LOGI("Callback %s executed successfully", methodName);
    }

    // Detach thread if we attached it
    if (attached) {
        if (g_jvm->DetachCurrentThread() != JNI_OK) {
            LOGE("Failed to detach thread after callback %s", methodName);
        }
    }

    // Handle retry logic
    if (success) {
        resetCallbackAttempts(methodNameStr);
    } else if (shouldRetryCallback(methodNameStr)) {
        // Retry the callback
        LOGI("Retrying failed callback %s", methodName);
        callJavaIntMethod(methodName, methodId, param);
    } else {
        LOGE("Callback %s failed after max retries", methodName);
    }
}

void callJavaIntIntMethod(const char* methodName, jmethodID methodId, int param1, int param2) {
    if (!g_jvm || !g_callbackObject || !methodId) {
        LOGE("Cannot call Java method %s: JVM=%p, callbackObject=%p, methodId=%p",
             methodName, g_jvm, g_callbackObject, methodId);
        return;
    }

    std::string methodNameStr(methodName);
    bool success = false;

    JNIEnv* env = nullptr;
    bool attached = false;

    // Attach current thread to JVM if not already attached
    int attachResult = g_jvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
    if (attachResult == JNI_EDETACHED) {
        if (g_jvm->AttachCurrentThread(&env, nullptr) == JNI_OK) {
            attached = true;
        } else {
            LOGE("Failed to attach thread for callback %s", methodName);
            return;
        }
    } else if (attachResult != JNI_OK) {
        LOGE("Failed to get JNI environment for callback %s", methodName);
        return;
    }

    // Call the Java method with exception handling
    jthrowable exception = nullptr;
    env->CallVoidMethod(g_callbackObject, methodId, param1, param2);

    // Check for exceptions
    if (env->ExceptionCheck()) {
        exception = env->ExceptionOccurred();
        if (exception) {
            env->ExceptionDescribe();
            env->ExceptionClear();
            LOGE("Java exception occurred in callback %s with params %d, %d", methodName, param1, param2);
        }
    } else {
        success = true;
        LOGI("Callback %s executed successfully", methodName);
    }

    // Detach thread if we attached it
    if (attached) {
        if (g_jvm->DetachCurrentThread() != JNI_OK) {
            LOGE("Failed to detach thread after callback %s", methodName);
        }
    }

    // Handle retry logic
    if (success) {
        resetCallbackAttempts(methodNameStr);
    } else if (shouldRetryCallback(methodNameStr)) {
        // Retry the callback
        LOGI("Retrying failed callback %s", methodName);
        callJavaIntIntMethod(methodName, methodId, param1, param2);
    } else {
        LOGE("Callback %s failed after max retries", methodName);
    }
}

void callJavaErrorMethod(const char* methodName, jmethodID methodId, int errorCode, const std::string& message) {
    if (!g_jvm || !g_callbackObject || !methodId) {
        LOGE("Cannot call Java method %s: JVM=%p, callbackObject=%p, methodId=%p",
             methodName, g_jvm, g_callbackObject, methodId);
        return;
    }

    std::string methodNameStr(methodName);
    bool success = false;

    JNIEnv* env = nullptr;
    bool attached = false;

    // Attach current thread to JVM if not already attached
    int attachResult = g_jvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
    if (attachResult == JNI_EDETACHED) {
        if (g_jvm->AttachCurrentThread(&env, nullptr) == JNI_OK) {
            attached = true;
        } else {
            LOGE("Failed to attach thread for callback %s", methodName);
            return;
        }
    } else if (attachResult != JNI_OK) {
        LOGE("Failed to get JNI environment for callback %s", methodName);
        return;
    }

    // Create Java string for the message with error handling
    jstring jMessage = nullptr;
    try {
        jMessage = env->NewStringUTF(message.c_str());
        if (!jMessage) {
            LOGE("Failed to create Java string for error message in callback %s", methodName);
            return;
        }

        // Call the Java method with exception handling
        jthrowable exception = nullptr;
        env->CallVoidMethod(g_callbackObject, methodId, errorCode, jMessage);

        // Check for exceptions
        if (env->ExceptionCheck()) {
            exception = env->ExceptionOccurred();
            if (exception) {
                env->ExceptionDescribe();
                env->ExceptionClear();
                LOGE("Java exception occurred in error callback %s", methodName);
            }
        } else {
            success = true;
            LOGI("Error callback %s executed successfully", methodName);
        }

        env->DeleteLocalRef(jMessage);
    } catch (const std::exception& e) {
        LOGE("Exception in error callback %s: %s", methodName, e.what());
        if (jMessage) {
            env->DeleteLocalRef(jMessage);
        }
    } catch (...) {
        LOGE("Unknown exception in error callback %s", methodName);
        if (jMessage) {
            env->DeleteLocalRef(jMessage);
        }
    }

    // Detach thread if we attached it
    if (attached) {
        if (g_jvm->DetachCurrentThread() != JNI_OK) {
            LOGE("Failed to detach thread after error callback %s", methodName);
        }
    }

    // Handle retry logic
    if (success) {
        resetCallbackAttempts(methodNameStr);
    } else if (shouldRetryCallback(methodNameStr)) {
        // Retry the callback
        LOGI("Retrying failed error callback %s", methodName);
        callJavaErrorMethod(methodName, methodId, errorCode, message);
    } else {
        LOGE("Error callback %s failed after max retries", methodName);
    }
}

JNIEXPORT void JNICALL
Java_com_dorflix_app_video_VideoPlayerController_nativeSetupCallbacks(JNIEnv *env, jobject thiz, jlong player_ptr) {
    if (player_ptr == 0) {
        LOGE("Invalid player pointer in nativeSetupCallbacks");
        return;
    }

    if (!g_jvm) {
        LOGE("JVM not available for callback setup");
        return;
    }

    try {
        auto* player = reinterpret_cast<VideoDecoder*>(player_ptr);

        LOGI("=== SETTING UP C++ TO JAVA CALLBACKS ===");

        // Clean up any existing global reference
        if (g_callbackObject) {
            env->DeleteGlobalRef(g_callbackObject);
            g_callbackObject = nullptr;
        }

        // Get the callback object (VideoPlayerListener)
        jobject callbackObject = getCallbackObject(env, thiz);
        if (!callbackObject) {
            LOGE("Failed to get callback object");
            return;
        }

        // Create global reference to prevent garbage collection
        g_callbackObject = env->NewGlobalRef(callbackObject);
        if (!g_callbackObject) {
            LOGE("Failed to create global reference for callback object");
            return;
        }

        // Get the class of the callback object
        jclass callbackClass = env->GetObjectClass(g_callbackObject);
        if (!callbackClass) {
            LOGE("Failed to get callback class");
            env->DeleteGlobalRef(g_callbackObject);
            g_callbackObject = nullptr;
            return;
        }

        // Cache method IDs for better performance
        if (!onVideoPreparedMethod) {
            onVideoPreparedMethod = env->GetMethodID(callbackClass, "onVideoPrepared", "(I)V");
            LOGI("Cached onVideoPrepared method ID: %p", onVideoPreparedMethod);
        }
        if (!onVideoStartedMethod) {
            onVideoStartedMethod = env->GetMethodID(callbackClass, "onVideoStarted", "()V");
            LOGI("Cached onVideoStarted method ID: %p", onVideoStartedMethod);
        }
        if (!onVideoPausedMethod) {
            onVideoPausedMethod = env->GetMethodID(callbackClass, "onVideoPaused", "()V");
            LOGI("Cached onVideoPaused method ID: %p", onVideoPausedMethod);
        }
        if (!onVideoStoppedMethod) {
            onVideoStoppedMethod = env->GetMethodID(callbackClass, "onVideoStopped", "()V");
            LOGI("Cached onVideoStopped method ID: %p", onVideoStoppedMethod);
        }
        if (!onVideoCompletedMethod) {
            onVideoCompletedMethod = env->GetMethodID(callbackClass, "onVideoCompleted", "()V");
            LOGI("Cached onVideoCompleted method ID: %p", onVideoCompletedMethod);
        }
        if (!onVideoErrorMethod) {
            onVideoErrorMethod = env->GetMethodID(callbackClass, "onVideoError", "(ILjava/lang/String;)V");
            LOGI("Cached onVideoError method ID: %p", onVideoErrorMethod);
        }
        if (!onVideoProgressChangedMethod) {
            onVideoProgressChangedMethod = env->GetMethodID(callbackClass, "onVideoProgressChanged", "(I)V");
            LOGI("Cached onVideoProgressChanged method ID: %p", onVideoProgressChangedMethod);
        }
        if (!onVideoBufferingStartedMethod) {
            onVideoBufferingStartedMethod = env->GetMethodID(callbackClass, "onVideoBufferingStarted", "()V");
            LOGI("Cached onVideoBufferingStarted method ID: %p", onVideoBufferingStartedMethod);
        }
        if (!onVideoBufferingEndedMethod) {
            onVideoBufferingEndedMethod = env->GetMethodID(callbackClass, "onVideoBufferingEnded", "()V");
            LOGI("Cached onVideoBufferingEnded method ID: %p", onVideoBufferingEndedMethod);
        }
        if (!onVideoSeekCompleteMethod) {
            onVideoSeekCompleteMethod = env->GetMethodID(callbackClass, "onVideoSeekComplete", "()V");
            LOGI("Cached onVideoSeekComplete method ID: %p", onVideoSeekCompleteMethod);
        }
        if (!onVideoSizeChangedMethod) {
            onVideoSizeChangedMethod = env->GetMethodID(callbackClass, "onVideoSizeChanged", "(II)V");
            LOGI("Cached onVideoSizeChanged method ID: %p", onVideoSizeChangedMethod);
        }

        // Set up thread-safe C++ callbacks that properly attach to JVM
        player->setVideoPreparedCallback([](int duration) {
            callJavaIntMethod("onVideoPrepared", onVideoPreparedMethod, duration);
        });

        player->setVideoStartedCallback([]() {
            callJavaVoidMethod("onVideoStarted", onVideoStartedMethod);
        });

        player->setVideoPausedCallback([]() {
            callJavaVoidMethod("onVideoPaused", onVideoPausedMethod);
        });

        player->setVideoStoppedCallback([]() {
            callJavaVoidMethod("onVideoStopped", onVideoStoppedMethod);
        });

        player->setVideoCompletedCallback([]() {
            callJavaVoidMethod("onVideoCompleted", onVideoCompletedMethod);
        });

        player->setVideoErrorCallback([](int errorCode, const std::string& message) {
            callJavaErrorMethod("onVideoError", onVideoErrorMethod, errorCode, message);
        });

        player->setVideoProgressCallback([](int position) {
            callJavaIntMethod("onVideoProgressChanged", onVideoProgressChangedMethod, position);
        });

        player->setVideoBufferingStartedCallback([]() {
            callJavaVoidMethod("onVideoBufferingStarted", onVideoBufferingStartedMethod);
        });

        player->setVideoBufferingEndedCallback([]() {
            callJavaVoidMethod("onVideoBufferingEnded", onVideoBufferingEndedMethod);
        });

        player->setVideoSeekCompleteCallback([]() {
            callJavaVoidMethod("onVideoSeekComplete", onVideoSeekCompleteMethod);
        });

        player->setVideoSizeChangedCallback([](int width, int height) {
            callJavaIntIntMethod("onVideoSizeChanged", onVideoSizeChangedMethod, width, height);
        });

        LOGI(" Successfully set up all thread-safe C++ to Java callbacks");

    } catch (const std::exception& e) {
        LOGE("Exception in nativeSetupCallbacks: %s", e.what());
    } catch (...) {
        LOGE("Unknown exception in nativeSetupCallbacks");
    }
}

JNIEXPORT void JNICALL
Java_com_dorflix_app_video_VideoPlayerController_nativePlay(JNIEnv *env, jobject thiz, jlong player_ptr) {
    LOGI("=== JNI: nativePlay CALLED ===");
    LOGI("Player pointer: %p", (void*)player_ptr);

    if (player_ptr != 0) {
        auto* player = reinterpret_cast<VideoDecoder*>(player_ptr);
        LOGI("Calling VideoDecoder::play()...");
        player->play();
        LOGI(" VideoDecoder::play() completed");
    } else {
        LOGE("L Invalid player pointer in nativePlay");
    }
    LOGI("=== JNI: nativePlay COMPLETED ===");
}

JNIEXPORT void JNICALL
Java_com_dorflix_app_video_VideoPlayerController_nativePause(JNIEnv *env, jobject thiz, jlong player_ptr) {
    if (player_ptr != 0) {
        auto* player = reinterpret_cast<VideoDecoder*>(player_ptr);
        player->pause();
    }
}

JNIEXPORT void JNICALL
Java_com_dorflix_app_video_VideoPlayerController_nativeStop(JNIEnv *env, jobject thiz, jlong player_ptr) {
    if (player_ptr != 0) {
        auto* player = reinterpret_cast<VideoDecoder*>(player_ptr);
        player->stop();
    }
}

JNIEXPORT void JNICALL
Java_com_dorflix_app_video_VideoPlayerController_nativeSeekTo(JNIEnv *env, jobject thiz, jlong player_ptr, jint position) {
    if (player_ptr != 0) {
        auto* player = reinterpret_cast<VideoDecoder*>(player_ptr);
        player->seekTo(position);
    }
}

JNIEXPORT jint JNICALL
Java_com_dorflix_app_video_VideoPlayerController_nativeGetCurrentPosition(JNIEnv *env, jobject thiz, jlong player_ptr) {
    if (player_ptr != 0) {
        auto* player = reinterpret_cast<VideoDecoder*>(player_ptr);
        return player->getCurrentPosition();
    }
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_dorflix_app_video_VideoPlayerController_nativeGetDuration(JNIEnv *env, jobject thiz, jlong player_ptr) {
    if (player_ptr != 0) {
        auto* player = reinterpret_cast<VideoDecoder*>(player_ptr);
        return player->getDuration();
    }
    return 0;
}

JNIEXPORT jboolean JNICALL
Java_com_dorflix_app_video_VideoPlayerController_nativeIsPlaying(JNIEnv *env, jobject thiz, jlong player_ptr) {
    if (player_ptr != 0) {
        auto* player = reinterpret_cast<VideoDecoder*>(player_ptr);
        return player->isPlaying();
    }
    return JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_dorflix_app_video_VideoPlayerController_nativeSetVolume(JNIEnv *env, jobject thiz, jlong player_ptr, jfloat volume) {
    if (player_ptr != 0) {
        auto* player = reinterpret_cast<VideoDecoder*>(player_ptr);
        player->setVolume(volume);
    }
}

JNIEXPORT void JNICALL
Java_com_dorflix_app_video_VideoPlayerController_nativeSetMute(JNIEnv *env, jobject thiz, jlong player_ptr, jboolean mute) {
    if (player_ptr != 0) {
        auto* player = reinterpret_cast<VideoDecoder*>(player_ptr);
        player->setMute(mute);
    }
}

// Preloader JNI functions

JNIEXPORT jlong JNICALL
Java_com_dorflix_app_video_VideoPreloader_nativeCreatePreloader(JNIEnv *env, jobject thiz) {
    try {
        auto preloader = std::make_unique<Preloader>();
        return reinterpret_cast<jlong>(preloader.release());
    } catch (const std::exception& e) {
        LOGE("Exception in nativeCreatePreloader: %s", e.what());
        return 0;
    }
}

JNIEXPORT void JNICALL
Java_com_dorflix_app_video_VideoPreloader_nativeDestroyPreloader(JNIEnv *env, jobject thiz, jlong preloader_ptr) {
    if (preloader_ptr != 0) {
        auto* preloader = reinterpret_cast<Preloader*>(preloader_ptr);
        delete preloader;
    }
}

JNIEXPORT void JNICALL
Java_com_dorflix_app_video_VideoPreloader_nativePreloadVideo(JNIEnv *env, jobject thiz, jlong preloader_ptr, jstring video_path) {
    if (preloader_ptr != 0) {
        auto* preloader = reinterpret_cast<Preloader*>(preloader_ptr);
        std::string path = jstringToString(env, video_path);
        preloader->preloadVideo(path);
    }
}

// Memory management JNI functions

JNIEXPORT jlong JNICALL
Java_com_dorflix_app_video_MemoryManager_nativeCreateMemoryPool(JNIEnv *env, jobject thiz, jint pool_size) {
    try {
        auto pool = std::make_unique<MemoryPool>(pool_size);
        return reinterpret_cast<jlong>(pool.release());
    } catch (const std::exception& e) {
        LOGE("Exception in nativeCreateMemoryPool: %s", e.what());
        return 0;
    }
}

JNIEXPORT void JNICALL
Java_com_dorflix_app_video_MemoryManager_nativeDestroyMemoryPool(JNIEnv *env, jobject thiz, jlong pool_ptr) {
    if (pool_ptr != 0) {
        auto* pool = reinterpret_cast<MemoryPool*>(pool_ptr);
        delete pool;
    }
}

JNIEXPORT jlong JNICALL
Java_com_dorflix_app_video_MemoryManager_nativeAllocate(JNIEnv *env, jobject thiz, jlong pool_ptr, jint size) {
    if (pool_ptr != 0) {
        auto* pool = reinterpret_cast<MemoryPool*>(pool_ptr);
        void* ptr = pool->allocate(size);
        return reinterpret_cast<jlong>(ptr);
    }
    return 0;
}

JNIEXPORT void JNICALL
Java_com_dorflix_app_video_MemoryManager_nativeFree(JNIEnv *env, jobject thiz, jlong pool_ptr, jlong ptr) {
    if (pool_ptr != 0 && ptr != 0) {
        auto* pool = reinterpret_cast<MemoryPool*>(pool_ptr);
        pool->deallocate(reinterpret_cast<void*>(ptr));
    }
}

JNIEXPORT jint JNICALL
Java_com_dorflix_app_video_MemoryManager_nativeGetPoolSize(JNIEnv *env, jobject thiz, jlong pool_ptr) {
    if (pool_ptr != 0) {
        auto* pool = reinterpret_cast<MemoryPool*>(pool_ptr);
        return pool->getPoolSize();
    }
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_dorflix_app_video_MemoryManager_nativeGetUsedSize(JNIEnv *env, jobject thiz, jlong pool_ptr) {
    if (pool_ptr != 0) {
        auto* pool = reinterpret_cast<MemoryPool*>(pool_ptr);
        return pool->getUsedSize();
    }
    return 0;
}

// VideoCacheManager JNI functions

static VideoCacheManager* g_cacheManager = nullptr;

JNIEXPORT void JNICALL
Java_com_dorflix_app_video_VideoDownloader_nativeInitializeCache(JNIEnv *env, jclass clazz, jstring cache_dir) {
    if (g_cacheManager) {
        delete g_cacheManager;
    }

    std::string cacheDir = jstringToString(env, cache_dir);
    g_cacheManager = new VideoCacheManager();

    if (g_cacheManager->initialize(cacheDir)) {
        LOGI("VideoCacheManager initialized successfully");
    } else {
        LOGE("Failed to initialize VideoCacheManager");
    }
}

JNIEXPORT jstring JNICALL
Java_com_dorflix_app_video_VideoDownloader_nativeDownloadVideo(JNIEnv *env, jclass clazz, jstring url) {
    if (!g_cacheManager) {
        LOGE("VideoCacheManager not initialized");
        return env->NewStringUTF("");
    }

    std::string urlStr = jstringToString(env, url);
    std::string cachedPath = g_cacheManager->downloadVideo(urlStr);

    return env->NewStringUTF(cachedPath.c_str());
}

JNIEXPORT jboolean JNICALL
Java_com_dorflix_app_video_VideoDownloader_nativeIsVideoCached(JNIEnv *env, jclass clazz, jstring url) {
    if (!g_cacheManager) {
        return JNI_FALSE;
    }

    std::string urlStr = jstringToString(env, url);
    return g_cacheManager->isCached(urlStr) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jstring JNICALL
Java_com_dorflix_app_video_VideoDownloader_nativeGetCachedPath(JNIEnv *env, jclass clazz, jstring url) {
    if (!g_cacheManager) {
        return env->NewStringUTF("");
    }

    std::string urlStr = jstringToString(env, url);
    std::string cachedPath = g_cacheManager->getCachedPath(urlStr);

    return env->NewStringUTF(cachedPath.c_str());
}

JNIEXPORT void JNICALL
Java_com_dorflix_app_video_VideoDownloader_nativePreloadVideo(JNIEnv *env, jclass clazz, jstring url) {
    if (!g_cacheManager) {
        LOGE("VideoCacheManager not initialized for preload");
        return;
    }

    std::string urlStr = jstringToString(env, url);
    g_cacheManager->preloadVideo(urlStr);
}

JNIEXPORT void JNICALL
Java_com_dorflix_app_video_VideoDownloader_nativeCleanupCache(JNIEnv *env, jclass clazz) {
    if (g_cacheManager) {
        g_cacheManager->cleanupCache();
    }
}

JNIEXPORT jlong JNICALL
Java_com_dorflix_app_video_VideoDownloader_nativeGetCacheSize(JNIEnv *env, jclass clazz) {
    if (!g_cacheManager) {
        return 0L;
    }

    return g_cacheManager->getCacheSize();
}



// JNI callback implementations (called from C++)
JNIEXPORT void JNICALL
Java_com_dorflix_app_video_VideoDownloader_onDownloadProgress(JNIEnv *env, jclass clazz, jstring url, jlong downloaded, jlong total) {
    // This is called from C++ VideoCacheManager
    LOGI("Download progress callback: %lld/%lld", (long long)downloaded, (long long)total);
}

JNIEXPORT void JNICALL
Java_com_dorflix_app_video_VideoDownloader_onDownloadComplete(JNIEnv *env, jclass clazz, jstring url, jstring localPath) {
    // This is called from C++ VideoCacheManager
    LOGI("Download complete callback");
}

JNIEXPORT void JNICALL
Java_com_dorflix_app_video_VideoDownloader_onDownloadError(JNIEnv *env, jclass clazz, jstring url, jstring error) {
    // This is called from C++ VideoCacheManager
    LOGI("Download error callback");
}

// JNI readiness check method
JNIEXPORT jboolean JNICALL
Java_com_dorflix_app_DorflixApplication_isJNILibraryReady(JNIEnv *env, jclass clazz) {
    return g_jniReady ? JNI_TRUE : JNI_FALSE;
}





// ===== MEDIACODEC ENUMERATION BRIDGE IMPLEMENTATIONS =====

// JNI implementations for CodecEnumerator data storage
JNIEXPORT void JNICALL
Java_com_dorflix_app_CodecEnumerator_storeCodecInfo(
    JNIEnv* env, jclass clazz,
    jstring name, jstring mimeType,
    jboolean isEncoder, jboolean isHardware,
    jint maxWidth, jint maxHeight,
    jint profile, jint level,
    jobjectArray hdrSupport, jobjectArray colorFormats,
    jint maxBitrate) {

    JNI_LOG_ENTRY("storeCodecInfo");

    long long startTime = clock();

    // Input parameter validation
    if (name == nullptr) {
        JNI_LOG_ERROR("storeCodecInfo", "name parameter is null");
        JNI_LOG_EXIT("storeCodecInfo", false);
        return;
    }

    if (mimeType == nullptr) {
        JNI_LOG_ERROR("storeCodecInfo", "mimeType parameter is null");
        JNI_LOG_EXIT("storeCodecInfo", false);
        return;
    }

    // Check for pending exceptions before proceeding
    if (env->ExceptionCheck()) {
        JNI_LOG_ERROR("storeCodecInfo", "Exception pending before string conversion");
        env->ExceptionDescribe();
        env->ExceptionClear();
        JNI_LOG_EXIT("storeCodecInfo", false);
        return;
    }

    // Convert Java strings to C++ strings with error checking
    const char* nameChars = nullptr;
    const char* mimeChars = nullptr;

    // Convert name string
    nameChars = env->GetStringUTFChars(name, nullptr);
    if (nameChars == nullptr) {
        JNI_LOG_ERROR("storeCodecInfo", "Failed to convert name string - GetStringUTFChars returned null");

        // Check if an exception was thrown
        if (env->ExceptionCheck()) {
            JNI_LOG_ERROR("storeCodecInfo", "Exception occurred during name string conversion");
            env->ExceptionDescribe();
            env->ExceptionClear();
        }

        JNI_LOG_EXIT("storeCodecInfo", false);
        return;
    }

    // Check for exceptions after name conversion
    if (env->ExceptionCheck()) {
        JNI_LOG_ERROR("storeCodecInfo", "Exception occurred during name string conversion");
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->ReleaseStringUTFChars(name, nameChars);
        JNI_LOG_EXIT("storeCodecInfo", false);
        return;
    }

    // Convert mimeType string
    mimeChars = env->GetStringUTFChars(mimeType, nullptr);
    if (mimeChars == nullptr) {
        JNI_LOG_ERROR("storeCodecInfo", "Failed to convert mimeType string - GetStringUTFChars returned null");

        // Clean up nameChars before returning
        env->ReleaseStringUTFChars(name, nameChars);

        // Check if an exception was thrown
        if (env->ExceptionCheck()) {
            JNI_LOG_ERROR("storeCodecInfo", "Exception occurred during mimeType string conversion");
            env->ExceptionDescribe();
            env->ExceptionClear();
        }

        JNI_LOG_EXIT("storeCodecInfo", false);
        return;
    }

    // Check for exceptions after mimeType conversion
    if (env->ExceptionCheck()) {
        JNI_LOG_ERROR("storeCodecInfo", "Exception occurred during mimeType string conversion");
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->ReleaseStringUTFChars(name, nameChars);
        env->ReleaseStringUTFChars(mimeType, mimeChars);
        JNI_LOG_EXIT("storeCodecInfo", false);
        return;
    }

    // Convert HDR support array
    std::vector<std::string> hdrSupportVector;
    if (hdrSupport != nullptr) {
        jsize hdrArrayLength = env->GetArrayLength(hdrSupport);
        for (jsize i = 0; i < hdrArrayLength; i++) {
            jstring hdrString = (jstring)env->GetObjectArrayElement(hdrSupport, i);
            if (hdrString != nullptr) {
                const char* hdrChars = env->GetStringUTFChars(hdrString, nullptr);
                if (hdrChars != nullptr) {
                    hdrSupportVector.push_back(std::string(hdrChars));
                    env->ReleaseStringUTFChars(hdrString, hdrChars);
                } else {
                    // Check for exceptions if GetStringUTFChars failed
                    if (env->ExceptionCheck()) {
                        JNI_LOG_ERROR("storeCodecInfo", "Exception occurred during HDR string conversion");
                        env->ExceptionDescribe();
                        env->ExceptionClear();
                    }
                    // Skip this element if conversion failed
                    JNI_LOG_ERROR("storeCodecInfo", "Failed to convert HDR string element - skipping");
                }
                env->DeleteLocalRef(hdrString);
            }
        }
    }

    // Convert color formats array
    std::vector<std::string> colorFormatsVector;
    if (colorFormats != nullptr) {
        jsize colorArrayLength = env->GetArrayLength(colorFormats);
        for (jsize i = 0; i < colorArrayLength; i++) {
            jstring colorString = (jstring)env->GetObjectArrayElement(colorFormats, i);
            if (colorString != nullptr) {
                const char* colorChars = env->GetStringUTFChars(colorString, nullptr);
                if (colorChars != nullptr) {
                    colorFormatsVector.push_back(std::string(colorChars));
                    env->ReleaseStringUTFChars(colorString, colorChars);
                } else {
                    // Check for exceptions if GetStringUTFChars failed
                    if (env->ExceptionCheck()) {
                        JNI_LOG_ERROR("storeCodecInfo", "Exception occurred during color format string conversion");
                        env->ExceptionDescribe();
                        env->ExceptionClear();
                    }
                    // Skip this element if conversion failed
                    JNI_LOG_ERROR("storeCodecInfo", "Failed to convert color format string element - skipping");
                }
                env->DeleteLocalRef(colorString);
            }
        }
    }

    // Thread-safe storage of codec information
    {
        std::lock_guard<std::mutex> lock(g_enumerationMutex);
        EnumeratedCodecInfo codecInfo;
        codecInfo.name = nameChars;
        codecInfo.mimeType = mimeChars;
        codecInfo.isEncoder = (bool)isEncoder;
        codecInfo.isHardware = (bool)isHardware;
        codecInfo.maxWidth = (int)maxWidth;
        codecInfo.maxHeight = (int)maxHeight;
        codecInfo.profile = (int)profile;
        codecInfo.level = (int)level;
        codecInfo.isValid = true;
        codecInfo.performanceScore = -1; // Not measured during enumeration
        codecInfo.hdrSupport = hdrSupportVector;
        codecInfo.colorFormats = colorFormatsVector;
        codecInfo.maxBitrate = (int)maxBitrate;

        g_enumeratedCodecs.emplace_back(codecInfo);
    }

    LOGI("=� Codec stored: %s (%s) HW=%d Res=%dx%d Profile=%d Level=%d HDR=%s Colors=%s Bitrate=%d",
         nameChars, mimeChars, (bool)isHardware, (int)maxWidth, (int)maxHeight, (int)profile, (int)level,
         hdrSupportVector.empty() ? "none" : hdrSupportVector[0].c_str(),
         colorFormatsVector.empty() ? "none" : colorFormatsVector[0].c_str(),
         (int)maxBitrate);

    JNI_LOG_MEMORY("codec_storage", (void*)&g_enumeratedCodecs, sizeof(EnumeratedCodecInfo));

    // Clean up with exception checking
    if (nameChars) {
        env->ReleaseStringUTFChars(name, nameChars);
        if (env->ExceptionCheck()) {
            JNI_LOG_ERROR("storeCodecInfo", "Exception occurred during name string cleanup");
            env->ExceptionDescribe();
            env->ExceptionClear();
        }
    }

    if (mimeChars) {
        env->ReleaseStringUTFChars(mimeType, mimeChars);
        if (env->ExceptionCheck()) {
            JNI_LOG_ERROR("storeCodecInfo", "Exception occurred during mimeType string cleanup");
            env->ExceptionDescribe();
            env->ExceptionClear();
        }
    }

    JNI_LOG_PERF("CodecStorage", startTime);
    JNI_LOG_EXIT("storeCodecInfo", true);
}

JNIEXPORT void JNICALL
Java_com_dorflix_app_CodecEnumerator_signalEnumerationComplete(JNIEnv* env, jclass clazz, jint totalCodecs) {
    {
        std::lock_guard<std::mutex> lock(g_enumerationMutex);
        g_enumerationComplete = true;
    }
    LOGI("<� MediaCodec enumeration complete: %d codecs found", (int)totalCodecs);
}

// Function to get enumerated codecs (called from C++) - thread-safe
std::vector<EnumeratedCodecInfo> getEnumeratedCodecs() {
    std::lock_guard<std::mutex> lock(g_enumerationMutex);
    return g_enumeratedCodecs;
}

bool isEnumerationComplete() {
    std::lock_guard<std::mutex> lock(g_enumerationMutex);
    return g_enumerationComplete;
}

void resetCodecEnumeration() {
    std::lock_guard<std::mutex> lock(g_enumerationMutex);
    g_enumeratedCodecs.clear();
    g_enumerationComplete = false;
}

// Forward declarations for VideoDownloader functions
JNIEXPORT void JNICALL
Java_com_dorflix_app_video_VideoDownloader_nativeInitializeCache(JNIEnv *env, jclass clazz, jstring cache_dir);

JNIEXPORT jstring JNICALL
Java_com_dorflix_app_video_VideoDownloader_nativeDownloadVideo(JNIEnv *env, jclass clazz, jstring url);

JNIEXPORT jboolean JNICALL
Java_com_dorflix_app_video_VideoDownloader_nativeIsVideoCached(JNIEnv *env, jclass clazz, jstring url);

JNIEXPORT jstring JNICALL
Java_com_dorflix_app_video_VideoDownloader_nativeGetCachedPath(JNIEnv *env, jclass clazz, jstring url);

JNIEXPORT void JNICALL
Java_com_dorflix_app_video_VideoDownloader_nativePreloadVideo(JNIEnv *env, jclass clazz, jstring url);

JNIEXPORT void JNICALL
Java_com_dorflix_app_video_VideoDownloader_nativeCleanupCache(JNIEnv *env, jclass clazz);

JNIEXPORT jlong JNICALL
Java_com_dorflix_app_video_VideoDownloader_nativeGetCacheSize(JNIEnv *env, jclass clazz);

JNIEXPORT void JNICALL
Java_com_dorflix_app_video_VideoDownloader_onDownloadProgress(JNIEnv *env, jclass clazz, jstring url, jlong downloaded, jlong total);

JNIEXPORT void JNICALL
Java_com_dorflix_app_video_VideoDownloader_onDownloadComplete(JNIEnv *env, jclass clazz, jstring url, jstring localPath);

JNIEXPORT void JNICALL
Java_com_dorflix_app_video_VideoDownloader_onDownloadError(JNIEnv *env, jclass clazz, jstring url, jstring error);

// JNI registration
static const JNINativeMethod videoPlayerMethods[] = {
    {"nativeCreatePlayer", "()J", (void*)Java_com_dorflix_app_video_VideoPlayerController_nativeCreatePlayer},
    {"nativeDestroyPlayer", "(J)V", (void*)Java_com_dorflix_app_video_VideoPlayerController_nativeDestroyPlayer},
    {"nativeSetSurface", "(JLandroid/view/Surface;)Z", (void*)Java_com_dorflix_app_video_VideoPlayerController_nativeSetSurface},
    {"nativeLoadVideo", "(JLjava/lang/String;)Z", (void*)Java_com_dorflix_app_video_VideoPlayerController_nativeLoadVideo},
    {"nativeSetupCallbacks", "(J)V", (void*)Java_com_dorflix_app_video_VideoPlayerController_nativeSetupCallbacks},
    {"nativePlay", "(J)V", (void*)Java_com_dorflix_app_video_VideoPlayerController_nativePlay},
    {"nativePause", "(J)V", (void*)Java_com_dorflix_app_video_VideoPlayerController_nativePause},
    {"nativeStop", "(J)V", (void*)Java_com_dorflix_app_video_VideoPlayerController_nativeStop},
    {"nativeSeekTo", "(JI)V", (void*)Java_com_dorflix_app_video_VideoPlayerController_nativeSeekTo},
    {"nativeGetCurrentPosition", "(J)I", (void*)Java_com_dorflix_app_video_VideoPlayerController_nativeGetCurrentPosition},
    {"nativeGetDuration", "(J)I", (void*)Java_com_dorflix_app_video_VideoPlayerController_nativeGetDuration},
    {"nativeIsPlaying", "(J)Z", (void*)Java_com_dorflix_app_video_VideoPlayerController_nativeIsPlaying},
    {"nativeSetVolume", "(JF)V", (void*)Java_com_dorflix_app_video_VideoPlayerController_nativeSetVolume},
    {"nativeSetMute", "(JZ)V", (void*)Java_com_dorflix_app_video_VideoPlayerController_nativeSetMute},
};

static const JNINativeMethod preloaderMethods[] = {
    {"nativeCreatePreloader", "()J", (void*)Java_com_dorflix_app_video_VideoPreloader_nativeCreatePreloader},
    {"nativeDestroyPreloader", "(J)V", (void*)Java_com_dorflix_app_video_VideoPreloader_nativeDestroyPreloader},
    {"nativePreloadVideo", "(JLjava/lang/String;)V", (void*)Java_com_dorflix_app_video_VideoPreloader_nativePreloadVideo},
};

static const JNINativeMethod memoryManagerMethods[] = {
    {"nativeCreateMemoryPool", "(I)J", (void*)Java_com_dorflix_app_video_MemoryManager_nativeCreateMemoryPool},
    {"nativeDestroyMemoryPool", "(J)V", (void*)Java_com_dorflix_app_video_MemoryManager_nativeDestroyMemoryPool},
    {"nativeAllocate", "(JI)J", (void*)Java_com_dorflix_app_video_MemoryManager_nativeAllocate},
    {"nativeFree", "(JJ)V", (void*)Java_com_dorflix_app_video_MemoryManager_nativeFree},
    {"nativeGetPoolSize", "(J)I", (void*)Java_com_dorflix_app_video_MemoryManager_nativeGetPoolSize},
    {"nativeGetUsedSize", "(J)I", (void*)Java_com_dorflix_app_video_MemoryManager_nativeGetUsedSize},
};

static const JNINativeMethod codecEnumeratorMethods[] = {
    {"storeCodecInfo", "(Ljava/lang/String;Ljava/lang/String;ZZIIII[Ljava/lang/String;[Ljava/lang/String;I)V", (void*)Java_com_dorflix_app_CodecEnumerator_storeCodecInfo},
    {"signalEnumerationComplete", "(I)V", (void*)Java_com_dorflix_app_CodecEnumerator_signalEnumerationComplete},
};

static const JNINativeMethod videoDownloaderMethods[] = {
    {"nativeInitializeCache", "(Ljava/lang/String;)V", (void*)Java_com_dorflix_app_video_VideoDownloader_nativeInitializeCache},
    {"nativeDownloadVideo", "(Ljava/lang/String;)Ljava/lang/String;", (void*)Java_com_dorflix_app_video_VideoDownloader_nativeDownloadVideo},
    {"nativeIsVideoCached", "(Ljava/lang/String;)Z", (void*)Java_com_dorflix_app_video_VideoDownloader_nativeIsVideoCached},
    {"nativeGetCachedPath", "(Ljava/lang/String;)Ljava/lang/String;", (void*)Java_com_dorflix_app_video_VideoDownloader_nativeGetCachedPath},
    {"nativePreloadVideo", "(Ljava/lang/String;)V", (void*)Java_com_dorflix_app_video_VideoDownloader_nativePreloadVideo},
    {"nativeCleanupCache", "()V", (void*)Java_com_dorflix_app_video_VideoDownloader_nativeCleanupCache},
    {"nativeGetCacheSize", "()J", (void*)Java_com_dorflix_app_video_VideoDownloader_nativeGetCacheSize},
    {"onDownloadProgress", "(Ljava/lang/String;JJ)V", (void*)Java_com_dorflix_app_video_VideoDownloader_onDownloadProgress},
    {"onDownloadComplete", "(Ljava/lang/String;Ljava/lang/String;)V", (void*)Java_com_dorflix_app_video_VideoDownloader_onDownloadComplete},
    {"onDownloadError", "(Ljava/lang/String;Ljava/lang/String;)V", (void*)Java_com_dorflix_app_video_VideoDownloader_onDownloadError},
};

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    JNI_LOG_THREAD("JNI_OnLoad started");

    long long startTime = clock();
    JNI_LOG_ENTRY("JNI_OnLoad");

    LOGI("=== JNI REGISTRATION START ===");
    LOGI("JVM pointer: %p, Reserved: %p", vm, reserved);

    // Store JVM reference for thread attachment
    g_jvm = vm;
    LOGI(" JVM reference stored: %p", g_jvm);

    JNIEnv* env;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        LOGE("L Failed to get JNI environment (JNI_VERSION_1_6)");
        JNI_LOG_ERROR("JNI_OnLoad", "Failed to get JNI environment");
        JNI_LOG_EXIT("JNI_OnLoad", false);
        return -1;
    }

    LOGI(" JNI environment acquired: %p, Version: %d", env, JNI_VERSION_1_6);

    LOGI("JNI_OnLoad called, verifying FFmpeg libraries");

    static const JNINativeMethod dorflixApplicationMethods[] = {
    {"isJNILibraryReady", "()Z", (void*)Java_com_dorflix_app_DorflixApplication_isJNILibraryReady},
    };

    // Verify FFmpeg libraries are properly loaded before proceeding
    if (!verifyFFmpegLibraries()) {
        LOGE("L FFmpeg library verification failed during JNI_OnLoad");
        return -1;
    }

    LOGI("=== STARTING JNI METHOD REGISTRATION ===");
    int totalClassesAttempted = 0;
    int totalClassesSuccessful = 0;
    int totalMethodsRegistered = 0;

    // ===== VIDEO PLAYER CONTROLLER REGISTRATION =====
    LOGI("=' Registering VideoPlayerController methods...");
    totalClassesAttempted++;

    jclass videoPlayerClass = env->FindClass("com/dorflix/app/video/VideoPlayerController");
    if (videoPlayerClass == nullptr) {
        LOGE("L Failed to find VideoPlayerController class - aborting JNI initialization");

        // Check for exceptions
        if (env->ExceptionCheck()) {
            LOGE("  � Exception during FindClass:");
            env->ExceptionDescribe();
            env->ExceptionClear();
        }
        return -1;
    }

    LOGI(" Found VideoPlayerController class: %p", videoPlayerClass);
    LOGI("  � Attempting to register %d methods", sizeof(videoPlayerMethods) / sizeof(videoPlayerMethods[0]));

    // Log method signatures for debugging
    for (size_t i = 0; i < sizeof(videoPlayerMethods) / sizeof(videoPlayerMethods[0]); ++i) {
        LOGI("    Method %d: %s %s", (int)i, videoPlayerMethods[i].name, videoPlayerMethods[i].signature);
    }

    int result = env->RegisterNatives(videoPlayerClass, videoPlayerMethods,
                                     sizeof(videoPlayerMethods) / sizeof(videoPlayerMethods[0]));
    if (result < 0) {
        LOGE("L Failed to register VideoPlayerController native methods (error code: %d)", result);

        // Check for pending exceptions that might explain the failure
        if (env->ExceptionCheck()) {
            LOGE("  � Exception occurred during RegisterNatives:");
            env->ExceptionDescribe();
            env->ExceptionClear();
        } else {
            LOGE("  � No exception occurred, but RegisterNatives returned error code %d", result);
        }
        return -1;
    }

    LOGI(" Successfully registered VideoPlayerController native methods (%d methods)",
         sizeof(videoPlayerMethods) / sizeof(videoPlayerMethods[0]));
    totalClassesSuccessful++;
    totalMethodsRegistered += sizeof(videoPlayerMethods) / sizeof(videoPlayerMethods[0]);
    
    // ===== VIDEO PRELOADER REGISTRATION =====
    LOGI("=' Registering VideoPreloader methods...");
    totalClassesAttempted++;

    jclass preloaderClass = env->FindClass("com/dorflix/app/video/VideoPreloader");
    if (preloaderClass == nullptr) {
        LOGE("L Failed to find VideoPreloader class - aborting JNI initialization");

        // Check for exceptions
        if (env->ExceptionCheck()) {
            LOGE("  � Exception during FindClass:");
            env->ExceptionDescribe();
            env->ExceptionClear();
        }
        return -1;
    }

    LOGI(" Found VideoPreloader class: %p", preloaderClass);
    LOGI("  � Attempting to register %d methods", sizeof(preloaderMethods) / sizeof(preloaderMethods[0]));

    result = env->RegisterNatives(preloaderClass, preloaderMethods,
                                 sizeof(preloaderMethods) / sizeof(preloaderMethods[0]));
    if (result < 0) {
        LOGE("L Failed to register VideoPreloader native methods (error code: %d)", result);

        // Check for pending exceptions
        if (env->ExceptionCheck()) {
            LOGE("  � Exception occurred during RegisterNatives:");
            env->ExceptionDescribe();
            env->ExceptionClear();
        }
        return -1;
    }

    LOGI(" Successfully registered VideoPreloader native methods (%d methods)",
         sizeof(preloaderMethods) / sizeof(preloaderMethods[0]));
    totalClassesSuccessful++;
    totalMethodsRegistered += sizeof(preloaderMethods) / sizeof(preloaderMethods[0]);

    // ===== MEMORY MANAGER REGISTRATION =====
    LOGI("=' Registering MemoryManager methods...");
    totalClassesAttempted++;

    jclass memoryManagerClass = env->FindClass("com/dorflix/app/video/MemoryManager");
    if (memoryManagerClass == nullptr) {
        LOGI("� MemoryManager class not found (optional) - continuing");
        LOGI(" MemoryManager registration skipped (optional class)");
    } else {
        LOGI(" Found MemoryManager class: %p", memoryManagerClass);
        LOGI("  � Attempting to register %d methods", sizeof(memoryManagerMethods) / sizeof(memoryManagerMethods[0]));

        result = env->RegisterNatives(memoryManagerClass, memoryManagerMethods,
                                     sizeof(memoryManagerMethods) / sizeof(memoryManagerMethods[0]));
        if (result < 0) {
            LOGE("L Failed to register MemoryManager native methods (error code: %d)", result);

            // Check for pending exceptions
            if (env->ExceptionCheck()) {
                LOGE("  � Exception occurred during RegisterNatives:");
                env->ExceptionDescribe();
                env->ExceptionClear();
            }
            return -1;
        }

        LOGI(" Successfully registered MemoryManager native methods (%d methods)",
             sizeof(memoryManagerMethods) / sizeof(memoryManagerMethods[0]));
        totalClassesSuccessful++;
        totalMethodsRegistered += sizeof(memoryManagerMethods) / sizeof(memoryManagerMethods[0]);
    } 

    // ===== CODEC ENUMERATOR REGISTRATION =====
    LOGI("=' Registering CodecEnumerator methods...");
    totalClassesAttempted++;

    jclass codecEnumeratorClass = env->FindClass("com/dorflix/app/CodecEnumerator");
    if (codecEnumeratorClass == nullptr) {
        LOGE("L Failed to find CodecEnumerator class - aborting JNI initialization");

        // Check for exceptions
        if (env->ExceptionCheck()) {
            LOGE("  � Exception during FindClass:");
            env->ExceptionDescribe();
            env->ExceptionClear();
        }
        return -1;
    }

    LOGI(" Found CodecEnumerator class: %p", codecEnumeratorClass);
    LOGI("  � Attempting to register %d methods", sizeof(codecEnumeratorMethods) / sizeof(codecEnumeratorMethods[0]));

    result = env->RegisterNatives(codecEnumeratorClass, codecEnumeratorMethods,
                                sizeof(codecEnumeratorMethods) / sizeof(codecEnumeratorMethods[0]));
    if (result < 0) {
        LOGE("L Failed to register CodecEnumerator native methods (error code: %d)", result);

        // Check for pending exceptions
        if (env->ExceptionCheck()) {
            LOGE("  � Exception occurred during RegisterNatives:");
            env->ExceptionDescribe();
            env->ExceptionClear();
        }
        return -1;
    }

    LOGI(" Successfully registered CodecEnumerator native methods (%d methods)",
        sizeof(codecEnumeratorMethods) / sizeof(codecEnumeratorMethods[0]));
    totalClassesSuccessful++;
    totalMethodsRegistered += sizeof(codecEnumeratorMethods) / sizeof(codecEnumeratorMethods[0]);

    // ===== DORFLIX APPLICATION REGISTRATION =====
    LOGI("=' Registering DorflixApplication methods...");
    totalClassesAttempted++;

    jclass dorflixApplicationClass = env->FindClass("com/dorflix/app/DorflixApplication");
    if (dorflixApplicationClass == nullptr) {
        LOGE("L Failed to find DorflixApplication class - aborting JNI initialization");
        
        // Check for exceptions
        if (env->ExceptionCheck()) {
            LOGE("  � Exception during FindClass:");
            env->ExceptionDescribe();
            env->ExceptionClear();
        }
        return -1;
    }

    LOGI(" Found DorflixApplication class: %p", dorflixApplicationClass);
    LOGI("  � Attempting to register %d methods", sizeof(dorflixApplicationMethods) / sizeof(dorflixApplicationMethods[0]));

    result = env->RegisterNatives(dorflixApplicationClass, dorflixApplicationMethods,
                                sizeof(dorflixApplicationMethods) / sizeof(dorflixApplicationMethods[0]));
    if (result < 0) {
        LOGE("L Failed to register DorflixApplication native methods (error code: %d)", result);
        
        // Check for pending exceptions
        if (env->ExceptionCheck()) {
            LOGE("  � Exception occurred during RegisterNatives:");
            env->ExceptionDescribe();
            env->ExceptionClear();
        }
        return -1;
    }

    LOGI(" Successfully registered DorflixApplication native methods (%d methods)",
        sizeof(dorflixApplicationMethods) / sizeof(dorflixApplicationMethods[0]));
    totalClassesSuccessful++;
    totalMethodsRegistered += sizeof(dorflixApplicationMethods) / sizeof(dorflixApplicationMethods[0]);


    // ===== VIDEO DOWNLOADER REGISTRATION =====
    LOGI("=' Registering VideoDownloader methods...");
    totalClassesAttempted++;

    jclass videoDownloaderClass = env->FindClass("com/dorflix/app/video/VideoDownloader");
    if (videoDownloaderClass == nullptr) {
        LOGE("L Failed to find VideoDownloader class - aborting JNI initialization");
        
        // Check for exceptions
        if (env->ExceptionCheck()) {
            LOGE("  � Exception during FindClass:");
            env->ExceptionDescribe();
            env->ExceptionClear();
        }
        return -1;
    }

    LOGI(" Found VideoDownloader class: %p", videoDownloaderClass);
    LOGI("  � Attempting to register %d methods", sizeof(videoDownloaderMethods) / sizeof(videoDownloaderMethods[0]));

    result = env->RegisterNatives(videoDownloaderClass, videoDownloaderMethods,
                                sizeof(videoDownloaderMethods) / sizeof(videoDownloaderMethods[0]));
    if (result < 0) {
        LOGE("L Failed to register VideoDownloader native methods (error code: %d)", result);
        
        // Check for pending exceptions
        if (env->ExceptionCheck()) {
            LOGE("  � Exception occurred during RegisterNatives:");
            env->ExceptionDescribe();
            env->ExceptionClear();
        }
        return -1;
    }

    LOGI(" Successfully registered VideoDownloader native methods (%d methods)",
        sizeof(videoDownloaderMethods) / sizeof(videoDownloaderMethods[0]));
    totalClassesSuccessful++;
    totalMethodsRegistered += sizeof(videoDownloaderMethods) / sizeof(videoDownloaderMethods[0]);
    
    
    // ===== JNI REGISTRATION COMPLETE =====
    LOGI("=== JNI REGISTRATION COMPLETE ===");
    LOGI("Total classes attempted: %d", totalClassesAttempted);
    LOGI("Total classes successful: %d", totalClassesSuccessful);
    LOGI("Total methods registered: %d", totalMethodsRegistered);

    // Set JNI readiness flag
    g_jniReady = true;
    LOGI(" JNI library ready for use");

    JNI_LOG_PERF("JNI_OnLoad", startTime);
    JNI_LOG_EXIT("JNI_OnLoad", true);

    // Return the JNI version
    return JNI_VERSION_1_6;
    }
