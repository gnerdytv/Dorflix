package com.dorflix.app.data.api;

/**
 * Retrofit API service interface for Dorflix backend
 */
@kotlin.Metadata(mv = {2, 3, 0}, k = 1, xi = 48, d1 = {"\u0000\u0090\u0001\n\u0002\u0018\u0002\n\u0002\u0010\u0000\n\u0000\n\u0002\u0018\u0002\n\u0002\u0010$\n\u0002\u0010\u000e\n\u0002\b\u0002\n\u0002\u0018\u0002\n\u0000\n\u0002\u0018\u0002\n\u0002\b\u0005\n\u0002\u0018\u0002\n\u0000\n\u0002\u0010\b\n\u0002\b\u0005\n\u0002\u0018\u0002\n\u0002\b\u0003\n\u0002\u0018\u0002\n\u0002\b\u0003\n\u0002\u0018\u0002\n\u0000\n\u0002\u0018\u0002\n\u0002\b\u0003\n\u0002\u0010 \n\u0002\u0018\u0002\n\u0002\b\u0004\n\u0002\u0010\u000b\n\u0002\b\u0002\n\u0002\u0018\u0002\n\u0002\b\u0002\n\u0002\u0018\u0002\n\u0000\n\u0002\u0018\u0002\n\u0000\n\u0002\u0018\u0002\n\u0002\b\u0005\n\u0002\u0018\u0002\n\u0000\n\u0002\u0018\u0002\n\u0000\bf\u0018\u00002\u00020\u0001J \u0010\u0002\u001a\u0014\u0012\u0010\u0012\u000e\u0012\u0004\u0012\u00020\u0005\u0012\u0004\u0012\u00020\u00010\u00040\u0003H\u00a7@\u00a2\u0006\u0002\u0010\u0006J\u001e\u0010\u0007\u001a\b\u0012\u0004\u0012\u00020\b0\u00032\b\b\u0001\u0010\t\u001a\u00020\nH\u00a7@\u00a2\u0006\u0002\u0010\u000bJ*\u0010\f\u001a\u0014\u0012\u0010\u0012\u000e\u0012\u0004\u0012\u00020\u0005\u0012\u0004\u0012\u00020\u00010\u00040\u00032\b\b\u0001\u0010\r\u001a\u00020\u0005H\u00a7@\u00a2\u0006\u0002\u0010\u000eJ<\u0010\u000f\u001a\b\u0012\u0004\u0012\u00020\u00100\u00032\b\b\u0003\u0010\u0011\u001a\u00020\u00122\b\b\u0003\u0010\u0013\u001a\u00020\u00122\b\b\u0003\u0010\u0014\u001a\u00020\u00052\b\b\u0003\u0010\u0015\u001a\u00020\u0005H\u00a7@\u00a2\u0006\u0002\u0010\u0016J2\u0010\u0017\u001a\b\u0012\u0004\u0012\u00020\u00180\u00032\b\b\u0001\u0010\u0019\u001a\u00020\u00052\b\b\u0003\u0010\u0011\u001a\u00020\u00122\b\b\u0003\u0010\u0013\u001a\u00020\u0012H\u00a7@\u00a2\u0006\u0002\u0010\u001aJ\u001e\u0010\u001b\u001a\b\u0012\u0004\u0012\u00020\u001c0\u00032\b\b\u0001\u0010\u001d\u001a\u00020\u0005H\u00a7@\u00a2\u0006\u0002\u0010\u000eJ*\u0010\u001e\u001a\u0014\u0012\u0010\u0012\u000e\u0012\u0004\u0012\u00020\u0005\u0012\u0004\u0012\u00020\u00010\u00040\u00032\b\b\u0001\u0010\u001d\u001a\u00020\u0005H\u00a7@\u00a2\u0006\u0002\u0010\u000eJ\u001e\u0010\u001f\u001a\b\u0012\u0004\u0012\u00020 0\u00032\b\b\u0001\u0010!\u001a\u00020\"H\u00a7@\u00a2\u0006\u0002\u0010#J \u0010$\u001a\n\u0012\u0006\u0012\u0004\u0018\u00010 0\u00032\b\b\u0001\u0010\u001d\u001a\u00020\u0005H\u00a7@\u00a2\u0006\u0002\u0010\u000eJ$\u0010%\u001a\u000e\u0012\n\u0012\b\u0012\u0004\u0012\u00020\'0&0\u00032\b\b\u0003\u0010\u0011\u001a\u00020\u0012H\u00a7@\u00a2\u0006\u0002\u0010(J*\u0010)\u001a\u0014\u0012\u0010\u0012\u000e\u0012\u0004\u0012\u00020\u0005\u0012\u0004\u0012\u00020\u00010\u00040\u00032\b\b\u0001\u0010\u001d\u001a\u00020\u0005H\u00a7@\u00a2\u0006\u0002\u0010\u000eJ*\u0010*\u001a\u0014\u0012\u0010\u0012\u000e\u0012\u0004\u0012\u00020\u0005\u0012\u0004\u0012\u00020\u00010\u00040\u00032\b\b\u0001\u0010\u001d\u001a\u00020\u0005H\u00a7@\u00a2\u0006\u0002\u0010\u000eJ*\u0010+\u001a\u0014\u0012\u0010\u0012\u000e\u0012\u0004\u0012\u00020\u0005\u0012\u0004\u0012\u00020,0\u00040\u00032\b\b\u0001\u0010\u001d\u001a\u00020\u0005H\u00a7@\u00a2\u0006\u0002\u0010\u000eJ4\u0010-\u001a\u0014\u0012\u0010\u0012\u000e\u0012\u0004\u0012\u00020\u0005\u0012\u0004\u0012\u00020\u00010\u00040\u00032\b\b\u0001\u0010\u001d\u001a\u00020\u00052\b\b\u0001\u0010.\u001a\u00020/H\u00a7@\u00a2\u0006\u0002\u00100J2\u00101\u001a\b\u0012\u0004\u0012\u0002020\u00032\b\b\u0001\u0010\u001d\u001a\u00020\u00052\b\b\u0003\u0010\u0011\u001a\u00020\u00122\b\b\u0003\u0010\u0013\u001a\u00020\u0012H\u00a7@\u00a2\u0006\u0002\u0010\u001aJ(\u00103\u001a\b\u0012\u0004\u0012\u0002040\u00032\b\b\u0001\u0010\u001d\u001a\u00020\u00052\b\b\u0001\u00105\u001a\u000206H\u00a7@\u00a2\u0006\u0002\u00107J*\u00108\u001a\u0014\u0012\u0010\u0012\u000e\u0012\u0004\u0012\u00020\u0005\u0012\u0004\u0012\u00020\u00010\u00040\u00032\b\b\u0001\u00109\u001a\u00020\u0005H\u00a7@\u00a2\u0006\u0002\u0010\u000eJ$\u0010:\u001a\u000e\u0012\n\u0012\b\u0012\u0004\u0012\u00020\'0&0\u00032\b\b\u0003\u0010\u0011\u001a\u00020\u0012H\u00a7@\u00a2\u0006\u0002\u0010(J\u0014\u0010;\u001a\b\u0012\u0004\u0012\u00020<0\u0003H\u00a7@\u00a2\u0006\u0002\u0010\u0006J\u0014\u0010=\u001a\b\u0012\u0004\u0012\u00020>0\u0003H\u00a7@\u00a2\u0006\u0002\u0010\u0006\u00a8\u0006?\u00c0\u0006\u0003"}, d2 = {"Lcom/dorflix/app/data/api/ApiService;", "", "healthCheck", "Lretrofit2/Response;", "", "", "(Lkotlin/coroutines/Continuation;)Ljava/lang/Object;", "registerDevice", "Lcom/dorflix/app/domain/model/DeviceRegistrationResponse;", "deviceData", "Lcom/dorflix/app/data/api/DeviceRegistrationRequest;", "(Lcom/dorflix/app/data/api/DeviceRegistrationRequest;Lkotlin/coroutines/Continuation;)Ljava/lang/Object;", "validateSession", "token", "(Ljava/lang/String;Lkotlin/coroutines/Continuation;)Ljava/lang/Object;", "getVideoCatalog", "Lcom/dorflix/app/domain/model/VideoCatalogResponse;", "limit", "", "offset", "sortBy", "sortOrder", "(IILjava/lang/String;Ljava/lang/String;Lkotlin/coroutines/Continuation;)Ljava/lang/Object;", "searchVideos", "Lcom/dorflix/app/domain/model/VideoSearchResponse;", "searchTerm", "(Ljava/lang/String;IILkotlin/coroutines/Continuation;)Ljava/lang/Object;", "getVideoById", "Lcom/dorflix/app/domain/model/VideoDetailsResponse;", "videoId", "getVideoStream", "saveWatchProgress", "Lcom/dorflix/app/domain/model/WatchProgress;", "progressData", "Lcom/dorflix/app/domain/model/WatchProgressRequest;", "(Lcom/dorflix/app/domain/model/WatchProgressRequest;Lkotlin/coroutines/Continuation;)Ljava/lang/Object;", "getWatchProgress", "getContinueWatching", "", "Lcom/dorflix/app/domain/model/Video;", "(ILkotlin/coroutines/Continuation;)Ljava/lang/Object;", "likeVideo", "unlikeVideo", "isVideoLiked", "", "shareVideo", "shareData", "Lcom/dorflix/app/domain/model/ShareRequest;", "(Ljava/lang/String;Lcom/dorflix/app/domain/model/ShareRequest;Lkotlin/coroutines/Continuation;)Ljava/lang/Object;", "getComments", "Lcom/dorflix/app/domain/model/CommentsResponse;", "addComment", "Lcom/dorflix/app/domain/model/CommentResponse;", "commentData", "Lcom/dorflix/app/domain/model/CommentRequest;", "(Ljava/lang/String;Lcom/dorflix/app/domain/model/CommentRequest;Lkotlin/coroutines/Continuation;)Ljava/lang/Object;", "deleteComment", "commentId", "getRecommendations", "getWatchStatistics", "Lcom/dorflix/app/domain/model/VideoStatistics;", "getSocialActivity", "Lcom/dorflix/app/domain/model/SocialActivityResponse;", "DorflixNative_debug"})
public abstract interface ApiService {
    
