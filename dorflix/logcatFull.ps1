# Clear the logger file at start
if (Test-Path "logger") {
    Clear-Content "logger"
} else {
    New-Item "logger" -ItemType File | Out-Null
}

adb logcat -c; adb logcat | ForEach-Object {
    $line = $_
    $output = ""
    
    if ($line -match "ActivityManager.*dorflix|ActivityManager.*Dorflix") {
        $output = "[APP START] $line"
        Write-Host $output -ForegroundColor Green
    }
    elseif ($line -match "JNI_OnLoad|System.loadLibrary|UnsatisfiedLinkError") {
        $output = "[JNI LOAD] $line"
        Write-Host $output -ForegroundColor Cyan
    }
    elseif ($line -match "Starting detailed FFmpeg function verification|Testing avcodec_|Testing sws_|FFmpeg function verification passed|Exception in avcodec_|Exception in sws_|FFmpeg codec functions not available|FFmpeg scaling functions not available") {
        $output = "[FFMPEG TEST] $line"
        Write-Host $output -ForegroundColor Blue
    }
    elseif ($line -match "avcodec_find_decoder returned|avcodec_alloc_context3 returned|sws_getContext returned|av_frame_alloc returned|av_frame_free completed") {
        $output = "[FFMPEG RESULT] $line"
        Write-Host $output -ForegroundColor DarkBlue
    }
    elseif ($line -match "FFmpeg library verification|FFmpeg functions are available|FFmpeg format context operations work") {
        $output = "[FFMPEG VERIFY] $line"
        Write-Host $output -ForegroundColor DarkCyan
    }
    elseif ($line -match "CodecEnumerator|MediaCodec") {
        $output = "[CODEC ENUM] $line"
        Write-Host $output -ForegroundColor Magenta
    }
    elseif ($line -match "VIDEO LOADING|VideoDecoder|nativeLoadVideo|VideoRepository|FeedFragment") {
        $output = "[VIDEO LOAD] $line"
        Write-Host $output -ForegroundColor Yellow
    }
    elseif ($line -match "JNI library ready|isJNILibraryReady") {
        $output = "[JNI READY] $line"
        Write-Host $output -ForegroundColor Green
    }
    elseif ($line -match "FATAL|CRASH|SIGSEGV|SIGABRT|Exception|UnsatisfiedLinkError") {
        $output = "[FATAL] $line"
        Write-Host $output -ForegroundColor Red
    }
    elseif ($line -match "WARNING|WARN|ï¿½") {
        $output = "[WARNING] $line"
        Write-Host $output -ForegroundColor Yellow
    }
    elseif ($line -match "ERROR|Error") {
        $output = "[ERROR] $line"
        Write-Host $output -ForegroundColor Red
    }
    else {
        $output = "[LOG] $line"
        Write-Host $output -ForegroundColor White
    }
    
    # Save to logger file
    if ($output) {
        Add-Content "logger" $output
    }
}
