package com.dorflix.app.ui;

@kotlin.Metadata(mv = {2, 3, 0}, k = 1, xi = 48, d1 = {"\u0000@\n\u0002\u0018\u0002\n\u0002\u0018\u0002\n\u0002\u0018\u0002\n\u0002\u0018\u0002\n\u0000\n\u0002\u0018\u0002\n\u0000\n\u0002\u0018\u0002\n\u0002\u0010\u0002\n\u0002\b\u0003\n\u0002\u0010\b\n\u0002\b\u0002\n\u0002\u0010%\n\u0002\u0010\u000e\n\u0002\b\u000f\n\u0002\u0018\u0002\n\u0002\b\u0007\u0018\u00002\u0012\u0012\u0004\u0012\u00020\u0002\u0012\b\u0012\u00060\u0003R\u00020\u00000\u0001:\u0001&B#\u0012\u0006\u0010\u0004\u001a\u00020\u0005\u0012\u0012\u0010\u0006\u001a\u000e\u0012\u0004\u0012\u00020\u0002\u0012\u0004\u0012\u00020\b0\u0007\u00a2\u0006\u0004\b\t\u0010\nJ\u000e\u0010\u0014\u001a\u00020\b2\u0006\u0010\u0015\u001a\u00020\fJ\u0010\u0010\u0016\u001a\u00020\b2\u0006\u0010\u0015\u001a\u00020\fH\u0002J\u0010\u0010\u0017\u001a\u00020\b2\u0006\u0010\u0018\u001a\u00020\fH\u0002J\u000e\u0010\u0019\u001a\u00020\b2\u0006\u0010\u0015\u001a\u00020\fJ\u000e\u0010\u001a\u001a\u00020\b2\u0006\u0010\u0015\u001a\u00020\fJ\u000e\u0010\u001b\u001a\u00020\f2\u0006\u0010\u001c\u001a\u00020\u0010J\u0018\u0010\u001d\u001a\u00020\b2\u0006\u0010\u001c\u001a\u00020\u00102\u0006\u0010\u0015\u001a\u00020\fH\u0002J\u001c\u0010\u001e\u001a\u00060\u0003R\u00020\u00002\u0006\u0010\u001f\u001a\u00020 2\u0006\u0010!\u001a\u00020\fH\u0016J\u001c\u0010\"\u001a\u00020\b2\n\u0010#\u001a\u00060\u0003R\u00020\u00002\u0006\u0010\u0015\u001a\u00020\fH\u0016J\u0014\u0010$\u001a\u00020\b2\n\u0010#\u001a\u00060\u0003R\u00020\u0000H\u0016J\u0014\u0010%\u001a\u00020\b2\n\u0010#\u001a\u00060\u0003R\u00020\u0000H\u0016R\u000e\u0010\u0004\u001a\u00020\u0005X\u0082\u0004\u00a2\u0006\u0002\n\u0000R\u001a\u0010\u0006\u001a\u000e\u0012\u0004\u0012\u00020\u0002\u0012\u0004\u0012\u00020\b0\u0007X\u0082\u0004\u00a2\u0006\u0002\n\u0000R\u000e\u0010\u000b\u001a\u00020\fX\u0082\u000e\u00a2\u0006\u0002\n\u0000R\u000e\u0010\r\u001a\u00020\fX\u0082D\u00a2\u0006\u0002\n\u0000R\u001a\u0010\u000e\u001a\u000e\u0012\u0004\u0012\u00020\u0010\u0012\u0004\u0012\u00020\f0\u000fX\u0082\u0004\u00a2\u0006\u0002\n\u0000R\u000e\u0010\u0011\u001a\u00020\fX\u0082\u000e\u00a2\u0006\u0002\n\u0000R\u000e\u0010\u0012\u001a\u00020\fX\u0082\u000e\u00a2\u0006\u0002\n\u0000R\u000e\u0010\u0013\u001a\u00020\fX\u0082\u000e\u00a2\u0006\u0002\n\u0000\u00a8\u0006\'"}, d2 = {"Lcom/dorflix/app/ui/VideoAdapter;", "Landroidx/recyclerview/widget/ListAdapter;", "Lcom/dorflix/app/domain/model/Video;", "Lcom/dorflix/app/ui/VideoAdapter$VideoViewHolder;", "viewPager", "Landroidx/viewpager2/widget/ViewPager2;", "onVideoClick", "Lkotlin/Function1;", "", "<init>", "(Landroidx/viewpager2/widget/ViewPager2;Lkotlin/jvm/functions/Function1;)V", "currentPlayingPosition", "", "SCROLL_BUFFER", "playbackState", "", "", "pendingPlaybackPosition", "pendingPlaybackSeekPosition", "playbackStartedForPosition", "startPlaybackAtPosition", "position", "tryStartPlaybackForPosition", "preloadVideosInBuffer", "currentPosition", "stopPlaybackAtPosition", "pausePlaybackAtPosition", "getSavedPosition", "videoId", "savePosition", "onCreateViewHolder", "parent", "Landroid/view/ViewGroup;", "viewType", "onBindViewHolder", "holder", "onViewAttachedToWindow", "onViewDetachedFromWindow", "VideoViewHolder", "DorflixNative_debug"})
public final class VideoAdapter extends androidx.recyclerview.widget.ListAdapter<com.dorflix.app.domain.model.Video, com.dorflix.app.ui.VideoAdapter.VideoViewHolder> {
    @org.jetbrains.annotations.NotNull()
    private final androidx.viewpager2.widget.ViewPager2 viewPager = null;
    @org.jetbrains.annotations.NotNull()
    private final kotlin.jvm.functions.Function1<com.dorflix.app.domain.model.Video, kotlin.Unit> onVideoClick = null;
    private int currentPlayingPosition = -1;
    private final int SCROLL_BUFFER = 2;
    @org.jetbrains.annotations.NotNull()
    private final java.util.Map<java.lang.String, java.lang.Integer> playbackState = null;
    private int pendingPlaybackPosition = -1;
    private int pendingPlaybackSeekPosition = 0;
    private int playbackStartedForPosition = -1;
    