    @retrofit2.http.GET(value = "/health")
    @org.jetbrains.annotations.Nullable()
    public abstract java.lang.Object healthCheck(@org.jetbrains.annotations.NotNull()
    kotlin.coroutines.Continuation<? super retrofit2.Response<java.util.Map<java.lang.String, java.lang.Object>>> $completion);
    
    @retrofit2.http.POST(value = "/api/devices/register")
    @org.jetbrains.annotations.Nullable()
    public abstract java.lang.Object registerDevice(@retrofit2.http.Body()
    @org.jetbrains.annotations.NotNull()
    com.dorflix.app.data.api.DeviceRegistrationRequest deviceData, @org.jetbrains.annotations.NotNull()
    kotlin.coroutines.Continuation<? super retrofit2.Response<com.dorflix.app.domain.model.DeviceRegistrationResponse>> $completion);
    
    @retrofit2.http.POST(value = "/api/devices/validate-session")
    @org.jetbrains.annotations.Nullable()
    public abstract java.lang.Object validateSession(@retrofit2.http.Header(value = "Authorization")
    @org.jetbrains.annotations.NotNull()
    java.lang.String token, @org.jetbrains.annotations.NotNull()
    kotlin.coroutines.Continuation<? super retrofit2.Response<java.util.Map<java.lang.String, java.lang.Object>>> $completion);
    
