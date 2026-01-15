package com.dorflix.app.ui;

/**
 * Fragment for video playback with C++ integration
 */
@kotlin.Metadata(mv = {2, 3, 0}, k = 1, xi = 48, d1 = {"\u0000\u0096\u0001\n\u0002\u0018\u0002\n\u0002\u0018\u0002\n\u0002\u0018\u0002\n\u0002\b\u0003\n\u0002\u0018\u0002\n\u0002\b\u0004\n\u0002\u0018\u0002\n\u0002\b\u0005\n\u0002\u0018\u0002\n\u0000\n\u0002\u0010\u000e\n\u0002\b\u0002\n\u0002\u0010\u000b\n\u0000\n\u0002\u0010\b\n\u0002\b\b\n\u0002\u0018\u0002\n\u0002\b\u0002\n\u0002\u0010\t\n\u0002\b\u0004\n\u0002\u0018\u0002\n\u0000\n\u0002\u0018\u0002\n\u0000\n\u0002\u0018\u0002\n\u0002\u0018\u0002\n\u0000\n\u0002\u0010\u0002\n\u0000\n\u0002\u0018\u0002\n\u0000\n\u0002\u0018\u0002\n\u0000\n\u0002\u0018\u0002\n\u0000\n\u0002\u0018\u0002\n\u0002\b\u001c\n\u0002\u0010$\n\u0002\u0010\u0000\n\u0002\b\u0017\n\u0002\u0018\u0002\n\u0002\b\t\u0018\u0000 v2\u00020\u00012\u00020\u0002:\u0001vB\u0007\u00a2\u0006\u0004\b\u0003\u0010\u0004J\u0012\u0010/\u001a\u0002002\b\u00101\u001a\u0004\u0018\u000102H\u0016J$\u00103\u001a\u0002042\u0006\u00105\u001a\u0002062\b\u00107\u001a\u0004\u0018\u0001082\b\u00101\u001a\u0004\u0018\u000102H\u0016J\u001a\u00109\u001a\u0002002\u0006\u0010:\u001a\u0002042\b\u00101\u001a\u0004\u0018\u000102H\u0016J\b\u0010;\u001a\u000200H\u0002J\b\u0010<\u001a\u000200H\u0002J\b\u0010=\u001a\u000200H\u0002J\b\u0010>\u001a\u000200H\u0002J\b\u0010?\u001a\u000200H\u0002J\u0010\u0010@\u001a\u0002002\u0006\u0010A\u001a\u00020\u0013H\u0002J\b\u0010B\u001a\u000200H\u0002J\b\u0010C\u001a\u000200H\u0002J\b\u0010D\u001a\u000200H\u0002J\b\u0010E\u001a\u000200H\u0002J!\u0010F\u001a\u0002002\u0006\u0010G\u001a\u00020\u00182\n\b\u0002\u0010H\u001a\u0004\u0018\u00010\u0018H\u0002\u00a2\u0006\u0002\u0010IJ\u0010\u0010J\u001a\u00020\u00132\u0006\u0010K\u001a\u00020\u0018H\u0002J\u0010\u0010L\u001a\u0002002\u0006\u0010M\u001a\u00020\u0013H\u0002J\u0010\u0010N\u001a\u0002002\u0006\u0010M\u001a\u00020\u0013H\u0002J\b\u0010O\u001a\u000200H\u0002J\b\u0010P\u001a\u000200H\u0002J\b\u0010Q\u001a\u000200H\u0002J&\u0010R\u001a\u0002002\u0006\u0010S\u001a\u00020\u00132\u0014\b\u0002\u0010T\u001a\u000e\u0012\u0004\u0012\u00020\u0013\u0012\u0004\u0012\u00020V0UH\u0002J\u0010\u0010W\u001a\u0002002\u0006\u0010H\u001a\u00020\u0018H\u0016J\b\u0010X\u001a\u000200H\u0016J\b\u0010Y\u001a\u000200H\u0016J\b\u0010Z\u001a\u000200H\u0016J\b\u0010[\u001a\u000200H\u0016J\u0018\u0010\\\u001a\u0002002\u0006\u0010]\u001a\u00020\u00182\u0006\u0010^\u001a\u00020\u0013H\u0016J\u0010\u0010_\u001a\u0002002\u0006\u0010G\u001a\u00020\u0018H\u0016J\b\u0010`\u001a\u000200H\u0016J\b\u0010a\u001a\u000200H\u0016J\b\u0010b\u001a\u000200H\u0016J\u0018\u0010c\u001a\u0002002\u0006\u0010d\u001a\u00020\u00182\u0006\u0010e\u001a\u00020\u0018H\u0016J\u0010\u0010f\u001a\u0002002\u0006\u0010g\u001a\u000202H\u0016J\b\u0010h\u001a\u000200H\u0016J\b\u0010i\u001a\u000200H\u0016J\b\u0010j\u001a\u000200H\u0016J\b\u0010k\u001a\u000200H\u0016J\u0010\u0010l\u001a\u0002002\u0006\u0010m\u001a\u00020nH\u0016J\u0018\u0010o\u001a\u0002002\u0006\u0010\u0012\u001a\u00020\u00132\u0006\u0010A\u001a\u00020\u0013H\u0002J\u0010\u0010p\u001a\u00020\u00182\u0006\u0010q\u001a\u00020\u0018H\u0002J\b\u0010r\u001a\u000200H\u0002J\b\u0010s\u001a\u000200H\u0002J\b\u0010t\u001a\u000200H\u0002J\b\u0010u\u001a\u000200H\u0002R\u0010\u0010\u0005\u001a\u0004\u0018\u00010\u0006X\u0082\u000e\u00a2\u0006\u0002\n\u0000R\u0014\u0010\u0007\u001a\u00020\u00068BX\u0082\u0004\u00a2\u0006\u0006\u001a\u0004\b\b\u0010\tR\u001b\u0010\n\u001a\u00020\u000b8BX\u0082\u0084\u0002\u00a2\u0006\f\n\u0004\b\u000e\u0010\u000f\u001a\u0004\b\f\u0010\rR\u000e\u0010\u0010\u001a\u00020\u0011X\u0082.\u00a2\u0006\u0002\n\u0000R\u000e\u0010\u0012\u001a\u00020\u0013X\u0082\u000e\u00a2\u0006\u0002\n\u0000R\u0010\u0010\u0014\u001a\u0004\u0018\u00010\u0013X\u0082\u000e\u00a2\u0006\u0002\n\u0000R\u000e\u0010\u0015\u001a\u00020\u0016X\u0082\u000e\u00a2\u0006\u0002\n\u0000R\u000e\u0010\u0017\u001a\u00020\u0018X\u0082\u000e\u00a2\u0006\u0002\n\u0000R\u000e\u0010\u0019\u001a\u00020\u0018X\u0082\u000e\u00a2\u0006\u0002\n\u0000R\u000e\u0010\u001a\u001a\u00020\u0018X\u0082\u000e\u00a2\u0006\u0002\n\u0000R\u000e\u0010\u001b\u001a\u00020\u0018X\u0082\u000e\u00a2\u0006\u0002\n\u0000R\u000e\u0010\u001c\u001a\u00020\u0016X\u0082\u000e\u00a2\u0006\u0002\n\u0000R\u000e\u0010\u001d\u001a\u00020\u0016X\u0082\u000e\u00a2\u0006\u0002\n\u0000R\u000e\u0010\u001e\u001a\u00020\u0016X\u0082\u000e\u00a2\u0006\u0002\n\u0000R\u000e\u0010\u001f\u001a\u00020\u0016X\u0082\u000e\u00a2\u0006\u0002\n\u0000R\u0010\u0010 \u001a\u0004\u0018\u00010!X\u0082\u000e\u00a2\u0006\u0002\n\u0000R\u000e\u0010\"\u001a\u00020\u0018X\u0082\u000e\u00a2\u0006\u0002\n\u0000R\u000e\u0010#\u001a\u00020$X\u0082\u000e\u00a2\u0006\u0002\n\u0000R\u000e\u0010%\u001a\u00020$X\u0082\u000e\u00a2\u0006\u0002\n\u0000R\u0010\u0010&\u001a\u0004\u0018\u00010\u0013X\u0082\u000e\u00a2\u0006\u0002\n\u0000R\u000e\u0010\'\u001a\u00020\u0016X\u0082\u000e\u00a2\u0006\u0002\n\u0000R\u000e\u0010(\u001a\u00020)X\u0082.\u00a2\u0006\u0002\n\u0000R\u000e\u0010*\u001a\u00020+X\u0082.\u00a2\u0006\u0002\n\u0000R\u0014\u0010,\u001a\b\u0018\u00010-R\u00020.X\u0082\u000e\u00a2\u0006\u0002\n\u0000\u00a8\u0006w"}, d2 = {"Lcom/dorflix/app/ui/VideoPlayerFragment;", "Landroidx/fragment/app/Fragment;", "Lcom/dorflix/app/video/VideoPlayerListener;", "<init>", "()V", "_binding", "Lcom/dorflix/app/databinding/FragmentVideoPlayerBinding;", "binding", "getBinding", "()Lcom/dorflix/app/databinding/FragmentVideoPlayerBinding;", "viewModel", "Lcom/dorflix/app/presentation/viewmodel/VideoViewModel;", "getViewModel", "()Lcom/dorflix/app/presentation/viewmodel/VideoViewModel;", "viewModel$delegate", "Lkotlin/Lazy;", "videoPlayerController", "Lcom/dorflix/app/video/VideoPlayerController;", "videoId", "", "currentVideoUrl", "isPlaying", "", "currentPosition", "", "videoDuration", "lastSavedPosition", "retryCount", "isFullscreen", "isViewDestroyed", "isVideoPrepared", "isSurfaceReady", "videoPreparationTimeoutJob", "Lkotlinx/coroutines/Job;", "callbackRetryCount", "videoLoadStartTime", "", "lastCallbackReceivedTime", "lastProcessedVideoId", "observerSetUp", "connectivityManager", "Landroid/net/ConnectivityManager;", "networkCallback", "Landroid/net/ConnectivityManager$NetworkCallback;", "wakeLock", "Landroid/os/PowerManager$WakeLock;", "Landroid/os/PowerManager;", "onCreate", "", "savedInstanceState", "Landroid/os/Bundle;", "onCreateView", "Landroid/view/View;", "inflater", "Landroid/view/LayoutInflater;", "container", "Landroid/view/ViewGroup;", "onViewCreated", "view", "setupUI", "setupVideoPlayer", "tryStartPlayback", "setupNetworkMonitoring", "loadVideo", "startPlayback", "videoUrl", "retryVideoLoading", "togglePlayPause", "updatePlayPauseButton", "toggleFullscreen", "updatePositionText", "position", "duration", "(ILjava/lang/Integer;)V", "formatTimeSafe", "milliseconds", "showErrorMessage", "message", "showToast", "saveWatchProgress", "acquireWakeLock", "releaseWakeLock", "logVideoEvent", "eventName", "properties", "", "", "onVideoPrepared", "onVideoStarted", "onVideoPaused", "onVideoStopped", "onVideoCompleted", "onVideoError", "errorCode", "errorMessage", "onVideoProgressChanged", "onVideoBufferingStarted", "onVideoBufferingEnded", "onVideoSeekComplete", "onVideoSizeChanged", "width", "height", "onSaveInstanceState", "outState", "onPause", "onResume", "onStop", "onDestroyView", "onConfigurationChanged", "newConfig", "Landroid/content/res/Configuration;", "monitorVideoUrlInFragment", "abs", "value", "startVideoPreparationTimeout", "resetPlaybackState", "retryCallbackIfNeeded", "tryStartPlaybackSafely", "Companion", "DorflixNative_debug"})
public final class VideoPlayerFragment extends androidx.fragment.app.Fragment implements com.dorflix.app.video.VideoPlayerListener {
    @org.jetbrains.annotations.Nullable()
    private com.dorflix.app.databinding.FragmentVideoPlayerBinding _binding;
    @org.jetbrains.annotations.NotNull()
    private final kotlin.Lazy viewModel$delegate = null;
    private com.dorflix.app.video.VideoPlayerController videoPlayerController;
    @org.jetbrains.annotations.NotNull()
    private java.lang.String videoId = "";
    @org.jetbrains.annotations.Nullable()
    private java.lang.String currentVideoUrl;
    private boolean isPlaying = false;
    private int currentPosition = 0;
    private int videoDuration = 0;
    private int lastSavedPosition = 0;
    private int retryCount = 0;
    private boolean isFullscreen = false;
    private boolean isViewDestroyed = false;
    private boolean isVideoPrepared = false;
    private boolean isSurfaceReady = false;
    @org.jetbrains.annotations.Nullable()
    private kotlinx.coroutines.Job videoPreparationTimeoutJob;
    private int callbackRetryCount = 0;
    private long videoLoadStartTime = 0L;
    private long lastCallbackReceivedTime = 0L;
    @org.jetbrains.annotations.Nullable()
    private java.lang.String lastProcessedVideoId;
    private boolean observerSetUp = false;
    private android.net.ConnectivityManager connectivityManager;
    private android.net.ConnectivityManager.NetworkCallback networkCallback;
    @org.jetbrains.annotations.Nullable()
    private android.os.PowerManager.WakeLock wakeLock;
    @org.jetbrains.annotations.NotNull()
    private static final java.lang.String ARG_VIDEO_ID = "video_id";
    @org.jetbrains.annotations.NotNull()
    private static final java.lang.String ARG_VIDEO_URL = "video_url";
    private static final int MAX_RETRY_COUNT = 3;
    private static final int PROGRESS_SAVE_INTERVAL = 5000;
    private static final long PLAYBACK_TIMEOUT = 30000L;
    private static final long WAKE_LOCK_TIMEOUT = 600000L;
    private static final long VIDEO_PREPARATION_TIMEOUT = 15000L;
    private static final long CALLBACK_RETRY_DELAY = 1000L;
    private static final int MAX_CALLBACK_RETRIES = 3;
    @org.jetbrains.annotations.NotNull()
    public static final com.dorflix.app.ui.VideoPlayerFragment.Companion Companion = null;
    
