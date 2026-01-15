package com.dorflix.app.presentation.viewmodel;

/**
 * ViewModel for video-related UI operations
 */
@kotlin.Metadata(mv = {2, 3, 0}, k = 1, xi = 48, d1 = {"\u0000v\n\u0002\u0018\u0002\n\u0002\u0018\u0002\n\u0002\b\u0003\n\u0002\u0018\u0002\n\u0000\n\u0002\u0010\t\n\u0000\n\u0002\u0010\u000b\n\u0002\b\u0005\n\u0002\u0018\u0002\n\u0002\u0010 \n\u0002\u0018\u0002\n\u0000\n\u0002\u0018\u0002\n\u0002\b\t\n\u0002\u0018\u0002\n\u0002\b\t\n\u0002\u0018\u0002\n\u0002\b\u0003\n\u0002\u0018\u0002\n\u0002\b\u0005\n\u0002\u0010\u000e\n\u0002\b\u0003\n\u0002\u0018\u0002\n\u0002\b\u0002\n\u0002\u0010\u0002\n\u0002\b\u0002\n\u0002\u0010\b\n\u0002\b\u0005\n\u0002\u0018\u0002\n\u0002\b\u0014\u0018\u00002\u00020\u0001:\u0001TB\u0007\u00a2\u0006\u0004\b\u0002\u0010\u0003J\u0006\u00107\u001a\u000208J\u0010\u00109\u001a\u0002082\b\b\u0002\u0010:\u001a\u00020;J\u000e\u0010<\u001a\u0002082\u0006\u0010=\u001a\u000201J\u000e\u0010>\u001a\u0002082\u0006\u0010?\u001a\u000201J\u0018\u0010@\u001a\u00020A2\u0006\u0010?\u001a\u0002012\u0006\u0010B\u001a\u000201H\u0002J\u0010\u0010C\u001a\u0002082\u0006\u0010D\u001a\u00020AH\u0002J\u0010\u0010E\u001a\u0002082\u0006\u0010?\u001a\u000201H\u0002J\u001e\u0010F\u001a\u0002082\u0006\u0010?\u001a\u0002012\u0006\u0010G\u001a\u00020;2\u0006\u0010H\u001a\u00020;J\u0006\u0010I\u001a\u000208J\u0006\u0010J\u001a\u000208J\u000e\u0010K\u001a\u0002082\u0006\u0010?\u001a\u000201J\u000e\u0010L\u001a\u0002082\u0006\u0010?\u001a\u000201J\u001a\u0010M\u001a\u0002082\u0006\u0010?\u001a\u0002012\n\b\u0002\u0010N\u001a\u0004\u0018\u000101J\u0016\u0010O\u001a\u0002082\u0006\u0010?\u001a\u0002012\u0006\u0010P\u001a\u000201J\u0006\u0010Q\u001a\u000208J\u0006\u0010R\u001a\u000208J\u0006\u0010-\u001a\u000208J\u0006\u0010S\u001a\u000208R\u000e\u0010\u0004\u001a\u00020\u0005X\u0082\u0004\u00a2\u0006\u0002\n\u0000R\u000e\u0010\u0006\u001a\u00020\u0007X\u0082\u000e\u00a2\u0006\u0002\n\u0000R\u001a\u0010\b\u001a\u00020\tX\u0086\u000e\u00a2\u0006\u000e\n\u0000\u001a\u0004\b\n\u0010\u000b\"\u0004\b\f\u0010\rR\u001a\u0010\u000e\u001a\u000e\u0012\n\u0012\b\u0012\u0004\u0012\u00020\u00110\u00100\u000fX\u0082\u0004\u00a2\u0006\u0002\n\u0000R\u001d\u0010\u0012\u001a\u000e\u0012\n\u0012\b\u0012\u0004\u0012\u00020\u00110\u00100\u0013\u00a2\u0006\b\n\u0000\u001a\u0004\b\u0014\u0010\u0015R\u001a\u0010\u0016\u001a\u000e\u0012\n\u0012\b\u0012\u0004\u0012\u00020\u00110\u00100\u000fX\u0082\u0004\u00a2\u0006\u0002\n\u0000R\u001d\u0010\u0017\u001a\u000e\u0012\n\u0012\b\u0012\u0004\u0012\u00020\u00110\u00100\u0013\u00a2\u0006\b\n\u0000\u001a\u0004\b\u0018\u0010\u0015R\u0016\u0010\u0019\u001a\n\u0012\u0006\u0012\u0004\u0018\u00010\u00110\u000fX\u0082\u0004\u00a2\u0006\u0002\n\u0000R\u0019\u0010\u001a\u001a\n\u0012\u0006\u0012\u0004\u0018\u00010\u00110\u0013\u00a2\u0006\b\n\u0000\u001a\u0004\b\u001b\u0010\u0015R\u0016\u0010\u001c\u001a\n\u0012\u0006\u0012\u0004\u0018\u00010\u001d0\u000fX\u0082\u0004\u00a2\u0006\u0002\n\u0000R\u0019\u0010\u001e\u001a\n\u0012\u0006\u0012\u0004\u0018\u00010\u001d0\u0013\u00a2\u0006\b\n\u0000\u001a\u0004\b\u001f\u0010\u0015R\u001a\u0010 \u001a\u000e\u0012\n\u0012\b\u0012\u0004\u0012\u00020\u00110\u00100\u000fX\u0082\u0004\u00a2\u0006\u0002\n\u0000R\u001d\u0010!\u001a\u000e\u0012\n\u0012\b\u0012\u0004\u0012\u00020\u00110\u00100\u0013\u00a2\u0006\b\n\u0000\u001a\u0004\b\"\u0010\u0015R\u001a\u0010#\u001a\u000e\u0012\n\u0012\b\u0012\u0004\u0012\u00020\u00110\u00100\u000fX\u0082\u0004\u00a2\u0006\u0002\n\u0000R\u001d\u0010$\u001a\u000e\u0012\n\u0012\b\u0012\u0004\u0012\u00020\u00110\u00100\u0013\u00a2\u0006\b\n\u0000\u001a\u0004\b%\u0010\u0015R\u0016\u0010&\u001a\n\u0012\u0006\u0012\u0004\u0018\u00010\'0\u000fX\u0082\u0004\u00a2\u0006\u0002\n\u0000R\u0019\u0010(\u001a\n\u0012\u0006\u0012\u0004\u0018\u00010\'0\u0013\u00a2\u0006\b\n\u0000\u001a\u0004\b)\u0010\u0015R\u0016\u0010*\u001a\n\u0012\u0006\u0012\u0004\u0018\u00010+0\u000fX\u0082\u0004\u00a2\u0006\u0002\n\u0000R\u0019\u0010,\u001a\n\u0012\u0006\u0012\u0004\u0018\u00010+0\u0013\u00a2\u0006\b\n\u0000\u001a\u0004\b-\u0010\u0015R\u0014\u0010.\u001a\b\u0012\u0004\u0012\u00020\t0\u000fX\u0082\u0004\u00a2\u0006\u0002\n\u0000R\u0017\u0010/\u001a\b\u0012\u0004\u0012\u00020\t0\u0013\u00a2\u0006\b\n\u0000\u001a\u0004\b/\u0010\u0015R\u0016\u00100\u001a\n\u0012\u0006\u0012\u0004\u0018\u0001010\u000fX\u0082\u0004\u00a2\u0006\u0002\n\u0000R\u0019\u00102\u001a\n\u0012\u0006\u0012\u0004\u0018\u0001010\u0013\u00a2\u0006\b\n\u0000\u001a\u0004\b3\u0010\u0015R\u0017\u00104\u001a\b\u0012\u0004\u0012\u0002050\u0013\u00a2\u0006\b\n\u0000\u001a\u0004\b6\u0010\u0015\u00a8\u0006U"}, d2 = {"Lcom/dorflix/app/presentation/viewmodel/VideoViewModel;", "Landroidx/lifecycle/ViewModel;", "<init>", "()V", "repository", "Lcom/dorflix/app/data/repository/VideoRepository;", "lastMonitorTime", "", "autoPlayAttempted", "", "getAutoPlayAttempted", "()Z", "setAutoPlayAttempted", "(Z)V", "_videoCatalog", "Lkotlinx/coroutines/flow/MutableStateFlow;", "", "Lcom/dorflix/app/domain/model/Video;", "videoCatalog", "Lkotlinx/coroutines/flow/StateFlow;", "getVideoCatalog", "()Lkotlinx/coroutines/flow/StateFlow;", "_searchResults", "searchResults", "getSearchResults", "_currentVideo", "currentVideo", "getCurrentVideo", "_watchProgress", "Lcom/dorflix/app/domain/model/WatchProgress;", "watchProgress", "getWatchProgress", "_continueWatching", "continueWatching", "getContinueWatching", "_recommendations", "recommendations", "getRecommendations", "_watchStatistics", "Lcom/dorflix/app/domain/model/VideoStatistics;", "watchStatistics", "getWatchStatistics", "_socialActivity", "Lcom/dorflix/app/domain/model/SocialActivityResponse;", "socialActivity", "getSocialActivity", "_isLoading", "isLoading", "_error", "", "error", "getError", "uiState", "Lcom/dorflix/app/presentation/viewmodel/VideoUiState;", "getUiState", "loadInitialCatalog", "", "loadVideoCatalog", "offset", "", "searchVideos", "query", "loadVideoDetails", "videoId", "monitorVideoUrl", "Lcom/dorflix/app/presentation/viewmodel/VideoViewModel$VideoUrlStatus;", "videoUrl", "logVideoUrlStatus", "status", "loadWatchProgress", "saveWatchProgress", "progressSeconds", "totalDuration", "loadContinueWatching", "loadRecommendations", "likeVideo", "unlikeVideo", "shareVideo", "platform", "addComment", "commentText", "clearError", "loadWatchStatistics", "clearSearch", "VideoUrlStatus", "DorflixNative_debug"})
public final class VideoViewModel extends androidx.lifecycle.ViewModel {
    @org.jetbrains.annotations.NotNull()
    private final com.dorflix.app.data.repository.VideoRepository repository = null;
    private long lastMonitorTime = 0L;
    private boolean autoPlayAttempted = false;
    @org.jetbrains.annotations.NotNull()
    private final kotlinx.coroutines.flow.MutableStateFlow<java.util.List<com.dorflix.app.domain.model.Video>> _videoCatalog = null;
    @org.jetbrains.annotations.NotNull()
    private final kotlinx.coroutines.flow.StateFlow<java.util.List<com.dorflix.app.domain.model.Video>> videoCatalog = null;
    @org.jetbrains.annotations.NotNull()
    private final kotlinx.coroutines.flow.MutableStateFlow<java.util.List<com.dorflix.app.domain.model.Video>> _searchResults = null;
    @org.jetbrains.annotations.NotNull()
    private final kotlinx.coroutines.flow.StateFlow<java.util.List<com.dorflix.app.domain.model.Video>> searchResults = null;
    @org.jetbrains.annotations.NotNull()
    private final kotlinx.coroutines.flow.MutableStateFlow<com.dorflix.app.domain.model.Video> _currentVideo = null;
    @org.jetbrains.annotations.NotNull()
    private final kotlinx.coroutines.flow.StateFlow<com.dorflix.app.domain.model.Video> currentVideo = null;
    @org.jetbrains.annotations.NotNull()
    private final kotlinx.coroutines.flow.MutableStateFlow<com.dorflix.app.domain.model.WatchProgress> _watchProgress = null;
    @org.jetbrains.annotations.NotNull()
    private final kotlinx.coroutines.flow.StateFlow<com.dorflix.app.domain.model.WatchProgress> watchProgress = null;
    @org.jetbrains.annotations.NotNull()
    private final kotlinx.coroutines.flow.MutableStateFlow<java.util.List<com.dorflix.app.domain.model.Video>> _continueWatching = null;
    @org.jetbrains.annotations.NotNull()
    private final kotlinx.coroutines.flow.StateFlow<java.util.List<com.dorflix.app.domain.model.Video>> continueWatching = null;
    @org.jetbrains.annotations.NotNull()
    private final kotlinx.coroutines.flow.MutableStateFlow<java.util.List<com.dorflix.app.domain.model.Video>> _recommendations = null;
    @org.jetbrains.annotations.NotNull()
    private final kotlinx.coroutines.flow.StateFlow<java.util.List<com.dorflix.app.domain.model.Video>> recommendations = null;
    @org.jetbrains.annotations.NotNull()
    private final kotlinx.coroutines.flow.MutableStateFlow<com.dorflix.app.domain.model.VideoStatistics> _watchStatistics = null;
    @org.jetbrains.annotations.NotNull()
    private final kotlinx.coroutines.flow.StateFlow<com.dorflix.app.domain.model.VideoStatistics> watchStatistics = null;
    @org.jetbrains.annotations.NotNull()
    private final kotlinx.coroutines.flow.MutableStateFlow<com.dorflix.app.domain.model.SocialActivityResponse> _socialActivity = null;
    @org.jetbrains.annotations.NotNull()
    private final kotlinx.coroutines.flow.StateFlow<com.dorflix.app.domain.model.SocialActivityResponse> socialActivity = null;
    @org.jetbrains.annotations.NotNull()
    private final kotlinx.coroutines.flow.MutableStateFlow<java.lang.Boolean> _isLoading = null;
    @org.jetbrains.annotations.NotNull()
    private final kotlinx.coroutines.flow.StateFlow<java.lang.Boolean> isLoading = null;
    @org.jetbrains.annotations.NotNull()
    private final kotlinx.coroutines.flow.MutableStateFlow<java.lang.String> _error = null;
    @org.jetbrains.annotations.NotNull()
    private final kotlinx.coroutines.flow.StateFlow<java.lang.String> error = null;
    @org.jetbrains.annotations.NotNull()
    private final kotlinx.coroutines.flow.StateFlow<com.dorflix.app.presentation.viewmodel.VideoUiState> uiState = null;
    
