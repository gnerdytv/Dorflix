package com.dorflix.app.domain.model;

/**
 * Video data model representing a video from the backend API
 * Field names match backend snake_case JSON response
 */
@kotlin.Metadata(mv = {2, 3, 0}, k = 1, xi = 48, d1 = {"\u0000(\n\u0002\u0018\u0002\n\u0002\u0010\u0000\n\u0000\n\u0002\u0010\u000e\n\u0002\b\u0005\n\u0002\u0010\b\n\u0002\b\u0007\n\u0002\u0010\u000b\n\u0000\n\u0002\u0018\u0002\n\u0002\b4\b\u0086\b\u0018\u00002\u00020\u0001B\u00a5\u0001\u0012\u0006\u0010\u0002\u001a\u00020\u0003\u0012\u0006\u0010\u0004\u001a\u00020\u0003\u0012\b\u0010\u0005\u001a\u0004\u0018\u00010\u0003\u0012\u0006\u0010\u0006\u001a\u00020\u0003\u0012\b\u0010\u0007\u001a\u0004\u0018\u00010\u0003\u0012\u0006\u0010\b\u001a\u00020\t\u0012\u0006\u0010\n\u001a\u00020\u0003\u0012\b\u0010\u000b\u001a\u0004\u0018\u00010\u0003\u0012\u0006\u0010\f\u001a\u00020\t\u0012\u0006\u0010\r\u001a\u00020\t\u0012\u0006\u0010\u000e\u001a\u00020\t\u0012\u0006\u0010\u000f\u001a\u00020\t\u0012\u0006\u0010\u0010\u001a\u00020\u0011\u0012\u0006\u0010\u0012\u001a\u00020\u0013\u0012\u0006\u0010\u0014\u001a\u00020\u0013\u0012\b\b\u0002\u0010\u0015\u001a\u00020\u0011\u0012\b\b\u0002\u0010\u0016\u001a\u00020\u0011\u0012\n\b\u0002\u0010\u0017\u001a\u0004\u0018\u00010\t\u00a2\u0006\u0004\b\u0018\u0010\u0019J\t\u0010/\u001a\u00020\u0003H\u00c6\u0003J\t\u00100\u001a\u00020\u0003H\u00c6\u0003J\u000b\u00101\u001a\u0004\u0018\u00010\u0003H\u00c6\u0003J\t\u00102\u001a\u00020\u0003H\u00c6\u0003J\u000b\u00103\u001a\u0004\u0018\u00010\u0003H\u00c6\u0003J\t\u00104\u001a\u00020\tH\u00c6\u0003J\t\u00105\u001a\u00020\u0003H\u00c6\u0003J\u000b\u00106\u001a\u0004\u0018\u00010\u0003H\u00c6\u0003J\t\u00107\u001a\u00020\tH\u00c6\u0003J\t\u00108\u001a\u00020\tH\u00c6\u0003J\t\u00109\u001a\u00020\tH\u00c6\u0003J\t\u0010:\u001a\u00020\tH\u00c6\u0003J\t\u0010;\u001a\u00020\u0011H\u00c6\u0003J\t\u0010<\u001a\u00020\u0013H\u00c6\u0003J\t\u0010=\u001a\u00020\u0013H\u00c6\u0003J\t\u0010>\u001a\u00020\u0011H\u00c6\u0003J\t\u0010?\u001a\u00020\u0011H\u00c6\u0003J\u0010\u0010@\u001a\u0004\u0018\u00010\tH\u00c6\u0003\u00a2\u0006\u0002\u0010-J\u00ca\u0001\u0010A\u001a\u00020\u00002\b\b\u0002\u0010\u0002\u001a\u00020\u00032\b\b\u0002\u0010\u0004\u001a\u00020\u00032\n\b\u0002\u0010\u0005\u001a\u0004\u0018\u00010\u00032\b\b\u0002\u0010\u0006\u001a\u00020\u00032\n\b\u0002\u0010\u0007\u001a\u0004\u0018\u00010\u00032\b\b\u0002\u0010\b\u001a\u00020\t2\b\b\u0002\u0010\n\u001a\u00020\u00032\n\b\u0002\u0010\u000b\u001a\u0004\u0018\u00010\u00032\b\b\u0002\u0010\f\u001a\u00020\t2\b\b\u0002\u0010\r\u001a\u00020\t2\b\b\u0002\u0010\u000e\u001a\u00020\t2\b\b\u0002\u0010\u000f\u001a\u00020\t2\b\b\u0002\u0010\u0010\u001a\u00020\u00112\b\b\u0002\u0010\u0012\u001a\u00020\u00132\b\b\u0002\u0010\u0014\u001a\u00020\u00132\b\b\u0002\u0010\u0015\u001a\u00020\u00112\b\b\u0002\u0010\u0016\u001a\u00020\u00112\n\b\u0002\u0010\u0017\u001a\u0004\u0018\u00010\tH\u00c6\u0001\u00a2\u0006\u0002\u0010BJ\u0014\u0010C\u001a\u00020\u00112\b\u0010D\u001a\u0004\u0018\u00010\u0001H\u00d6\u0083\u0004J\n\u0010E\u001a\u00020\tH\u00d6\u0081\u0004J\n\u0010F\u001a\u00020\u0003H\u00d6\u0081\u0004R\u0011\u0010\u0002\u001a\u00020\u0003\u00a2\u0006\b\n\u0000\u001a\u0004\b\u001a\u0010\u001bR\u0011\u0010\u0004\u001a\u00020\u0003\u00a2\u0006\b\n\u0000\u001a\u0004\b\u001c\u0010\u001bR\u0013\u0010\u0005\u001a\u0004\u0018\u00010\u0003\u00a2\u0006\b\n\u0000\u001a\u0004\b\u001d\u0010\u001bR\u0011\u0010\u0006\u001a\u00020\u0003\u00a2\u0006\b\n\u0000\u001a\u0004\b\u001e\u0010\u001bR\u0013\u0010\u0007\u001a\u0004\u0018\u00010\u0003\u00a2\u0006\b\n\u0000\u001a\u0004\b\u001f\u0010\u001bR\u0011\u0010\b\u001a\u00020\t\u00a2\u0006\b\n\u0000\u001a\u0004\b \u0010!R\u0011\u0010\n\u001a\u00020\u0003\u00a2\u0006\b\n\u0000\u001a\u0004\b\"\u0010\u001bR\u0013\u0010\u000b\u001a\u0004\u0018\u00010\u0003\u00a2\u0006\b\n\u0000\u001a\u0004\b#\u0010\u001bR\u0011\u0010\f\u001a\u00020\t\u00a2\u0006\b\n\u0000\u001a\u0004\b$\u0010!R\u0011\u0010\r\u001a\u00020\t\u00a2\u0006\b\n\u0000\u001a\u0004\b%\u0010!R\u0011\u0010\u000e\u001a\u00020\t\u00a2\u0006\b\n\u0000\u001a\u0004\b&\u0010!R\u0011\u0010\u000f\u001a\u00020\t\u00a2\u0006\b\n\u0000\u001a\u0004\b\'\u0010!R\u0011\u0010\u0010\u001a\u00020\u0011\u00a2\u0006\b\n\u0000\u001a\u0004\b\u0010\u0010(R\u0011\u0010\u0012\u001a\u00020\u0013\u00a2\u0006\b\n\u0000\u001a\u0004\b)\u0010*R\u0011\u0010\u0014\u001a\u00020\u0013\u00a2\u0006\b\n\u0000\u001a\u0004\b+\u0010*R\u0011\u0010\u0015\u001a\u00020\u0011\u00a2\u0006\b\n\u0000\u001a\u0004\b\u0015\u0010(R\u0011\u0010\u0016\u001a\u00020\u0011\u00a2\u0006\b\n\u0000\u001a\u0004\b\u0016\u0010(R\u0015\u0010\u0017\u001a\u0004\u0018\u00010\t\u00a2\u0006\n\n\u0002\u0010.\u001a\u0004\b,\u0010-\u00a8\u0006G"}, d2 = {"Lcom/dorflix/app/domain/model/Video;", "", "id", "", "title", "description", "video_url", "thumbnail_url", "duration", "", "uploader_name", "uploader_avatar_url", "likes_count", "comments_count", "shares_count", "views_count", "is_active", "", "created_at", "Ljava/util/Date;", "updated_at", "is_liked", "is_completed", "current_progress", "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;ILjava/lang/String;Ljava/lang/String;IIIIZLjava/util/Date;Ljava/util/Date;ZZLjava/lang/Integer;)V", "getId", "()Ljava/lang/String;", "getTitle", "getDescription", "getVideo_url", "getThumbnail_url", "getDuration", "()I", "getUploader_name", "getUploader_avatar_url", "getLikes_count", "getComments_count", "getShares_count", "getViews_count", "()Z", "getCreated_at", "()Ljava/util/Date;", "getUpdated_at", "getCurrent_progress", "()Ljava/lang/Integer;", "Ljava/lang/Integer;", "component1", "component2", "component3", "component4", "component5", "component6", "component7", "component8", "component9", "component10", "component11", "component12", "component13", "component14", "component15", "component16", "component17", "component18", "copy", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;ILjava/lang/String;Ljava/lang/String;IIIIZLjava/util/Date;Ljava/util/Date;ZZLjava/lang/Integer;)Lcom/dorflix/app/domain/model/Video;", "equals", "other", "hashCode", "toString", "DorflixNative_debug"})
public final class Video {
    @org.jetbrains.annotations.NotNull()
    private final java.lang.String id = null;
    @org.jetbrains.annotations.NotNull()
    private final java.lang.String title = null;
    @org.jetbrains.annotations.Nullable()
    private final java.lang.String description = null;
    @org.jetbrains.annotations.NotNull()
    private final java.lang.String video_url = null;
    @org.jetbrains.annotations.Nullable()
    private final java.lang.String thumbnail_url = null;
    private final int duration = 0;
    @org.jetbrains.annotations.NotNull()
    private final java.lang.String uploader_name = null;
    @org.jetbrains.annotations.Nullable()
    private final java.lang.String uploader_avatar_url = null;
    private final int likes_count = 0;
    private final int comments_count = 0;
    private final int shares_count = 0;
    private final int views_count = 0;
    private final boolean is_active = false;
    @org.jetbrains.annotations.NotNull()
    private final java.util.Date created_at = null;
    @org.jetbrains.annotations.NotNull()
    private final java.util.Date updated_at = null;
    private final boolean is_liked = false;
    private final boolean is_completed = false;
    @org.jetbrains.annotations.Nullable()
    private final java.lang.Integer current_progress = null;
    
    public Video(@org.jetbrains.annotations.NotNull()
    java.lang.String id, @org.jetbrains.annotations.NotNull()
    java.lang.String title, @org.jetbrains.annotations.Nullable()
    java.lang.String description, @org.jetbrains.annotations.NotNull()
    java.lang.String video_url, @org.jetbrains.annotations.Nullable()
    java.lang.String thumbnail_url, int duration, @org.jetbrains.annotations.NotNull()
    java.lang.String uploader_name, @org.jetbrains.annotations.Nullable()
    java.lang.String uploader_avatar_url, int likes_count, int comments_count, int shares_count, int views_count, boolean is_active, @org.jetbrains.annotations.NotNull()
    java.util.Date created_at, @org.jetbrains.annotations.NotNull()
    java.util.Date updated_at, boolean is_liked, boolean is_completed, @org.jetbrains.annotations.Nullable()
    java.lang.Integer current_progress) {
        super();
    }
    
    @org.jetbrains.annotations.NotNull()
    public final java.lang.String getId() {
        return null;
    }
    
    @org.jetbrains.annotations.NotNull()
    public final java.lang.String getTitle() {
        return null;
    }
    
    @org.jetbrains.annotations.Nullable()
    public final java.lang.String getDescription() {
        return null;
    }
    
    @org.jetbrains.annotations.NotNull()
    public final java.lang.String getVideo_url() {
        return null;
    }
    
    @org.jetbrains.annotations.Nullable()
    public final java.lang.String getThumbnail_url() {
        return null;
    }
    
    public final int getDuration() {
        return 0;
    }
    
    @org.jetbrains.annotations.NotNull()
    public final java.lang.String getUploader_name() {
        return null;
    }
    
    @org.jetbrains.annotations.Nullable()
    public final java.lang.String getUploader_avatar_url() {
        return null;
    }
    
    public final int getLikes_count() {
        return 0;
    }
    
    public final int getComments_count() {
        return 0;
    }
    
    public final int getShares_count() {
        return 0;
    }
    
    public final int getViews_count() {
        return 0;
    }
    
    public final boolean is_active() {
        return false;
    }
    
    @org.jetbrains.annotations.NotNull()
    public final java.util.Date getCreated_at() {
        return null;
    }
    
    @org.jetbrains.annotations.NotNull()
    public final java.util.Date getUpdated_at() {
        return null;
    }
    
    public final boolean is_liked() {
        return false;
    }
    
    public final boolean is_completed() {
        return false;
    }
    
    @org.jetbrains.annotations.Nullable()
    public final java.lang.Integer getCurrent_progress() {
        return null;
    }
    
    @org.jetbrains.annotations.NotNull()
    public final java.lang.String component1() {
        return null;
    }
    
    public final int component10() {
        return 0;
    }
    
    public final int component11() {
        return 0;
    }
    
    public final int component12() {
        return 0;
    }
    
    public final boolean component13() {
        return false;
    }
    
    @org.jetbrains.annotations.NotNull()
    public final java.util.Date component14() {
        return null;
    }
    
    @org.jetbrains.annotations.NotNull()
    public final java.util.Date component15() {
        return null;
    }
    
    public final boolean component16() {
        return false;
    }
    
    public final boolean component17() {
        return false;
    }
    
    @org.jetbrains.annotations.Nullable()
    public final java.lang.Integer component18() {
        return null;
    }
    
    @org.jetbrains.annotations.NotNull()
    public final java.lang.String component2() {
        return null;
    }
    
    @org.jetbrains.annotations.Nullable()
    public final java.lang.String component3() {
        return null;
    }
    
    @org.jetbrains.annotations.NotNull()
    public final java.lang.String component4() {
        return null;
    }
    
    @org.jetbrains.annotations.Nullable()
    public final java.lang.String component5() {
        return null;
    }
    
    public final int component6() {
        return 0;
    }
    
    @org.jetbrains.annotations.NotNull()
    public final java.lang.String component7() {
        return null;
    }
    
    @org.jetbrains.annotations.Nullable()
    public final java.lang.String component8() {
        return null;
    }
    
    public final int component9() {
        return 0;
    }
    
    @org.jetbrains.annotations.NotNull()
    public final com.dorflix.app.domain.model.Video copy(@org.jetbrains.annotations.NotNull()
    java.lang.String id, @org.jetbrains.annotations.NotNull()
    java.lang.String title, @org.jetbrains.annotations.Nullable()
    java.lang.String description, @org.jetbrains.annotations.NotNull()
    java.lang.String video_url, @org.jetbrains.annotations.Nullable()
    java.lang.String thumbnail_url, int duration, @org.jetbrains.annotations.NotNull()
    java.lang.String uploader_name, @org.jetbrains.annotations.Nullable()
    java.lang.String uploader_avatar_url, int likes_count, int comments_count, int shares_count, int views_count, boolean is_active, @org.jetbrains.annotations.NotNull()
    java.util.Date created_at, @org.jetbrains.annotations.NotNull()
    java.util.Date updated_at, boolean is_liked, boolean is_completed, @org.jetbrains.annotations.Nullable()
    java.lang.Integer current_progress) {
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