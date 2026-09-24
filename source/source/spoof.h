#pragma once
#include <intrin.h>

// Call spoofing macros - compatibility layer
// These wrap function calls for anti-cheat evasion

namespace CallSpoofer {
    class SpoofFunction {
    public:
        // Constructor that takes return address - matching slowdown.h usage
        SpoofFunction(void* addr) {}
        // Fallback for any other usage
        template<typename T>
        SpoofFunction(T addr) {}
    };
}

#ifndef SPOOF
#define SPOOF do {} while(0)
#endif

#ifndef SPOOF_FUNC
#define SPOOF_FUNC CallSpoofer::SpoofFunction spoof(_AddressOfReturnAddress());
#endif

#ifndef SPOOF_CALL
#define SPOOF_CALL(func) func
#endif

// String obfuscation macro
#ifndef OBF
#define OBF(str) str
#endif
