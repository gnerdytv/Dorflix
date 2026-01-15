package com.dorflix.app.video;

/**
 * Controller for video playback using C++ backend
 */
@kotlin.Metadata(mv = {2, 3, 0}, k = 1, xi = 48, d1 = {"\u0000N\n\u0002\u0018\u0002\n\u0002\u0010\u0000\n\u0000\n\u0002\u0018\u0002\n\u0002\b\u0004\n\u0002\u0010\t\n\u0000\n\u0002\u0010\u0002\n\u0002\b\u0002\n\u0002\u0010\u000b\n\u0000\n\u0002\u0010\u000e\n\u0002\b\u0005\n\u0002\u0010\b\n\u0002\b\u0002\n\u0002\u0018\u0002\n\u0002\b\u0005\n\u0002\u0010\u0007\n\u0002\b\u0012\n\u0002\u0018\u0002\n\u0002\b\u0014\u0018\u0000 D2\u00020\u0001:\u0001DB\u000f\u0012\u0006\u0010\u0002\u001a\u00020\u0003\u00a2\u0006\u0004\b\u0004\u0010\u0005J\t\u0010\u0007\u001a\u00020\bH\u0082 J\u0011\u0010\t\u001a\u00020\n2\u0006\u0010\u000b\u001a\u00020\bH\u0082 J\u0019\u0010\f\u001a\u00020\r2\u0006\u0010\u000b\u001a\u00020\b2\u0006\u0010\u000e\u001a\u00020\u000fH\u0082 J\u0011\u0010\u0010\u001a\u00020\n2\u0006\u0010\u000b\u001a\u00020\bH\u0082 J\u0011\u0010\u0011\u001a\u00020\n2\u0006\u0010\u000b\u001a\u00020\bH\u0082 J\u0011\u0010\u0012\u001a\u00020\n2\u0006\u0010\u000b\u001a\u00020\bH\u0082 J\u0019\u0010\u0013\u001a\u00020\n2\u0006\u0010\u000b\u001a\u00020\b2\u0006\u0010\u0014\u001a\u00020\u0015H\u0082 J\u001b\u0010\u0016\u001a\u00020\r2\u0006\u0010\u000b\u001a\u00020\b2\b\u0010\u0017\u001a\u0004\u0018\u00010\u0018H\u0082 J\u0011\u0010\u0019\u001a\u00020\u00152\u0006\u0010\u000b\u001a\u00020\bH\u0082 J\u0011\u0010\u001a\u001a\u00020\u00152\u0006\u0010\u000b\u001a\u00020\bH\u0082 J\u0011\u0010\u001b\u001a\u00020\r2\u0006\u0010\u000b\u001a\u00020\bH\u0082 J\u0019\u0010\u001c\u001a\u00020\n2\u0006\u0010\u000b\u001a\u00020\b2\u0006\u0010\u001d\u001a\u00020\u001eH\u0082 J\u0019\u0010\u001f\u001a\u00020\n2\u0006\u0010\u000b\u001a\u00020\b2\u0006\u0010 \u001a\u00020\rH\u0082 J\u0011\u0010!\u001a\u00020\n2\u0006\u0010\u000b\u001a\u00020\bH\u0082 J\u000e\u0010\"\u001a\u00020\r2\u0006\u0010\u000e\u001a\u00020\u000fJ\u0006\u0010#\u001a\u00020\nJ\u0006\u0010$\u001a\u00020\nJ\u0006\u0010%\u001a\u00020\nJ\u0006\u0010&\u001a\u00020\nJ\u000e\u0010\'\u001a\u00020\n2\u0006\u0010\u0014\u001a\u00020\u0015J\u0010\u0010(\u001a\u00020\r2\b\u0010\u0017\u001a\u0004\u0018\u00010\u0018J\u0006\u0010)\u001a\u00020\u0015J\u0006\u0010*\u001a\u00020\u0015J\u0006\u0010+\u001a\u00020\rJ\u0006\u0010,\u001a\u00020\nJ\u0006\u0010-\u001a\u00020\rJ)\u0010.\u001a\u0004\u0018\u0001H/\"\u0004\b\u0000\u0010/2\u0012\u00100\u001a\u000e\u0012\u0004\u0012\u00020\b\u0012\u0004\u0012\u0002H/01H\u0002\u00a2\u0006\u0002\u00102J\u0010\u00103\u001a\u00020\n2\u0006\u00104\u001a\u00020\u0015H\u0002J\b\u00105\u001a\u00020\nH\u0002J\b\u00106\u001a\u00020\nH\u0002J\b\u00107\u001a\u00020\nH\u0002J\b\u00108\u001a\u00020\nH\u0002J\u0018\u00109\u001a\u00020\n2\u0006\u0010:\u001a\u00020\u00152\u0006\u0010;\u001a\u00020\u000fH\u0002J\u0010\u0010<\u001a\u00020\n2\u0006\u0010=\u001a\u00020\u0015H\u0002J\b\u0010>\u001a\u00020\nH\u0002J\b\u0010?\u001a\u00020\nH\u0002J\b\u0010@\u001a\u00020\nH\u0002J\u0018\u0010A\u001a\u00020\n2\u0006\u0010B\u001a\u00020\u00152\u0006\u0010C\u001a\u00020\u0015H\u0002R\u000e\u0010\u0002\u001a\u00020\u0003X\u0082\u0004\u00a2\u0006\u0002\n\u0000R\u0010\u0010\u0006\u001a\u00020\u00038\u0006X\u0087\u0004\u00a2\u0006\u0002\n\u0000R\u000e\u0010\u000b\u001a\u00020\bX\u0082\u000e\u00a2\u0006\u0002\n\u0000\u00a8\u0006E"}, d2 = {"Lcom/dorflix/app/video/VideoPlayerController;", "", "listener", "Lcom/dorflix/app/video/VideoPlayerListener;", "<init>", "(Lcom/dorflix/app/video/VideoPlayerListener;)V", "callback", "nativeCreatePlayer", "", "nativeSetupCallbacks", "", "playerHandle", "nativeLoadVideo", "", "videoPath", "", "nativePlay", "nativePause", "nativeStop", "nativeSeekTo", "positionMs", "", "nativeSetSurface", "surface", "Landroid/view/Surface;", "nativeGetCurrentPosition", "nativeGetDuration", "nativeIsPlaying", "nativeSetVolume", "volume", "", "nativeSetMute", "mute", "nativeDestroyPlayer", "loadVideo", "play", "pause", "resume", "stop", "seekTo", "setSurface", "getCurrentPosition", "getDuration", "isPlaying", "release", "isInitialized", "withPlayer", "T", "operation", "Lkotlin/Function1;", "(Lkotlin/jvm/functions/Function1;)Ljava/lang/Object;", "onVideoPrepared", "duration", "onVideoStarted", "onVideoPaused", "onVideoStopped", "onVideoCompleted", "onVideoError", "errorCode", "errorMessage", "onVideoProgressChanged", "position", "onVideoBufferingStarted", "onVideoBufferingEnded", "onVideoSeekComplete", "onVideoSizeChanged", "width", "height", "Companion", "DorflixNative_debug"})
public final class VideoPlayerController {
    @org.jetbrains.annotations.NotNull()
    private final com.dorflix.app.video.VideoPlayerListener listener = null;
    @kotlin.jvm.JvmField()
    @org.jetbrains.annotations.NotNull()
    public final com.dorflix.app.video.VideoPlayerListener callback = null;
    private long playerHandle = 0L;
    @org.jetbrains.annotations.NotNull()
    public static final com.dorflix.app.video.VideoPlayerController.Companion Companion = null;
    