    public VideoPlayerFragment() {
        super();
    }
    
    private final com.dorflix.app.databinding.FragmentVideoPlayerBinding getBinding() {
        return null;
    }
    
    private final com.dorflix.app.presentation.viewmodel.VideoViewModel getViewModel() {
        return null;
    }
    
    @java.lang.Override()
    public void onCreate(@org.jetbrains.annotations.Nullable()
    android.os.Bundle savedInstanceState) {
    }
    
    @java.lang.Override()
    @org.jetbrains.annotations.NotNull()
    public android.view.View onCreateView(@org.jetbrains.annotations.NotNull()
    android.view.LayoutInflater inflater, @org.jetbrains.annotations.Nullable()
    android.view.ViewGroup container, @org.jetbrains.annotations.Nullable()
    android.os.Bundle savedInstanceState) {
        return null;
    }
    
    @java.lang.Override()
    public void onViewCreated(@org.jetbrains.annotations.NotNull()
    android.view.View view, @org.jetbrains.annotations.Nullable()
    android.os.Bundle savedInstanceState) {
    }
    
    private final void setupUI() {
    }
    
    private final void setupVideoPlayer() {
    }
    
    /**
     * Attempts to start playback when both video and surface are ready
     * TikTok-style: Auto-play immediately when ready
     */
    private final void tryStartPlayback() {
    }
    
