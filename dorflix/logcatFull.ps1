adb logcat -c; adb logcat | ForEach-Object {
    $line = $_
    
    if ($line -match "ActivityManager.*dorflix|ActivityManager.*Dorflix") {
        Write-Host "[APP START] $line" -ForegroundColor Green
    }
    elseif ($line -match "JNI_OnLoad|System.loadLibrary|UnsatisfiedLinkError") {
        Write-Host "[JNI LOAD] $line" -ForegroundColor Cyan
        # Break when error/crash is detected
        # break
    }
    elseif ($line -match "CodecEnumerator|MediaCodec") {
        Write-Host "[CODEC ENUM] $line" -ForegroundColor Magenta
        # break
    }
    elseif ($line -match "VIDEO LOADING|VideoDecoder|nativeLoadVideo") {
        Write-Host "[VIDEO LOAD] $line" -ForegroundColor Yellow
        # break
    }
    elseif ($line -match "FATAL|CRASH|SIGSEGV|SIGABRT|Exception") {
        Write-Host "[FATAL] $line" -ForegroundColor Red
        # break
    }
    elseif ($line -match "WARNING|WARN") {
        Write-Host "[WARNING] $line" -ForegroundColor Yellow
        # break
    }
    else {
        Write-Host "[LOG] $line" -ForegroundColor White
    }
}