    @retrofit2.http.GET(value = "/api/videos")
    @org.jetbrains.annotations.Nullable()
    public abstract java.lang.Object getVideoCatalog(@retrofit2.http.Query(value = "limit")
    int limit, @retrofit2.http.Query(value = "offset")
    int offset, @retrofit2.http.Query(value = "sortBy")
    @org.jetbrains.annotations.NotNull()
    java.lang.String sortBy, @retrofit2.http.Query(value = "sortOrder")
    @org.jetbrains.annotations.NotNull()
    java.lang.String sortOrder, @org.jetbrains.annotations.NotNull()
    kotlin.coroutines.Continuation<? super retrofit2.Response<com.dorflix.app.domain.model.VideoCatalogResponse>> $completion);
    
    @retrofit2.http.GET(value = "/api/videos/search")
    @org.jetbrains.annotations.Nullable()
    public abstract java.lang.Object searchVideos(@retrofit2.http.Query(value = "q")
    @org.jetbrains.annotations.NotNull()
    java.lang.String searchTerm, @retrofit2.http.Query(value = "limit")
    int limit, @retrofit2.http.Query(value = "offset")
    int offset, @org.jetbrains.annotations.NotNull()
    kotlin.coroutines.Continuation<? super retrofit2.Response<com.dorflix.app.domain.model.VideoSearchResponse>> $completion);
    
