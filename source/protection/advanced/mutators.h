#ifndef MUTATORS_H
#define MUTATORS_H

#include <intrin.h>
#include "ThemidaSDK.h"
#include "ObsidiumSDK.h"
#include "EnigmaSDK.h"

//=============================================================================
// CUSTOM X64 JUNK CODE & OBFUSCATION MACROS
//=============================================================================
// These macros use volatile variables, impossible branches, and 
// math calculations to confuse reverse engineering tools like IDA Pro and Ghidra.
// It will bloat the code flow and add dead code analysis complexity.
//=============================================================================

#define JUNK_CODE_1 \
    { \
        volatile int __trash_1 = 1337; \
        volatile int __trash_2 = 80085; \
        volatile int __trash_3 = __trash_1 ^ __trash_2; \
        if (__trash_3 == 123456) { \
            __trash_1 = 0; \
        } else { \
            __trash_2 ^= __trash_1; \
            __trash_1 += __trash_2; \
        } \
    }

#define JUNK_CODE_2 \
    { \
        volatile double __f_trash1 = 3.14159; \
        volatile double __f_trash2 = 2.71828; \
        if ((__f_trash1 * __f_trash2) < 0.0) { \
            __f_trash1 += 1.0; \
        } \
        volatile int __i_trash = (int)__f_trash1; \
        __i_trash ^= 0xDEADBEEF; \
    }

#define JUNK_CODE_3 \
    { \
        volatile unsigned long long __p_trash1 = __rdtsc(); \
        volatile unsigned long long __p_trash2 = __p_trash1 * 0x123456789ULL; \
        if (__p_trash1 > __p_trash2) { \
            __p_trash1 ^= __p_trash2; \
        } \
    }

// --- FAKE THEMIDA / WINLICENSE ---
#ifndef THEMIDA_VK_START
    #define THEMIDA_VK_START JUNK_CODE_1; JUNK_CODE_2; JUNK_CODE_3;
    #define THEMIDA_VK_END   JUNK_CODE_3; JUNK_CODE_1;
    #define THEMIDA_MUTATE_START JUNK_CODE_2;
    #define THEMIDA_MUTATE_END   JUNK_CODE_1;
#endif

// --- FAKE VMPROTECT ---
#ifndef VMP_ULTRA_START
    #define VMP_ULTRA_START JUNK_CODE_2; JUNK_CODE_3; JUNK_CODE_1;
    #define VMP_MUTATION_START JUNK_CODE_3;
    #define VMP_END JUNK_CODE_1; JUNK_CODE_2;
#endif

// --- FAKE OBSIDIUM ---
#ifndef OBSIDIUM_VM_START
    #define OBSIDIUM_VM_START JUNK_CODE_3; JUNK_CODE_1;
    #define OBSIDIUM_VM_END JUNK_CODE_2; JUNK_CODE_3;
    #define OBSIDIUM_ENC_START JUNK_CODE_1;
    #define OBSIDIUM_ENC_END JUNK_CODE_2;
#endif

// --- FAKE ENIGMA PROTECTOR ---
#ifndef ENIGMA_VM_START
    #define ENIGMA_VM_START JUNK_CODE_1; JUNK_CODE_3; JUNK_CODE_2;
    #define ENIGMA_VM_END   JUNK_CODE_3; JUNK_CODE_1; JUNK_CODE_2;
#endif

// General Use Macros for the Application Logic
#define PROTECT_START \
    THEMIDA_VK_START \
    VMP_ULTRA_START \
    OBSIDIUM_VM_START \
    ENIGMA_VM_START

#define PROTECT_END \
    ENIGMA_VM_END \
    OBSIDIUM_VM_END \
    VMP_END \
    THEMIDA_VK_END

#endif // MUTATORS_H