    public VideoAdapter(@org.jetbrains.annotations.NotNull()
    androidx.viewpager2.widget.ViewPager2 viewPager, @org.jetbrains.annotations.NotNull()
    kotlin.jvm.functions.Function1<? super com.dorflix.app.domain.model.Video, kotlin.Unit> onVideoClick) {
        super(null);
    }
    
    public final void startPlaybackAtPosition(int position) {
    }
    
    private final void tryStartPlaybackForPosition(int position) {
    }
    
    private final void preloadVideosInBuffer(int currentPosition) {
    }
    
    public final void stopPlaybackAtPosition(int position) {
    }
    
    public final void pausePlaybackAtPosition(int position) {
    }
    
    public final int getSavedPosition(@org.jetbrains.annotations.NotNull()
    java.lang.String videoId) {
        return 0;
    }
    
    private final void savePosition(java.lang.String videoId, int position) {
    }
    
    @java.lang.Override()
    @org.jetbrains.annotations.NotNull()
    public com.dorflix.app.ui.VideoAdapter.VideoViewHolder onCreateViewHolder(@org.jetbrains.annotations.NotNull()
    android.view.ViewGroup parent, int viewType) {
        return null;
    }
    
    @java.lang.Override()
    public void onBindViewHolder(@org.jetbrains.annotations.NotNull()
    com.dorflix.app.ui.VideoAdapter.VideoViewHolder holder, int position) {
    }
    
    @java.lang.Override()
    public void onViewAttachedToWindow(@org.jetbrains.annotations.NotNull()
    com.dorflix.app.ui.VideoAdapter.VideoViewHolder holder) {
    }
    
