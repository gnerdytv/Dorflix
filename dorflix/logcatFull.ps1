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
        $output = "$([char]27)[32m[APP START] $line$([char]27)[0m"
        Write-Host $output
    }
    elseif ($line -match "JNI_OnLoad|System.loadLibrary|UnsatisfiedLinkError") {
        $output = "$([char]27)[36m[JNI LOAD] $line$([char]27)[0m"
        Write-Host $output
    }
    elseif ($line -match "Starting detailed FFmpeg function verification|Testing avcodec_|Testing sws_|FFmpeg function verification passed|Exception in avcodec_|Exception in sws_|FFmpeg codec functions not available|FFmpeg scaling functions not available") {
        $output = "$([char]27)[34m[FFMPEG TEST] $line$([char]27)[0m"
        Write-Host $output
    }
    elseif ($line -match "avcodec_find_decoder returned|avcodec_alloc_context3 returned|sws_getContext returned|av_frame_alloc returned|av_frame_free completed") {
        $output = "$([char]27)[94m[FFMPEG RESULT] $line$([char]27)[0m"
        Write-Host $output
    }
    elseif ($line -match "FFmpeg library verification|FFmpeg functions are available|FFmpeg format context operations work") {
        $output = "$([char]27)[96m[FFMPEG VERIFY] $line$([char]27)[0m"
        Write-Host $output
    }
    elseif ($line -match "CodecEnumerator|MediaCodec") {
        $output = "$([char]27)[35m[CODEC ENUM] $line$([char]27)[0m"
        Write-Host $output
    }
    elseif ($line -match "VIDEO LOADING|VideoDecoder|nativeLoadVideo|VideoRepository|FeedFragment") {
        $output = "$([char]27)[33m[VIDEO LOAD] $line$([char]27)[0m"
        Write-Host $output
    }
    elseif ($line -match "JNI library ready|isJNILibraryReady") {
        $output = "$([char]27)[32m[JNI READY] $line$([char]27)[0m"
        Write-Host $output
    }
    elseif ($line -match "FATAL|CRASH|SIGSEGV|SIGABRT|Exception|UnsatisfiedLinkError") {
        $output = "$([char]27)[31m[FATAL] $line$([char]27)[0m"
        Write-Host $output
    }
    elseif ($line -match "WARNING|WARN|ï¿½") {
        $output = "$([char]27)[33m[WARNING] $line$([char]27)[0m"
        Write-Host $output
    }
    elseif ($line -match "ERROR|Error") {
        $output = "$([char]27)[31m[ERROR] $line$([char]27)[0m"
        Write-Host $output
    }
    else {
        $output = "$([char]27)[37m[LOG] $line$([char]27)[0m"
        Write-Host $output
    }
    
    # Save to logger file
    if ($output) {
        Add-Content "logger" $output
    }
}
