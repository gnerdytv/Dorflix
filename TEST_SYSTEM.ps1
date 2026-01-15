#!/usr/bin/env pwsh

# Test script for Dorflix system
Write-Host "Testing Dorflix Video Streaming System..." -ForegroundColor Green
Write-Host "=========================================" -ForegroundColor Green

# Test 1: Backend TypeScript compilation
Write-Host "`n1. Testing Backend TypeScript compilation..." -ForegroundColor Yellow
Set-Location backend
try {
    $result = npm run build 2>&1
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✅ Backend TypeScript compilation: PASSED" -ForegroundColor Green
    } else {
        Write-Host "❌ Backend TypeScript compilation: FAILED" -ForegroundColor Red
        Write-Host "Error: $result"
    }
} catch {
    Write-Host "❌ Backend TypeScript compilation: FAILED" -ForegroundColor Red
    Write-Host "Error: $_"
}

# Test 2: Backend dependencies
Write-Host "`n2. Testing Backend dependencies..." -ForegroundColor Yellow
try {
    $packageJson = Get-Content package.json | ConvertFrom-Json
    if ($packageJson.dependencies.axios) {
        Write-Host "✅ Axios dependency: INSTALLED" -ForegroundColor Green
    } else {
        Write-Host "❌ Axios dependency: MISSING" -ForegroundColor Red
    }
    
    if ($packageJson.devDependencies.'@types/node') {
        Write-Host "✅ @types/node dependency: INSTALLED" -ForegroundColor Green
    } else {
        Write-Host "❌ @types/node dependency: MISSING" -ForegroundColor Red
    }
} catch {
    Write-Host "❌ Package.json read error: $_" -ForegroundColor Red
}

# Test 3: C++ files exist
Write-Host "`n3. Testing C++ implementation files..." -ForegroundColor Yellow
Set-Location ../dorflix/src/main/cpp
$cppFiles = @(
    "video_decoder/VideoDecoder.h",
    "video_decoder/VideoDecoder.cpp", 
    "video_decoder/FrameBufferManager.h",
    "video_decoder/FrameBufferManager.cpp",
    "video_preloader/Preloader.h",
    "video_preloader/Preloader.cpp",
    "CMakeLists.txt"
)

$allExist = $true
foreach ($file in $cppFiles) {
    if (Test-Path $file) {
        Write-Host "✅ $file: EXISTS" -ForegroundColor Green
    } else {
        Write-Host "❌ $file: MISSING" -ForegroundColor Red
        $allExist = $false
    }
}

if ($allExist) {
    Write-Host "✅ All C++ files: PRESENT" -ForegroundColor Green
} else {
    Write-Host "❌ Some C++ files: MISSING" -ForegroundColor Red
}

# Test 4: CMakeLists.txt configuration
Write-Host "`n4. Testing CMakeLists.txt configuration..." -ForegroundColor Yellow
$cMakeContent = Get-Content CMakeLists.txt -Raw
if ($cMakeContent -match "FFMPEG") {
    Write-Host "✅ FFmpeg integration: CONFIGURED" -ForegroundColor Green
} else {
    Write-Host "❌ FFmpeg integration: NOT CONFIGURED" -ForegroundColor Red
}

if ($cMakeContent -match "pkg_check_modules") {
    Write-Host "✅ Pkg-config integration: CONFIGURED" -ForegroundColor Green
} else {
    Write-Host "❌ Pkg-config integration: NOT CONFIGURED" -ForegroundColor Red
}

# Test 5: Android build configuration
Write-Host "`n5. Testing Android build configuration..." -ForegroundColor Yellow
Set-Location ../../..
if (Test-Path "dorflix/build.gradle") {
    $gradleContent = Get-Content dorflix/build.gradle -Raw
    if ($gradleContent -match "ndkVersion") {
        Write-Host "✅ NDK version: CONFIGURED" -ForegroundColor Green
    } else {
        Write-Host "❌ NDK version: NOT CONFIGURED" -ForegroundColor Red
    }
    
    if ($gradleContent -match "FFmpeg") {
        Write-Host "✅ FFmpeg configuration: CONFIGURED" -ForegroundColor Green
    } else {
        Write-Host "❌ FFmpeg configuration: NOT CONFIGURED" -ForegroundColor Red
    }
} else {
    Write-Host "❌ build.gradle: MISSING" -ForegroundColor Red
}

# Test 6: Performance optimization documentation
Write-Host "`n6. Testing Performance optimization documentation..." -ForegroundColor Yellow
if (Test-Path "PERFORMANCE_OPTIMIZATION.md") {
    Write-Host "✅ Performance optimization guide: CREATED" -ForegroundColor Green
} else {
    Write-Host "❌ Performance optimization guide: MISSING" -ForegroundColor Red
}

# Summary
Write-Host "`n=========================================" -ForegroundColor Green
Write-Host "Test Summary:" -ForegroundColor Yellow
Write-Host "- Backend TypeScript compilation and dependencies" -ForegroundColor White
Write-Host "- C++ FFmpeg video decoder implementation" -ForegroundColor White  
Write-Host "- Frame buffer management system" -ForegroundColor White
Write-Host "- Video preloading system" -ForegroundColor White
Write-Host "- Android NDK configuration" -ForegroundColor White
Write-Host "- Performance optimization documentation" -ForegroundColor White
Write-Host "=========================================" -ForegroundColor Green

Write-Host "`nTo build the system:" -ForegroundColor Cyan
Write-Host "1. Backend: cd backend && npm run build" -ForegroundColor Cyan
Write-Host "2. Android: ./gradlew build" -ForegroundColor Cyan
Write-Host "3. C++: Android build will compile C++ code automatically" -ForegroundColor Cyan