    public VideoViewModel() {
        super();
    }
    
    public final boolean getAutoPlayAttempted() {
        return false;
    }
    
    public final void setAutoPlayAttempted(boolean p0) {
    }
    
    @org.jetbrains.annotations.NotNull()
    public final kotlinx.coroutines.flow.StateFlow<java.util.List<com.dorflix.app.domain.model.Video>> getVideoCatalog() {
        return null;
    }
    
    @org.jetbrains.annotations.NotNull()
    public final kotlinx.coroutines.flow.StateFlow<java.util.List<com.dorflix.app.domain.model.Video>> getSearchResults() {
        return null;
    }
    
    @org.jetbrains.annotations.NotNull()
    public final kotlinx.coroutines.flow.StateFlow<com.dorflix.app.domain.model.Video> getCurrentVideo() {
        return null;
    }
    
    @org.jetbrains.annotations.NotNull()
    public final kotlinx.coroutines.flow.StateFlow<com.dorflix.app.domain.model.WatchProgress> getWatchProgress() {
        return null;
    }
    
    @org.jetbrains.annotations.NotNull()
    public final kotlinx.coroutines.flow.StateFlow<java.util.List<com.dorflix.app.domain.model.Video>> getContinueWatching() {
        return null;
    }
    
    @org.jetbrains.annotations.NotNull()
    public final kotlinx.coroutines.flow.StateFlow<java.util.List<com.dorflix.app.domain.model.Video>> getRecommendations() {
        return null;
    }
    