    private final void setupNetworkMonitoring() {
    }
    
    private final void loadVideo() {
    }
    
    private final void startPlayback(java.lang.String videoUrl) {
    }
    
    private final void retryVideoLoading() {
    }
    
    private final void togglePlayPause() {
    }
    
    private final void updatePlayPauseButton() {
    }
    
    private final void toggleFullscreen() {
    }
    
    private final void updatePositionText(int position, java.lang.Integer duration) {
    }
    
    private final java.lang.String formatTimeSafe(int milliseconds) {
        return null;
    }
    
    private final void showErrorMessage(java.lang.String message) {
    }
    
    private final void showToast(java.lang.String message) {
    }
    
    private final void saveWatchProgress() {
    }
    
    private final void acquireWakeLock() {
    }
    
    private final void releaseWakeLock() {
    }
    
    private final void logVideoEvent(java.lang.String eventName, java.util.Map<java.lang.String, ? extends java.lang.Object> properties) {
    }
    
    @java.lang.Override()
    public void onVideoPrepared(int duration) {
    }
    
    @java.lang.Override()
    public void onVideoStarted() {
    }
    
    @java.lang.Override()
    public void onVideoPaused() {
    }
    
    @java.lang.Override()
    public void onVideoStopped() {
    }
    
    @java.lang.Override()
    public void onVideoCompleted() {
    }
    