    @retrofit2.http.GET(value = "/api/videos/{id}")
    @org.jetbrains.annotations.Nullable()
    public abstract java.lang.Object getVideoById(@retrofit2.http.Path(value = "id")
    @org.jetbrains.annotations.NotNull()
    java.lang.String videoId, @org.jetbrains.annotations.NotNull()
    kotlin.coroutines.Continuation<? super retrofit2.Response<com.dorflix.app.domain.model.VideoDetailsResponse>> $completion);
    
    @retrofit2.http.GET(value = "/api/videos/{id}/stream")
    @org.jetbrains.annotations.Nullable()
    public abstract java.lang.Object getVideoStream(@retrofit2.http.Path(value = "id")
    @org.jetbrains.annotations.NotNull()
    java.lang.String videoId, @org.jetbrains.annotations.NotNull()
    kotlin.coroutines.Continuation<? super retrofit2.Response<java.util.Map<java.lang.String, java.lang.Object>>> $completion);
    
    @retrofit2.http.POST(value = "/api/watch-progress")
    @org.jetbrains.annotations.Nullable()
    public abstract java.lang.Object saveWatchProgress(@retrofit2.http.Body()
    @org.jetbrains.annotations.NotNull()
    com.dorflix.app.domain.model.WatchProgressRequest progressData, @org.jetbrains.annotations.NotNull()
    kotlin.coroutines.Continuation<? super retrofit2.Response<com.dorflix.app.domain.model.WatchProgress>> $completion);
    
    @retrofit2.http.GET(value = "/api/watch-progress/{videoId}")
    @org.jetbrains.annotations.Nullable()
    public abstract java.lang.Object getWatchProgress(@retrofit2.http.Path(value = "videoId")
    @org.jetbrains.annotations.NotNull()
    java.lang.String videoId, @org.jetbrains.annotations.NotNull()
    kotlin.coroutines.Continuation<? super retrofit2.Response<com.dorflix.app.domain.model.WatchProgress>> $completion);
    
    @retrofit2.http.GET(value = "/api/continue-watching")
    @org.jetbrains.annotations.Nullable()
    public abstract java.lang.Object getContinueWatching(@retrofit2.http.Query(value = "limit")
    int limit, @org.jetbrains.annotations.NotNull()
    kotlin.coroutines.Continuation<? super retrofit2.Response<java.util.List<com.dorflix.app.domain.model.Video>>> $completion);
    
    @retrofit2.http.POST(value = "/api/videos/{id}/like")
    @org.jetbrains.annotations.Nullable()
    public abstract java.lang.Object likeVideo(@retrofit2.http.Path(value = "id")
    @org.jetbrains.annotations.NotNull()
    java.lang.String videoId, @org.jetbrains.annotations.NotNull()
    kotlin.coroutines.Continuation<? super retrofit2.Response<java.util.Map<java.lang.String, java.lang.Object>>> $completion);
    
    @retrofit2.http.DELETE(value = "/api/videos/{id}/like")
    @org.jetbrains.annotations.Nullable()
    public abstract java.lang.Object unlikeVideo(@retrofit2.http.Path(value = "id")
    @org.jetbrains.annotations.NotNull()
    java.lang.String videoId, @org.jetbrains.annotations.NotNull()
    kotlin.coroutines.Continuation<? super retrofit2.Response<java.util.Map<java.lang.String, java.lang.Object>>> $completion);
    
