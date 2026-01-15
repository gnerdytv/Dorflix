#ifndef ADDRESS_SPACE_DETECTOR_H
#define ADDRESS_SPACE_DETECTOR_H

#include <cstdint>
#include <sys/mman.h>

class AddressSpaceDetector {
public:
    enum class AddressSpace {
        UNKNOWN,
        BIT_32,    // 32-bit: max address 0xFFFFFFFF (4GB)
        BIT_64     // 64-bit: much higher limits
    };

    static AddressSpace detect();

    static uintptr_t getSafeMemoryLimit();

    static bool is32Bit();

    static bool is64Bit();

    // Get maximum safe allocation size for this address space
    static size_t getMaxSafeAllocationMB();

private:
    static AddressSpace s_detectedSpace;
    static bool s_detectionDone;
};

#endif // ADDRESS_SPACE_DETECTOR_H