    @java.lang.Override()
    public void onVideoError(int errorCode, @org.jetbrains.annotations.NotNull()
    java.lang.String errorMessage) {
    }
    
    @java.lang.Override()
    public void onVideoProgressChanged(int position) {
    }
    
    @java.lang.Override()
    public void onVideoBufferingStarted() {
    }
    
    @java.lang.Override()
    public void onVideoBufferingEnded() {
    }
    
    @java.lang.Override()
    public void onVideoSeekComplete() {
    }
    
    @java.lang.Override()
    public void onVideoSizeChanged(int width, int height) {
    }
    
    @java.lang.Override()
    public void onSaveInstanceState(@org.jetbrains.annotations.NotNull()
    android.os.Bundle outState) {
    }
    
    @java.lang.Override()
    public void onPause() {
    }
    
    @java.lang.Override()
    public void onResume() {
    }
    
    @java.lang.Override()
    public void onStop() {
    }
    
    @java.lang.Override()
    public void onDestroyView() {
    }
    
    @java.lang.Override()
    public void onConfigurationChanged(@org.jetbrains.annotations.NotNull()
    android.content.res.Configuration newConfig) {
    }
    
    /**
     * Monitor video URL processing in the fragment
     */
    private final void monitorVideoUrlInFragment(java.lang.String videoId, java.lang.String videoUrl) {
    }
    
