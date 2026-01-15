package com.dorflix.app.presentation.viewmodel;

/**
 * UI State for video operations
 */
@kotlin.Metadata(mv = {2, 3, 0}, k = 1, xi = 48, d1 = {"\u0000,\n\u0002\u0018\u0002\n\u0002\u0010\u0000\n\u0000\n\u0002\u0010\u000b\n\u0000\n\u0002\u0010\u000e\n\u0000\n\u0002\u0018\u0002\n\u0000\n\u0002\u0018\u0002\n\u0002\b\u0011\n\u0002\u0010\b\n\u0002\b\u0002\b\u0086\b\u0018\u00002\u00020\u0001B5\u0012\b\b\u0002\u0010\u0002\u001a\u00020\u0003\u0012\n\b\u0002\u0010\u0004\u001a\u0004\u0018\u00010\u0005\u0012\n\b\u0002\u0010\u0006\u001a\u0004\u0018\u00010\u0007\u0012\n\b\u0002\u0010\b\u001a\u0004\u0018\u00010\t\u00a2\u0006\u0004\b\n\u0010\u000bJ\t\u0010\u0013\u001a\u00020\u0003H\u00c6\u0003J\u000b\u0010\u0014\u001a\u0004\u0018\u00010\u0005H\u00c6\u0003J\u000b\u0010\u0015\u001a\u0004\u0018\u00010\u0007H\u00c6\u0003J\u000b\u0010\u0016\u001a\u0004\u0018\u00010\tH\u00c6\u0003J7\u0010\u0017\u001a\u00020\u00002\b\b\u0002\u0010\u0002\u001a\u00020\u00032\n\b\u0002\u0010\u0004\u001a\u0004\u0018\u00010\u00052\n\b\u0002\u0010\u0006\u001a\u0004\u0018\u00010\u00072\n\b\u0002\u0010\b\u001a\u0004\u0018\u00010\tH\u00c6\u0001J\u0014\u0010\u0018\u001a\u00020\u00032\b\u0010\u0019\u001a\u0004\u0018\u00010\u0001H\u00d6\u0083\u0004J\n\u0010\u001a\u001a\u00020\u001bH\u00d6\u0081\u0004J\n\u0010\u001c\u001a\u00020\u0005H\u00d6\u0081\u0004R\u0011\u0010\u0002\u001a\u00020\u0003\u00a2\u0006\b\n\u0000\u001a\u0004\b\u0002\u0010\fR\u0013\u0010\u0004\u001a\u0004\u0018\u00010\u0005\u00a2\u0006\b\n\u0000\u001a\u0004\b\r\u0010\u000eR\u0013\u0010\u0006\u001a\u0004\u0018\u00010\u0007\u00a2\u0006\b\n\u0000\u001a\u0004\b\u000f\u0010\u0010R\u0013\u0010\b\u001a\u0004\u0018\u00010\t\u00a2\u0006\b\n\u0000\u001a\u0004\b\u0011\u0010\u0012\u00a8\u0006\u001d"}, d2 = {"Lcom/dorflix/app/presentation/viewmodel/VideoUiState;", "", "isLoading", "", "error", "", "currentVideo", "Lcom/dorflix/app/domain/model/Video;", "watchProgress", "Lcom/dorflix/app/domain/model/WatchProgress;", "<init>", "(ZLjava/lang/String;Lcom/dorflix/app/domain/model/Video;Lcom/dorflix/app/domain/model/WatchProgress;)V", "()Z", "getError", "()Ljava/lang/String;", "getCurrentVideo", "()Lcom/dorflix/app/domain/model/Video;", "getWatchProgress", "()Lcom/dorflix/app/domain/model/WatchProgress;", "component1", "component2", "component3", "component4", "copy", "equals", "other", "hashCode", "", "toString", "DorflixNative_debug"})
public final class VideoUiState {
    private final boolean isLoading = false;
    @org.jetbrains.annotations.Nullable()
    private final java.lang.String error = null;
    @org.jetbrains.annotations.Nullable()
    private final com.dorflix.app.domain.model.Video currentVideo = null;
    @org.jetbrains.annotations.Nullable()
    private final com.dorflix.app.domain.model.WatchProgress watchProgress = null;
    
    public VideoUiState(boolean isLoading, @org.jetbrains.annotations.Nullable()
    java.lang.String error, @org.jetbrains.annotations.Nullable()
    com.dorflix.app.domain.model.Video currentVideo, @org.jetbrains.annotations.Nullable()
    com.dorflix.app.domain.model.WatchProgress watchProgress) {
        super();
    }
    
    public final boolean isLoading() {
        return false;
    }
    
    @org.jetbrains.annotations.Nullable()
    public final java.lang.String getError() {
        return null;
    }
    
    @org.jetbrains.annotations.Nullable()
    public final com.dorflix.app.domain.model.Video getCurrentVideo() {
        return null;
    }
    
    @org.jetbrains.annotations.Nullable()
    public final com.dorflix.app.domain.model.WatchProgress getWatchProgress() {
        return null;
    }
    
    public VideoUiState() {
        super();
    }
    
    public final boolean component1() {
        return false;
    }
    
    @org.jetbrains.annotations.Nullable()
    public final java.lang.String component2() {
        return null;
    }
    
    @org.jetbrains.annotations.Nullable()
    public final com.dorflix.app.domain.model.Video component3() {
        return null;
    }
    
    @org.jetbrains.annotations.Nullable()
    public final com.dorflix.app.domain.model.WatchProgress component4() {
        return null;
    }
    
    @org.jetbrains.annotations.NotNull()
    public final com.dorflix.app.presentation.viewmodel.VideoUiState copy(boolean isLoading, @org.jetbrains.annotations.Nullable()
    java.lang.String error, @org.jetbrains.annotations.Nullable()
    com.dorflix.app.domain.model.Video currentVideo, @org.jetbrains.annotations.Nullable()
    com.dorflix.app.domain.model.WatchProgress watchProgress) {
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