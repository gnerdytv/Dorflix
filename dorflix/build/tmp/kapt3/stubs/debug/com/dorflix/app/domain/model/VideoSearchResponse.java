package com.dorflix.app.domain.model;

/**
 * Video search response from API
 */
@kotlin.Metadata(mv = {2, 3, 0}, k = 1, xi = 48, d1 = {"\u0000(\n\u0002\u0018\u0002\n\u0002\u0010\u0000\n\u0000\n\u0002\u0010\u000e\n\u0000\n\u0002\u0010 \n\u0002\u0018\u0002\n\u0000\n\u0002\u0010\b\n\u0000\n\u0002\u0010\u000b\n\u0002\b\u0014\b\u0086\b\u0018\u00002\u00020\u0001B-\u0012\u0006\u0010\u0002\u001a\u00020\u0003\u0012\f\u0010\u0004\u001a\b\u0012\u0004\u0012\u00020\u00060\u0005\u0012\u0006\u0010\u0007\u001a\u00020\b\u0012\u0006\u0010\t\u001a\u00020\n\u00a2\u0006\u0004\b\u000b\u0010\fJ\t\u0010\u0015\u001a\u00020\u0003H\u00c6\u0003J\u000f\u0010\u0016\u001a\b\u0012\u0004\u0012\u00020\u00060\u0005H\u00c6\u0003J\t\u0010\u0017\u001a\u00020\bH\u00c6\u0003J\t\u0010\u0018\u001a\u00020\nH\u00c6\u0003J7\u0010\u0019\u001a\u00020\u00002\b\b\u0002\u0010\u0002\u001a\u00020\u00032\u000e\b\u0002\u0010\u0004\u001a\b\u0012\u0004\u0012\u00020\u00060\u00052\b\b\u0002\u0010\u0007\u001a\u00020\b2\b\b\u0002\u0010\t\u001a\u00020\nH\u00c6\u0001J\u0014\u0010\u001a\u001a\u00020\n2\b\u0010\u001b\u001a\u0004\u0018\u00010\u0001H\u00d6\u0083\u0004J\n\u0010\u001c\u001a\u00020\bH\u00d6\u0081\u0004J\n\u0010\u001d\u001a\u00020\u0003H\u00d6\u0081\u0004R\u0011\u0010\u0002\u001a\u00020\u0003\u00a2\u0006\b\n\u0000\u001a\u0004\b\r\u0010\u000eR\u0017\u0010\u0004\u001a\b\u0012\u0004\u0012\u00020\u00060\u0005\u00a2\u0006\b\n\u0000\u001a\u0004\b\u000f\u0010\u0010R\u0011\u0010\u0007\u001a\u00020\b\u00a2\u0006\b\n\u0000\u001a\u0004\b\u0011\u0010\u0012R\u0011\u0010\t\u001a\u00020\n\u00a2\u0006\b\n\u0000\u001a\u0004\b\u0013\u0010\u0014\u00a8\u0006\u001e"}, d2 = {"Lcom/dorflix/app/domain/model/VideoSearchResponse;", "", "searchTerm", "", "videos", "", "Lcom/dorflix/app/domain/model/Video;", "total", "", "hasMore", "", "<init>", "(Ljava/lang/String;Ljava/util/List;IZ)V", "getSearchTerm", "()Ljava/lang/String;", "getVideos", "()Ljava/util/List;", "getTotal", "()I", "getHasMore", "()Z", "component1", "component2", "component3", "component4", "copy", "equals", "other", "hashCode", "toString", "DorflixNative_debug"})
public final class VideoSearchResponse {
    @org.jetbrains.annotations.NotNull()
    private final java.lang.String searchTerm = null;
    @org.jetbrains.annotations.NotNull()
    private final java.util.List<com.dorflix.app.domain.model.Video> videos = null;
    private final int total = 0;
    private final boolean hasMore = false;
    
    public VideoSearchResponse(@org.jetbrains.annotations.NotNull()
    java.lang.String searchTerm, @org.jetbrains.annotations.NotNull()
    java.util.List<com.dorflix.app.domain.model.Video> videos, int total, boolean hasMore) {
        super();
    }
    
    @org.jetbrains.annotations.NotNull()
    public final java.lang.String getSearchTerm() {
        return null;
    }
    
    @org.jetbrains.annotations.NotNull()
    public final java.util.List<com.dorflix.app.domain.model.Video> getVideos() {
        return null;
    }
    
    public final int getTotal() {
        return 0;
    }
    
    public final boolean getHasMore() {
        return false;
    }
    
    @org.jetbrains.annotations.NotNull()
    public final java.lang.String component1() {
        return null;
    }
    
    @org.jetbrains.annotations.NotNull()
    public final java.util.List<com.dorflix.app.domain.model.Video> component2() {
        return null;
    }
    
    public final int component3() {
        return 0;
    }
    
    public final boolean component4() {
        return false;
    }
    
    @org.jetbrains.annotations.NotNull()
    public final com.dorflix.app.domain.model.VideoSearchResponse copy(@org.jetbrains.annotations.NotNull()
    java.lang.String searchTerm, @org.jetbrains.annotations.NotNull()
    java.util.List<com.dorflix.app.domain.model.Video> videos, int total, boolean hasMore) {
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