    private final int abs(int value) {
        return 0;
    }
    
    /**
     * Start video preparation timeout to detect when callbacks fail
     */
    private final void startVideoPreparationTimeout() {
    }
    
    /**
     * Reset playback state for new attempts
     */
    private final void resetPlaybackState() {
    }
    
    /**
     * Attempt to retry callback if JNI communication failed
     */
    private final void retryCallbackIfNeeded() {
    }
    
    /**
     * Enhanced tryStartPlayback with additional safety checks
     */
    private final void tryStartPlaybackSafely() {
    }
    
    @kotlin.Metadata(mv = {2, 3, 0}, k = 1, xi = 48, d1 = {"\u0000,\n\u0002\u0018\u0002\n\u0002\u0010\u0000\n\u0002\b\u0003\n\u0002\u0010\u000e\n\u0002\b\u0002\n\u0002\u0010\b\n\u0002\b\u0002\n\u0002\u0010\t\n\u0002\b\u0005\n\u0002\u0018\u0002\n\u0002\b\u0003\b\u0086\u0003\u0018\u00002\u00020\u0001B\t\b\u0002\u00a2\u0006\u0004\b\u0002\u0010\u0003J\u001a\u0010\u0010\u001a\u00020\u00112\u0006\u0010\u0012\u001a\u00020\u00052\n\b\u0002\u0010\u0013\u001a\u0004\u0018\u00010\u0005R\u000e\u0010\u0004\u001a\u00020\u0005X\u0082T\u00a2\u0006\u0002\n\u0000R\u000e\u0010\u0006\u001a\u00020\u0005X\u0082T\u00a2\u0006\u0002\n\u0000R\u000e\u0010\u0007\u001a\u00020\bX\u0082T\u00a2\u0006\u0002\n\u0000R\u000e\u0010\t\u001a\u00020\bX\u0082T\u00a2\u0006\u0002\n\u0000R\u000e\u0010\n\u001a\u00020\u000bX\u0082T\u00a2\u0006\u0002\n\u0000R\u000e\u0010\f\u001a\u00020\u000bX\u0082T\u00a2\u0006\u0002\n\u0000R\u000e\u0010\r\u001a\u00020\u000bX\u0082T\u00a2\u0006\u0002\n\u0000R\u000e\u0010\u000e\u001a\u00020\u000bX\u0082T\u00a2\u0006\u0002\n\u0000R\u000e\u0010\u000f\u001a\u00020\bX\u0082T\u00a2\u0006\u0002\n\u0000\u00a8\u0006\u0014"}, d2 = {"Lcom/dorflix/app/ui/VideoPlayerFragment$Companion;", "", "<init>", "()V", "ARG_VIDEO_ID", "", "ARG_VIDEO_URL", "MAX_RETRY_COUNT", "", "PROGRESS_SAVE_INTERVAL", "PLAYBACK_TIMEOUT", "", "WAKE_LOCK_TIMEOUT", "VIDEO_PREPARATION_TIMEOUT", "CALLBACK_RETRY_DELAY", "MAX_CALLBACK_RETRIES", "newInstance", "Lcom/dorflix/app/ui/VideoPlayerFragment;", "videoId", "videoUrl", "DorflixNative_debug"})
    public static final class Companion {
        
        private Companion() {
            super();
        }
        
        @org.jetbrains.annotations.NotNull()
        public final com.dorflix.app.ui.VideoPlayerFragment newInstance(@org.jetbrains.annotations.NotNull()
        java.lang.String videoId, @org.jetbrains.annotations.Nullable()
        java.lang.String videoUrl) {
            return null;
        }
    }
}