    @org.jetbrains.annotations.NotNull()
    public final kotlinx.coroutines.flow.StateFlow<com.dorflix.app.domain.model.VideoStatistics> getWatchStatistics() {
        return null;
    }
    
    @org.jetbrains.annotations.NotNull()
    public final kotlinx.coroutines.flow.StateFlow<com.dorflix.app.domain.model.SocialActivityResponse> getSocialActivity() {
        return null;
    }
    
    @org.jetbrains.annotations.NotNull()
    public final kotlinx.coroutines.flow.StateFlow<java.lang.Boolean> isLoading() {
        return null;
    }
    
    @org.jetbrains.annotations.NotNull()
    public final kotlinx.coroutines.flow.StateFlow<java.lang.String> getError() {
        return null;
    }
    
    @org.jetbrains.annotations.NotNull()
    public final kotlinx.coroutines.flow.StateFlow<com.dorflix.app.presentation.viewmodel.VideoUiState> getUiState() {
        return null;
    }
    
    /**
     * Load initial video catalog
     */
    public final void loadInitialCatalog() {
    }
    
    /**
     * Load video catalog with pagination
     */
    public final void loadVideoCatalog(int offset) {
    }
    
    /**
     * Search videos
     */
    public final void searchVideos(@org.jetbrains.annotations.NotNull()
    java.lang.String query) {
    }
    