    public VideoPlayerController(@org.jetbrains.annotations.NotNull()
    com.dorflix.app.video.VideoPlayerListener listener) {
        super();
    }
    
    private final native long nativeCreatePlayer() {
        return 0L;
    }
    
    private final native void nativeSetupCallbacks(long playerHandle) {
    }
    
    private final native boolean nativeLoadVideo(long playerHandle, java.lang.String videoPath) {
        return false;
    }
    
    private final native void nativePlay(long playerHandle) {
    }
    
    private final native void nativePause(long playerHandle) {
    }
    
    private final native void nativeStop(long playerHandle) {
    }
    
    private final native void nativeSeekTo(long playerHandle, int positionMs) {
    }
    
    private final native boolean nativeSetSurface(long playerHandle, android.view.Surface surface) {
        return false;
    }
    
    private final native int nativeGetCurrentPosition(long playerHandle) {
        return 0;
    }
    
    private final native int nativeGetDuration(long playerHandle) {
        return 0;
    }
    
    private final native boolean nativeIsPlaying(long playerHandle) {
        return false;
    }
    
    private final native void nativeSetVolume(long playerHandle, float volume) {
    }
    
    private final native void nativeSetMute(long playerHandle, boolean mute) {
    }
    
    private final native void nativeDestroyPlayer(long playerHandle) {
    }
    