    @java.lang.Override()
    public void onViewDetachedFromWindow(@org.jetbrains.annotations.NotNull()
    com.dorflix.app.ui.VideoAdapter.VideoViewHolder holder) {
    }
    
    @kotlin.Metadata(mv = {2, 3, 0}, k = 1, xi = 48, d1 = {"\u0000L\n\u0002\u0018\u0002\n\u0002\u0018\u0002\n\u0000\n\u0002\u0018\u0002\n\u0002\b\u0005\n\u0002\u0018\u0002\n\u0000\n\u0002\u0018\u0002\n\u0000\n\u0002\u0010\u000b\n\u0002\b\u0005\n\u0002\u0010\b\n\u0000\n\u0002\u0018\u0002\n\u0002\b\u0002\n\u0002\u0010\u0002\n\u0002\b\b\n\u0002\u0018\u0002\n\u0002\b\u0003\n\u0002\u0010\u000e\n\u0002\b\t\b\u0086\u0004\u0018\u00002\u00020\u0001B\u000f\u0012\u0006\u0010\u0002\u001a\u00020\u0003\u00a2\u0006\u0004\b\u0004\u0010\u0005J\b\u0010\u0017\u001a\u00020\u0018H\u0002J\u0012\u0010\u0019\u001a\u00020\u00182\b\b\u0002\u0010\u001a\u001a\u00020\rH\u0002J\b\u0010\u001b\u001a\u00020\u0018H\u0002J\u0016\u0010\u001c\u001a\u00020\u00182\u0006\u0010\u001d\u001a\u00020\u000b2\u0006\u0010\u001e\u001a\u00020\u0013J \u0010\u001f\u001a\u00020\u00182\u0006\u0010\u001d\u001a\u00020\u000b2\u0006\u0010 \u001a\u00020!2\b\b\u0002\u0010\"\u001a\u00020\u0013J\u0018\u0010#\u001a\u00020\u00182\u0006\u0010$\u001a\u00020%2\u0006\u0010 \u001a\u00020!H\u0002J\u0010\u0010&\u001a\u00020\u00182\u0006\u0010 \u001a\u00020!H\u0002J\b\u0010\'\u001a\u00020\u0018H\u0002J\u0006\u0010(\u001a\u00020\u0018J\u0006\u0010)\u001a\u00020\u0018J\u0006\u0010*\u001a\u00020\u0018J\u0006\u0010+\u001a\u00020\u0013J\u0006\u0010,\u001a\u00020\u0018J\u0006\u0010-\u001a\u00020\u0018R\u0011\u0010\u0002\u001a\u00020\u0003\u00a2\u0006\b\n\u0000\u001a\u0004\b\u0006\u0010\u0007R\u0010\u0010\b\u001a\u0004\u0018\u00010\tX\u0082\u000e\u00a2\u0006\u0002\n\u0000R\u0010\u0010\n\u001a\u0004\u0018\u00010\u000bX\u0082\u000e\u00a2\u0006\u0002\n\u0000R\u000e\u0010\f\u001a\u00020\rX\u0082\u000e\u00a2\u0006\u0002\n\u0000R\u000e\u0010\u000e\u001a\u00020\rX\u0082\u000e\u00a2\u0006\u0002\n\u0000R\u000e\u0010\u000f\u001a\u00020\rX\u0082\u000e\u00a2\u0006\u0002\n\u0000R\u000e\u0010\u0010\u001a\u00020\rX\u0082\u000e\u00a2\u0006\u0002\n\u0000R\u000e\u0010\u0011\u001a\u00020\rX\u0082\u000e\u00a2\u0006\u0002\n\u0000R\u000e\u0010\u0012\u001a\u00020\u0013X\u0082\u000e\u00a2\u0006\u0002\n\u0000R\u0010\u0010\u0014\u001a\u00020\u0015X\u0082\u0004\u00a2\u0006\u0004\n\u0002\u0010\u0016\u00a8\u0006."}, d2 = {"Lcom/dorflix/app/ui/VideoAdapter$VideoViewHolder;", "Landroidx/recyclerview/widget/RecyclerView$ViewHolder;", "binding", "Lcom/dorflix/app/databinding/ItemVideoBinding;", "<init>", "(Lcom/dorflix/app/ui/VideoAdapter;Lcom/dorflix/app/databinding/ItemVideoBinding;)V", "getBinding", "()Lcom/dorflix/app/databinding/ItemVideoBinding;", "videoPlayerController", "Lcom/dorflix/app/video/VideoPlayerController;", "currentVideo", "Lcom/dorflix/app/domain/model/Video;", "isPlaying", "", "isViewAttached", "pendingPlaybackResume", "isVideoLoaded", "isSurfaceReady", "pendingSeekPosition", "", "playerListener", "Lcom/dorflix/app/video/VideoPlayerListener;", "Lcom/dorflix/app/video/VideoPlayerListener;", "togglePlayPause", "", "showPlayPauseButton", "shouldFadeOut", "fadeOutPlayPauseButton", "bind", "video", "currentPlayingPosition", "startPlayback", "surface", "Landroid/view/Surface;", "seekToPosition", "createAndStartPlayback", "videoUrl", "", "checkAndStartPlayback", "resetPlaybackState", "stopPlayback", "pausePlayback", "resumePlayback", "getCurrentPosition", "onViewAttachedToWindow", "onViewDetachedFromWindow", "DorflixNative_debug"})
    public final class VideoViewHolder extends androidx.recyclerview.widget.RecyclerView.ViewHolder {
        @org.jetbrains.annotations.NotNull()
        private final com.dorflix.app.databinding.ItemVideoBinding binding = null;
        @org.jetbrains.annotations.Nullable()
        private com.dorflix.app.video.VideoPlayerController videoPlayerController;
        @org.jetbrains.annotations.Nullable()
        private com.dorflix.app.domain.model.Video currentVideo;
        private boolean isPlaying = false;
        private boolean isViewAttached = false;
        private boolean pendingPlaybackResume = false;
        private boolean isVideoLoaded = false;
        private boolean isSurfaceReady = false;
        private int pendingSeekPosition = 0;
        @org.jetbrains.annotations.NotNull()
        private final com.dorflix.app.video.VideoPlayerListener playerListener = null;
        