    @retrofit2.http.GET(value = "/api/videos/{id}/is-liked")
    @org.jetbrains.annotations.Nullable()
    public abstract java.lang.Object isVideoLiked(@retrofit2.http.Path(value = "id")
    @org.jetbrains.annotations.NotNull()
    java.lang.String videoId, @org.jetbrains.annotations.NotNull()
    kotlin.coroutines.Continuation<? super retrofit2.Response<java.util.Map<java.lang.String, java.lang.Boolean>>> $completion);
    
    @retrofit2.http.POST(value = "/api/videos/{id}/share")
    @org.jetbrains.annotations.Nullable()
    public abstract java.lang.Object shareVideo(@retrofit2.http.Path(value = "id")
    @org.jetbrains.annotations.NotNull()
    java.lang.String videoId, @retrofit2.http.Body()
    @org.jetbrains.annotations.NotNull()
    com.dorflix.app.domain.model.ShareRequest shareData, @org.jetbrains.annotations.NotNull()
    kotlin.coroutines.Continuation<? super retrofit2.Response<java.util.Map<java.lang.String, java.lang.Object>>> $completion);
    
    @retrofit2.http.GET(value = "/api/videos/{id}/comments")
    @org.jetbrains.annotations.Nullable()
    public abstract java.lang.Object getComments(@retrofit2.http.Path(value = "id")
    @org.jetbrains.annotations.NotNull()
    java.lang.String videoId, @retrofit2.http.Query(value = "limit")
    int limit, @retrofit2.http.Query(value = "offset")
    int offset, @org.jetbrains.annotations.NotNull()
    kotlin.coroutines.Continuation<? super retrofit2.Response<com.dorflix.app.domain.model.CommentsResponse>> $completion);
    
    @retrofit2.http.POST(value = "/api/videos/{id}/comments")
    @org.jetbrains.annotations.Nullable()
    public abstract java.lang.Object addComment(@retrofit2.http.Path(value = "id")
    @org.jetbrains.annotations.NotNull()
    java.lang.String videoId, @retrofit2.http.Body()
    @org.jetbrains.annotations.NotNull()
    com.dorflix.app.domain.model.CommentRequest commentData, @org.jetbrains.annotations.NotNull()
    kotlin.coroutines.Continuation<? super retrofit2.Response<com.dorflix.app.domain.model.CommentResponse>> $completion);
    
    @retrofit2.http.DELETE(value = "/api/comments/{id}")
    @org.jetbrains.annotations.Nullable()
    public abstract java.lang.Object deleteComment(@retrofit2.http.Path(value = "id")
    @org.jetbrains.annotations.NotNull()
    java.lang.String commentId, @org.jetbrains.annotations.NotNull()
    kotlin.coroutines.Continuation<? super retrofit2.Response<java.util.Map<java.lang.String, java.lang.Object>>> $completion);
    
    @retrofit2.http.GET(value = "/api/recommendations")
    @org.jetbrains.annotations.Nullable()
    public abstract java.lang.Object getRecommendations(@retrofit2.http.Query(value = "limit")
    int limit, @org.jetbrains.annotations.NotNull()
    kotlin.coroutines.Continuation<? super retrofit2.Response<java.util.List<com.dorflix.app.domain.model.Video>>> $completion);
    
    @retrofit2.http.GET(value = "/api/stats/watch")
    @org.jetbrains.annotations.Nullable()
    public abstract java.lang.Object getWatchStatistics(@org.jetbrains.annotations.NotNull()
    kotlin.coroutines.Continuation<? super retrofit2.Response<com.dorflix.app.domain.model.VideoStatistics>> $completion);
    
    @retrofit2.http.GET(value = "/api/stats/social")
    @org.jetbrains.annotations.Nullable()
    public abstract java.lang.Object getSocialActivity(@org.jetbrains.annotations.NotNull()
    kotlin.coroutines.Continuation<? super retrofit2.Response<com.dorflix.app.domain.model.SocialActivityResponse>> $completion);
    
    /**
     * Retrofit API service interface for Dorflix backend
     */
    @kotlin.Metadata(mv = {2, 3, 0}, k = 3, xi = 48)
    public static final class DefaultImpls {
    }
}