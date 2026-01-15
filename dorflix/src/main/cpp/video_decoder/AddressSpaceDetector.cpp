#include "AddressSpaceDetector.h"
#include <android/log.h>
#include <unistd.h>
#include <limits.h>

// Logging macros
#define LOG_TAG_ASD "AddressSpaceDetector"
#define LOGI_ASD(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_ASD, __VA_ARGS__)
#define LOGW_ASD(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG_ASD, __VA_ARGS__)
#define LOGE_ASD(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_ASD, __VA_ARGS__)

// Static members - definition
AddressSpaceDetector::AddressSpace AddressSpaceDetector::s_detectedSpace = AddressSpaceDetector::AddressSpace::UNKNOWN;
bool AddressSpaceDetector::s_detectionDone = false;

AddressSpaceDetector::AddressSpace AddressSpaceDetector::detect() {
    if (s_detectionDone) {
        return s_detectedSpace;
    }

    LOGI_ASD("Detecting address space...");

    // Method 1: Check pointer size at compile time (most reliable)
#if UINTPTR_MAX == UINT32_MAX
    s_detectedSpace = AddressSpace::BIT_32;
    LOGI_ASD("Compile-time detection: 32-bit address space (UINTPTR_MAX = 0x%llx)", (unsigned long long)UINTPTR_MAX);
#elif UINTPTR_MAX == UINT64_MAX
    s_detectedSpace = AddressSpace::BIT_64;
    LOGI_ASD("Compile-time detection: 64-bit address space (UINTPTR_MAX = 0x%llx)", (unsigned long long)UINTPTR_MAX);
#else
    LOGW_ASD("Unknown pointer size, falling back to runtime detection");

    // Method 2: Runtime detection - try to allocate at 4GB boundary
    void* test_ptr = mmap((void*)0x100000000ULL, 4096, PROT_NONE,
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (test_ptr != MAP_FAILED) {
        munmap(test_ptr, 4096);
        s_detectedSpace = AddressSpace::BIT_64;
        LOGI_ASD("Runtime detection: 64-bit (successfully allocated at 4GB boundary)");
    } else {
        s_detectedSpace = AddressSpace::BIT_32;
        LOGI_ASD("Runtime detection: 32-bit (failed to allocate at 4GB boundary)");
    }
#endif

    s_detectionDone = true;

    // Log safety limits
    uintptr_t safe_limit = getSafeMemoryLimit();
    LOGI_ASD("Safe memory limit: 0x%llx (%zu MB)", (unsigned long long)safe_limit,
             safe_limit / (1024 * 1024));

    return s_detectedSpace;
}

uintptr_t AddressSpaceDetector::getSafeMemoryLimit() {
    AddressSpace space = detect();
    switch (space) {
        case AddressSpace::BIT_32:
            return 0x7FFFFFFFULL; // 2GB safe limit for 32-bit
        case AddressSpace::BIT_64:
            return 0xFFFFFFFFFFFFFFFULL; // Very high for 64-bit
        default:
            return 0x7FFFFFFFULL; // Conservative default
    }
}

bool AddressSpaceDetector::is32Bit() {
    return detect() == AddressSpace::BIT_32;
}

bool AddressSpaceDetector::is64Bit() {
    return detect() == AddressSpace::BIT_64;
}

size_t AddressSpaceDetector::getMaxSafeAllocationMB() {
    AddressSpace space = detect();
    switch (space) {
        case AddressSpace::BIT_32:
            return 512; // 512MB max for 32-bit
        case AddressSpace::BIT_64:
            return 2048; // 2GB max for 64-bit (reasonable limit)
        default:
            return 256; // Conservative default
    }
}