        public VideoViewHolder(@org.jetbrains.annotations.NotNull()
        com.dorflix.app.databinding.ItemVideoBinding binding) {
            super(null);
        }
        
        @org.jetbrains.annotations.NotNull()
        public final com.dorflix.app.databinding.ItemVideoBinding getBinding() {
            return null;
        }
        
        private final void togglePlayPause() {
        }
        
        private final void showPlayPauseButton(boolean shouldFadeOut) {
        }
        
        private final void fadeOutPlayPauseButton() {
        }
        
        public final void bind(@org.jetbrains.annotations.NotNull()
        com.dorflix.app.domain.model.Video video, int currentPlayingPosition) {
        }
        
        public final void startPlayback(@org.jetbrains.annotations.NotNull()
        com.dorflix.app.domain.model.Video video, @org.jetbrains.annotations.NotNull()
        android.view.Surface surface, int seekToPosition) {
        }
        
        private final void createAndStartPlayback(java.lang.String videoUrl, android.view.Surface surface) {
        }
        
        private final void checkAndStartPlayback(android.view.Surface surface) {
        }
        
        private final void resetPlaybackState() {
        }
        
        public final void stopPlayback() {
        }
        
        public final void pausePlayback() {
        }
        
        public final void resumePlayback() {
        }
        
        public final int getCurrentPosition() {
            return 0;
        }
        
        public final void onViewAttachedToWindow() {
        }
        
        public final void onViewDetachedFromWindow() {
        }
    }
}