    /**
     * Load video details
     */
    public final void loadVideoDetails(@org.jetbrains.annotations.NotNull()
    java.lang.String videoId) {
    }
    
    /**
     * Monitor video URL availability and format - SILENT METHOD
     * Returns monitoring results without any logging to prevent flow restarts
     */
    private final com.dorflix.app.presentation.viewmodel.VideoViewModel.VideoUrlStatus monitorVideoUrl(java.lang.String videoId, java.lang.String videoUrl) {
        return null;
    }
    
    /**
     * Log video URL monitoring results outside the flow emission
     */
    private final void logVideoUrlStatus(com.dorflix.app.presentation.viewmodel.VideoViewModel.VideoUrlStatus status) {
    }
    
    /**
     * Load watch progress for current video
     */
    private final void loadWatchProgress(java.lang.String videoId) {
    }
    
    /**
     * Save watch progress
     */
    public final void saveWatchProgress(@org.jetbrains.annotations.NotNull()
    java.lang.String videoId, int progressSeconds, int totalDuration) {
    }
    
    /**
     * Load continue watching videos
     */
    public final void loadContinueWatching() {
    }
    
    /**
     * Load video recommendations
     */
    public final void loadRecommendations() {
    }
    
    /**
     * Like a video
     */
    public final void likeVideo(@org.jetbrains.annotations.NotNull()
    java.lang.String videoId) {
    }
    