    public final boolean loadVideo(@org.jetbrains.annotations.NotNull()
    java.lang.String videoPath) {
        return false;
    }
    
    public final void play() {
    }
    
    public final void pause() {
    }
    
    public final void resume() {
    }
    
    public final void stop() {
    }
    
    public final void seekTo(int positionMs) {
    }
    
    public final boolean setSurface(@org.jetbrains.annotations.Nullable()
    android.view.Surface surface) {
        return false;
    }
    
    public final int getCurrentPosition() {
        return 0;
    }
    
    public final int getDuration() {
        return 0;
    }
    
    public final boolean isPlaying() {
        return false;
    }
    
    public final void release() {
    }
    
    /**
     * Check if the native player is properly initialized
     */
    public final boolean isInitialized() {
        return false;
    }
    
    /**
     * Safely perform an operation that requires the native player
     */
    private final <T extends java.lang.Object>T withPlayer(kotlin.jvm.functions.Function1<? super java.lang.Long, ? extends T> operation) {
        return null;
    }
    
    @kotlin.Suppress(names = {"unused"})
    private final void onVideoPrepared(int duration) {
    }
    
    @kotlin.Suppress(names = {"unused"})
    private final void onVideoStarted() {
    }
    
    @kotlin.Suppress(names = {"unused"})
    private final void onVideoPaused() {
    }
    
    @kotlin.Suppress(names = {"unused"})
    private final void onVideoStopped() {
    }
    
    @kotlin.Suppress(names = {"unused"})
    private final void onVideoCompleted() {
    }
    
    @kotlin.Suppress(names = {"unused"})
    private final void onVideoError(int errorCode, java.lang.String errorMessage) {
    }
    
    @kotlin.Suppress(names = {"unused"})
    private final void onVideoProgressChanged(int position) {
    }
    
    @kotlin.Suppress(names = {"unused"})
    private final void onVideoBufferingStarted() {
    }
    
    @kotlin.Suppress(names = {"unused"})
    private final void onVideoBufferingEnded() {
    }
    
    @kotlin.Suppress(names = {"unused"})
    private final void onVideoSeekComplete() {
    }
    
    @kotlin.Suppress(names = {"unused"})
    private final void onVideoSizeChanged(int width, int height) {
    }
    
    @kotlin.Metadata(mv = {2, 3, 0}, k = 1, xi = 48, d1 = {"\u0000\f\n\u0002\u0018\u0002\n\u0002\u0010\u0000\n\u0002\b\u0003\b\u0086\u0003\u0018\u00002\u00020\u0001B\t\b\u0002\u00a2\u0006\u0004\b\u0002\u0010\u0003\u00a8\u0006\u0004"}, d2 = {"Lcom/dorflix/app/video/VideoPlayerController$Companion;", "", "<init>", "()V", "DorflixNative_debug"})
    public static final class Companion {
        
        private Companion() {
            super();
        }
    }
}