#ifndef __ENIGMA_SDK_H__
#define __ENIGMA_SDK_H__

// Enigma Protector SDK
// Protection macros for code virtualization, mutation, and encryption
// Note: x64 compatible version

#ifdef __cplusplus
extern "C" {
#endif

// Enigma Virtual Machine markers
#ifdef _M_X64
    #define ENIGMA_VM_START
    #define ENIGMA_VM_END
#else
    #define ENIGMA_VM_START __asm _emit 0xEB __asm _emit 0x0E __asm _emit 0x45 __asm _emit 0x4E __asm _emit 0x49 __asm _emit 0x47 __asm _emit 0x4D __asm _emit 0x41 __asm _emit 0x5F __asm _emit 0x56 __asm _emit 0x4D __asm _emit 0x5F __asm _emit 0x30 __asm _emit 0x31 __asm _emit 0x00
    #define ENIGMA_VM_END __asm _emit 0xEB __asm _emit 0x0E __asm _emit 0x45 __asm _emit 0x4E __asm _emit 0x49 __asm _emit 0x47 __asm _emit 0x4D __asm _emit 0x41 __asm _emit 0x5F __asm _emit 0x56 __asm _emit 0x4D __asm _emit 0x5F __asm _emit 0x30 __asm _emit 0x32 __asm _emit 0x00
#endif

// Enigma Code Mutation markers
#ifdef _M_X64
    #define ENIGMA_MUTATE_START
    #define ENIGMA_MUTATE_END
#else
    #define ENIGMA_MUTATE_START __asm _emit 0xEB __asm _emit 0x10 __asm _emit 0x45 __asm _emit 0x4E __asm _emit 0x49 __asm _emit 0x47 __asm _emit 0x4D __asm _emit 0x41 __asm _emit 0x5F __asm _emit 0x4D __asm _emit 0x55 __asm _emit 0x54 __asm _emit 0x41 __asm _emit 0x54 __asm _emit 0x45 __asm _emit 0x5F __asm _emit 0x30 __asm _emit 0x31
    #define ENIGMA_MUTATE_END __asm _emit 0xEB __asm _emit 0x10 __asm _emit 0x45 __asm _emit 0x4E __asm _emit 0x49 __asm _emit 0x47 __asm _emit 0x4D __asm _emit 0x41 __asm _emit 0x5F __asm _emit 0x4D __asm _emit 0x55 __asm _emit 0x54 __asm _emit 0x41 __asm _emit 0x54 __asm _emit 0x45 __asm _emit 0x5F __asm _emit 0x30 __asm _emit 0x32
#endif

// Enigma Ultra protection (VM + Mutation combined)
#ifdef _M_X64
    #define ENIGMA_ULTRA_START
    #define ENIGMA_ULTRA_END
#else
    #define ENIGMA_ULTRA_START __asm _emit 0xEB __asm _emit 0x10 __asm _emit 0x45 __asm _emit 0x4E __asm _emit 0x49 __asm _emit 0x47 __asm _emit 0x4D __asm _emit 0x41 __asm _emit 0x5F __asm _emit 0x55 __asm _emit 0x4C __asm _emit 0x54 __asm _emit 0x52 __asm _emit 0x41 __asm _emit 0x5F __asm _emit 0x30 __asm _emit 0x31 __asm _emit 0x00
    #define ENIGMA_ULTRA_END __asm _emit 0xEB __asm _emit 0x10 __asm _emit 0x45 __asm _emit 0x4E __asm _emit 0x49 __asm _emit 0x47 __asm _emit 0x4D __asm _emit 0x41 __asm _emit 0x5F __asm _emit 0x55 __asm _emit 0x4C __asm _emit 0x54 __asm _emit 0x52 __asm _emit 0x41 __asm _emit 0x5F __asm _emit 0x30 __asm _emit 0x32 __asm _emit 0x00
#endif

// String encryption markers
#ifdef _M_X64
    #define ENIGMA_STR_ENCRYPT_START
    #define ENIGMA_STR_ENCRYPT_END
#else
    #define ENIGMA_STR_ENCRYPT_START __asm _emit 0xEB __asm _emit 0x0C __asm _emit 0x45 __asm _emit 0x4E __asm _emit 0x49 __asm _emit 0x47 __asm _emit 0x4D __asm _emit 0x41 __asm _emit 0x5F __asm _emit 0x53 __asm _emit 0x54 __asm _emit 0x52 __asm _emit 0x5F __asm _emit 0x30 __asm _emit 0x31
    #define ENIGMA_STR_ENCRYPT_END __asm _emit 0xEB __asm _emit 0x0C __asm _emit 0x45 __asm _emit 0x4E __asm _emit 0x49 __asm _emit 0x47 __asm _emit 0x4D __asm _emit 0x41 __asm _emit 0x5F __asm _emit 0x53 __asm _emit 0x54 __asm _emit 0x52 __asm _emit 0x5F __asm _emit 0x30 __asm _emit 0x32
#endif

// Anti-debugging checks
#ifdef _M_X64
    #define ENIGMA_CHECK_DEBUGGER
    #define ENIGMA_CHECK_VM
    #define ENIGMA_CHECK_CODE
    #define ENIGMA_PROTECT_MEMORY
    #define ENIGMA_PROTECT_IMPORTS
    #define ENIGMA_PROTECT_RESOURCES
#else
    #define ENIGMA_CHECK_DEBUGGER __asm _emit 0xEB __asm _emit 0x0E __asm _emit 0x45 __asm _emit 0x4E __asm _emit 0x49 __asm _emit 0x47 __asm _emit 0x4D __asm _emit 0x41 __asm _emit 0x5F __asm _emit 0x43 __asm _emit 0x48 __asm _emit 0x4B __asm _emit 0x5F __asm _emit 0x44 __asm _emit 0x42 __asm _emit 0x47
    #define ENIGMA_CHECK_VM __asm _emit 0xEB __asm _emit 0x0D __asm _emit 0x45 __asm _emit 0x4E __asm _emit 0x49 __asm _emit 0x47 __asm _emit 0x4D __asm _emit 0x41 __asm _emit 0x5F __asm _emit 0x43 __asm _emit 0x48 __asm _emit 0x4B __asm _emit 0x5F __asm _emit 0x56 __asm _emit 0x4D
    #define ENIGMA_CHECK_CODE __asm _emit 0xEB __asm _emit 0x0F __asm _emit 0x45 __asm _emit 0x4E __asm _emit 0x49 __asm _emit 0x47 __asm _emit 0x4D __asm _emit 0x41 __asm _emit 0x5F __asm _emit 0x43 __asm _emit 0x48 __asm _emit 0x4B __asm _emit 0x5F __asm _emit 0x43 __asm _emit 0x4F __asm _emit 0x44 __asm _emit 0x45
    #define ENIGMA_PROTECT_MEMORY __asm _emit 0xEB __asm _emit 0x0F __asm _emit 0x45 __asm _emit 0x4E __asm _emit 0x49 __asm _emit 0x47 __asm _emit 0x4D __asm _emit 0x41 __asm _emit 0x5F __asm _emit 0x50 __asm _emit 0x52 __asm _emit 0x4F __asm _emit 0x54 __asm _emit 0x5F __asm _emit 0x4D __asm _emit 0x45 __asm _emit 0x4D
    #define ENIGMA_PROTECT_IMPORTS __asm _emit 0xEB __asm _emit 0x0F __asm _emit 0x45 __asm _emit 0x4E __asm _emit 0x49 __asm _emit 0x47 __asm _emit 0x4D __asm _emit 0x41 __asm _emit 0x5F __asm _emit 0x50 __asm _emit 0x52 __asm _emit 0x4F __asm _emit 0x54 __asm _emit 0x5F __asm _emit 0x49 __asm _emit 0x4D __asm _emit 0x50
    #define ENIGMA_PROTECT_RESOURCES __asm _emit 0xEB __asm _emit 0x0F __asm _emit 0x45 __asm _emit 0x4E __asm _emit 0x49 __asm _emit 0x47 __asm _emit 0x4D __asm _emit 0x41 __asm _emit 0x5F __asm _emit 0x50 __asm _emit 0x52 __asm _emit 0x4F __asm _emit 0x54 __asm _emit 0x5F __asm _emit 0x52 __asm _emit 0x45 __asm _emit 0x53
#endif

#ifdef __cplusplus
}
#endif

#endif // __ENIGMA_SDK_H__
