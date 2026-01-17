package com.dorflix.app.video;

@kotlin.Metadata(mv = {2, 3, 0}, k = 1, xi = 48, d1 = {"\u0000\u001e\n\u0002\u0018\u0002\n\u0002\u0010\u0000\n\u0000\n\u0002\u0010\u0002\n\u0000\n\u0002\u0010\u000e\n\u0000\n\u0002\u0010\t\n\u0002\b\u0006\bf\u0018\u00002\u00020\u0001J \u0010\u0002\u001a\u00020\u00032\u0006\u0010\u0004\u001a\u00020\u00052\u0006\u0010\u0006\u001a\u00020\u00072\u0006\u0010\b\u001a\u00020\u0007H&J\u0018\u0010\t\u001a\u00020\u00032\u0006\u0010\u0004\u001a\u00020\u00052\u0006\u0010\n\u001a\u00020\u0005H&J\u0018\u0010\u000b\u001a\u00020\u00032\u0006\u0010\u0004\u001a\u00020\u00052\u0006\u0010\f\u001a\u00020\u0005H&\u00a8\u0006\r\u00c0\u0006\u0003"}, d2 = {"Lcom/dorflix/app/video/DownloadListener;", "", "onDownloadProgress", "", "url", "", "downloaded", "", "total", "onDownloadComplete", "localPath", "onDownloadError", "error", "DorflixNative_debug"})
public abstract interface DownloadListener {
    
    public abstract void onDownloadProgress(@org.jetbrains.annotations.NotNull()
    java.lang.String url, long downloaded, long total);
    
    public abstract void onDownloadComplete(@org.jetbrains.annotations.NotNull()
    java.lang.String url, @org.jetbrains.annotations.NotNull()
    java.lang.String localPath);
    
    public abstract void onDownloadError(@org.jetbrains.annotations.NotNull()
    java.lang.String url, @org.jetbrains.annotations.NotNull()
    java.lang.String error);
}