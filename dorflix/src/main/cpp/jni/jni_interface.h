#ifndef JNI_INTERFACE_H
#define JNI_INTERFACE_H

#include <jni.h>   // ✅ MUST be first

#include <string>
#include <vector>

// Forward declaration for EnumeratedCodecInfo
struct EnumeratedCodecInfo;

// ===== EXTERN DECLARATIONS FOR ENUMERATION FUNCTIONS =====
// These functions are implemented in jni_interface.cpp but called from MediaCodecHardwareDecoder.cpp
// They need to be extern so they can be linked across library boundaries

// Global storage access functions (C++ only - extern needed for cross-translation-unit linking)
extern std::vector<EnumeratedCodecInfo> getEnumeratedCodecs();
extern bool isEnumerationComplete();
extern void resetCodecEnumeration();

#ifdef __cplusplus
extern "C" {
#endif

// JNI callback functions for CodecEnumerator - matches Kotlin external declarations
JNIEXPORT void JNICALL
Java_com_dorflix_app_CodecEnumerator_storeCodecInfo(
    JNIEnv* env, jclass clazz,
    jstring name, jstring mimeType,
    jboolean isEncoder, jboolean isHardware,
    jint maxWidth, jint maxHeight,
    jint profile, jint level,
    jobjectArray hdrSupport, jobjectArray colorFormats,
    jint maxBitrate);

JNIEXPORT void JNICALL
Java_com_dorflix_app_CodecEnumerator_signalEnumerationComplete(JNIEnv* env, jclass clazz, jint totalCodecs);

#ifdef __cplusplus
}
#endif

/**
 * JNI interface for C++ video processing
 */

#ifdef __cplusplus
extern "C" {
#endif

// Video player functions
JNIEXPORT jlong JNICALL
Java_com_dorflix_app_video_VideoPlayerController_nativeCreatePlayer(JNIEnv *env, jobject thiz);

JNIEXPORT void JNICALL
Java_com_dorflix_app_video_VideoPlayerController_nativeDestroyPlayer(JNIEnv *env, jobject thiz, jlong player_ptr);

JNIEXPORT jboolean JNICALL
Java_com_dorflix_app_video_VideoPlayerController_nativeSetSurface(JNIEnv *env, jobject thiz, jlong player_ptr, jobject surface);

JNIEXPORT jboolean JNICALL
Java_com_dorflix_app_video_VideoPlayerController_nativeLoadVideo(JNIEnv *env, jobject thiz, jlong player_ptr, jstring video_path);

JNIEXPORT void JNICALL
Java_com_dorflix_app_video_VideoPlayerController_nativeSetupCallbacks(JNIEnv *env, jobject thiz, jlong player_ptr);

JNIEXPORT void JNICALL
Java_com_dorflix_app_video_VideoPlayerController_nativePlay(JNIEnv *env, jobject thiz, jlong player_ptr);

JNIEXPORT void JNICALL
Java_com_dorflix_app_video_VideoPlayerController_nativePause(JNIEnv *env, jobject thiz, jlong player_ptr);

JNIEXPORT void JNICALL
Java_com_dorflix_app_video_VideoPlayerController_nativeStop(JNIEnv *env, jobject thiz, jlong player_ptr);

JNIEXPORT void JNICALL
Java_com_dorflix_app_video_VideoPlayerController_nativeSeekTo(JNIEnv *env, jobject thiz, jlong player_ptr, jint position);

JNIEXPORT jint JNICALL
Java_com_dorflix_app_video_VideoPlayerController_nativeGetCurrentPosition(JNIEnv *env, jobject thiz, jlong player_ptr);

JNIEXPORT jint JNICALL
Java_com_dorflix_app_video_VideoPlayerController_nativeGetDuration(JNIEnv *env, jobject thiz, jlong player_ptr);

JNIEXPORT jboolean JNICALL
Java_com_dorflix_app_video_VideoPlayerController_nativeIsPlaying(JNIEnv *env, jobject thiz, jlong player_ptr);

JNIEXPORT void JNICALL
Java_com_dorflix_app_video_VideoPlayerController_nativeSetVolume(JNIEnv *env, jobject thiz, jlong player_ptr, jfloat volume);

JNIEXPORT void JNICALL
Java_com_dorflix_app_video_VideoPlayerController_nativeSetMute(JNIEnv *env, jobject thiz, jlong player_ptr, jboolean mute);

// Preloader functions
JNIEXPORT jlong JNICALL
Java_com_dorflix_app_video_VideoPreloader_nativeCreatePreloader(JNIEnv *env, jobject thiz);

JNIEXPORT void JNICALL
Java_com_dorflix_app_video_VideoPreloader_nativeDestroyPreloader(JNIEnv *env, jobject thiz, jlong preloader_ptr);

JNIEXPORT void JNICALL
Java_com_dorflix_app_video_VideoPreloader_nativePreloadVideo(JNIEnv *env, jobject thiz, jlong preloader_ptr, jstring video_path);

// Memory management functions
JNIEXPORT jlong JNICALL
Java_com_dorflix_app_video_MemoryManager_nativeCreateMemoryPool(JNIEnv *env, jobject thiz, jint pool_size);

JNIEXPORT void JNICALL
Java_com_dorflix_app_video_MemoryManager_nativeDestroyMemoryPool(JNIEnv *env, jobject thiz, jlong pool_ptr);

JNIEXPORT jlong JNICALL
Java_com_dorflix_app_video_MemoryManager_nativeAllocate(JNIEnv *env, jobject thiz, jlong pool_ptr, jint size);

JNIEXPORT void JNICALL
Java_com_dorflix_app_video_MemoryManager_nativeFree(JNIEnv *env, jobject thiz, jlong pool_ptr, jlong ptr);

JNIEXPORT jint JNICALL
Java_com_dorflix_app_video_MemoryManager_nativeGetPoolSize(JNIEnv *env, jobject thiz, jlong pool_ptr);

JNIEXPORT jint JNICALL
Java_com_dorflix_app_video_MemoryManager_nativeGetUsedSize(JNIEnv *env, jobject thiz, jlong pool_ptr);

#ifdef __cplusplus
}
#endif

#endif // JNI_INTERFACE_H