    /**
     * Unlike a video
     */
    public final void unlikeVideo(@org.jetbrains.annotations.NotNull()
    java.lang.String videoId) {
    }
    
    /**
     * Share a video
     */
    public final void shareVideo(@org.jetbrains.annotations.NotNull()
    java.lang.String videoId, @org.jetbrains.annotations.Nullable()
    java.lang.String platform) {
    }
    
    /**
     * Add comment to video
     */
    public final void addComment(@org.jetbrains.annotations.NotNull()
    java.lang.String videoId, @org.jetbrains.annotations.NotNull()
    java.lang.String commentText) {
    }
    
    /**
     * Clear error state
     */
    public final void clearError() {
    }
    
    /**
     * Load watch statistics
     */
    public final void loadWatchStatistics() {
    }
    
    /**
     * Get social activity
     */
    public final void getSocialActivity() {
    }
    
    /**
     * Reset search results
     */
    public final void clearSearch() {
    }
    
    /**
     * Data class for video URL monitoring results
     */
    @kotlin.Metadata(mv = {2, 3, 0}, k = 1, xi = 48, d1 = {"\u0000 \n\u0002\u0018\u0002\n\u0002\u0010\u0000\n\u0000\n\u0002\u0010\u000e\n\u0000\n\u0002\u0010\u000b\n\u0002\b\u0002\n\u0002\u0010\b\n\u0002\b\u0014\b\u0082\b\u0018\u00002\u00020\u0001B/\u0012\u0006\u0010\u0002\u001a\u00020\u0003\u0012\u0006\u0010\u0004\u001a\u00020\u0005\u0012\u0006\u0010\u0006\u001a\u00020\u0005\u0012\u0006\u0010\u0007\u001a\u00020\b\u0012\u0006\u0010\t\u001a\u00020\u0005\u00a2\u0006\u0004\b\n\u0010\u000bJ\t\u0010\u0012\u001a\u00020\u0003H\u00c6\u0003J\t\u0010\u0013\u001a\u00020\u0005H\u00c6\u0003J\t\u0010\u0014\u001a\u00020\u0005H\u00c6\u0003J\t\u0010\u0015\u001a\u00020\bH\u00c6\u0003J\t\u0010\u0016\u001a\u00020\u0005H\u00c6\u0003J;\u0010\u0017\u001a\u00020\u00002\b\b\u0002\u0010\u0002\u001a\u00020\u00032\b\b\u0002\u0010\u0004\u001a\u00020\u00052\b\b\u0002\u0010\u0006\u001a\u00020\u00052\b\b\u0002\u0010\u0007\u001a\u00020\b2\b\b\u0002\u0010\t\u001a\u00020\u0005H\u00c6\u0001J\u0014\u0010\u0018\u001a\u00020\u00052\b\u0010\u0019\u001a\u0004\u0018\u00010\u0001H\u00d6\u0083\u0004J\n\u0010\u001a\u001a\u00020\bH\u00d6\u0081\u0004J\n\u0010\u001b\u001a\u00020\u0003H\u00d6\u0081\u0004R\u0011\u0010\u0002\u001a\u00020\u0003\u00a2\u0006\b\n\u0000\u001a\u0004\b\f\u0010\rR\u0011\u0010\u0004\u001a\u00020\u0005\u00a2\u0006\b\n\u0000\u001a\u0004\b\u0004\u0010\u000eR\u0011\u0010\u0006\u001a\u00020\u0005\u00a2\u0006\b\n\u0000\u001a\u0004\b\u0006\u0010\u000eR\u0011\u0010\u0007\u001a\u00020\b\u00a2\u0006\b\n\u0000\u001a\u0004\b\u000f\u0010\u0010R\u0011\u0010\t\u001a\u00020\u0005\u00a2\u0006\b\n\u0000\u001a\u0004\b\u0011\u0010\u000e\u00a8\u0006\u001c"}, d2 = {"Lcom/dorflix/app/presentation/viewmodel/VideoViewModel$VideoUrlStatus;", "", "videoId", "", "isUrlPresent", "", "isMp4Format", "urlLength", "", "hasValidProtocol", "<init>", "(Ljava/lang/String;ZZIZ)V", "getVideoId", "()Ljava/lang/String;", "()Z", "getUrlLength", "()I", "getHasValidProtocol", "component1", "component2", "component3", "component4", "component5", "copy", "equals", "other", "hashCode", "toString", "DorflixNative_debug"})
    static final class VideoUrlStatus {
        @org.jetbrains.annotations.NotNull()
        private final java.lang.String videoId = null;
        private final boolean isUrlPresent = false;
        private final boolean isMp4Format = false;
        private final int urlLength = 0;
        private final boolean hasValidProtocol = false;
        
