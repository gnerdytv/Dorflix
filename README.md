# Dorflix Native Android App

A high-performance video streaming application built with Kotlin and C++ for optimal video processing and user experience.

## Architecture Overview

### Kotlin Layer (App Lifecycle, UI & Gestures, Networking, Feed Logic)

- **MainActivity & Navigation** - App lifecycle management
- **VideoPlayerActivity** - Main video playback interface
- **FeedFragment** - Video catalog and recommendations
- **Networking Layer** - Retrofit-based API client
- **Repository Pattern** - Data abstraction
- **ViewModel Architecture** - State management

### C++ Layer (Video Decoding, Preloading, Frame Timing, Memory Pools)

- **VideoDecoder** - FFmpeg-based video decoding
- **FrameBufferManager** - Memory pool management
- **Preloader** - Intelligent video preloading
- **FrameTimer** - Precise frame timing
- **MemoryPool** - Efficient memory allocation

### Integration

- **JNI Bridge** - Kotlin ↔ C++ communication
- **Shared Memory** - Efficient frame data transfer
- **Synchronized State** - Coordinated playback control

## Project Structure

```
dorflix-native/
├── app/
│   ├── src/main/
│   │   ├── java/com/dorflix/
│   │   │   ├── MainActivity.kt
│   │   │   ├── VideoPlayerActivity.kt
│   │   │   ├── ui/
│   │   │   │   ├── FeedFragment.kt
│   │   │   │   ├── VideoPlayerFragment.kt
│   │   │   │   └── components/
│   │   │   ├── data/
│   │   │   │   ├── repository/
│   │   │   │   ├── api/
│   │   │   │   └── local/
│   │   │   ├── domain/
│   │   │   │   ├── usecase/
│   │   │   │   └── model/
│   │   │   ├── presentation/
│   │   │   │   ├── viewmodel/
│   │   │   │   └── state/
│   │   │   └── di/
│   │   ├── res/
│   │   │   ├── layout/
│   │   │   ├── values/
│   │   │   └── drawable/
│   │   └── AndroidManifest.xml
│   └── build.gradle
├── cpp/
│   ├── CMakeLists.txt
│   ├── video_decoder/
│   │   ├── VideoDecoder.cpp
│   │   ├── VideoDecoder.h
│   │   ├── FrameBufferManager.cpp
│   │   └── FrameBufferManager.h
│   ├── video_preloader/
│   │   ├── Preloader.cpp
│   │   ├── Preloader.h
│   │   └── CacheManager.cpp
│   ├── frame_timing/
│   │   ├── FrameTimer.cpp
│   │   ├── FrameTimer.h
│   │   └── FrameSync.cpp
│   ├── memory/
│   │   ├── MemoryPool.cpp
│   │   ├── MemoryPool.h
│   │   └── BufferAllocator.cpp
│   ├── jni/
│   │   ├── jni_interface.cpp
│   │   └── jni_interface.h
│   └── third_party/
│       └── ffmpeg/ (FFmpeg libraries)
├── build/
│   └── intermediate/
└── gradle/
    ├── wrapper/
    └── build.gradle
```

## Setup Instructions

### Prerequisites

- Android Studio (latest version)
- Android NDK (Native Development Kit)
- CMake
- FFmpeg libraries

### Build Configuration

1. Install Android NDK and CMake via SDK Manager
2. Configure CMake in app/build.gradle
3. Set up FFmpeg libraries in cpp/third_party/
4. Configure JNI paths and library linking

### Development Workflow

1. Kotlin UI development in `app/src/main/java/`
2. C++ video processing in `cpp/` directory
3. JNI integration in `cpp/jni/`
4. Testing with Android emulator or physical device

## Features

### Core Features

- [ ] Device registration and session management
- [ ] Video catalog browsing with pagination
- [ ] Search functionality
- [ ] Social features (likes, comments, shares)
- [ ] Watch progress tracking
- [ ] Continue watching functionality

### Video Processing Features

- [ ] Hardware-accelerated video decoding
- [ ] Adaptive bitrate streaming support
- [ ] Frame-accurate seeking
- [ ] Memory-efficient buffering
- [ ] Background preloading for next videos

## Performance Goals

- Sub-100ms video start time
- Smooth 60fps playback
- Memory usage under 100MB for video processing
- Battery-efficient playback
- Network-optimized preloading

## Development Notes

- Use Kotlin Coroutines for async operations
- Implement proper error handling in JNI layer
- Optimize memory allocation in C++ layer
- Test on various Android API levels
- Profile performance with Android Studio tools