        public VideoUrlStatus(@org.jetbrains.annotations.NotNull()
        java.lang.String videoId, boolean isUrlPresent, boolean isMp4Format, int urlLength, boolean hasValidProtocol) {
            super();
        }
        
        @org.jetbrains.annotations.NotNull()
        public final java.lang.String getVideoId() {
            return null;
        }
        
        public final boolean isUrlPresent() {
            return false;
        }
        
        public final boolean isMp4Format() {
            return false;
        }
        
        public final int getUrlLength() {
            return 0;
        }
        
        public final boolean getHasValidProtocol() {
            return false;
        }
        
        @org.jetbrains.annotations.NotNull()
        public final java.lang.String component1() {
            return null;
        }
        
        public final boolean component2() {
            return false;
        }
        
        public final boolean component3() {
            return false;
        }
        
        public final int component4() {
            return 0;
        }
        
        public final boolean component5() {
            return false;
        }
        
        @org.jetbrains.annotations.NotNull()
        public final com.dorflix.app.presentation.viewmodel.VideoViewModel.VideoUrlStatus copy(@org.jetbrains.annotations.NotNull()
        java.lang.String videoId, boolean isUrlPresent, boolean isMp4Format, int urlLength, boolean hasValidProtocol) {
            return null;
        }
        
        @java.lang.Override()
        public boolean equals(@org.jetbrains.annotations.Nullable()
        java.lang.Object other) {
            return false;
        }
        
        @java.lang.Override()
        public int hashCode() {
            return 0;
        }
        
        @java.lang.Override()
        @org.jetbrains.annotations.NotNull()
        public java.lang.String toString() {
            return null;
        }
    }
}