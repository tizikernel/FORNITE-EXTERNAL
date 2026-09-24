#ifndef __OBSIDIUM_SDK_H__
#define __OBSIDIUM_SDK_H__

// Obsidium Protector SDK
// Advanced code protection, virtualization, and anti-tampering
// Note: x64 compatible version

#ifdef __cplusplus
extern "C" {
#endif

// Obsidium VM Protection markers
#ifdef _M_X64
    #define OBSIDIUM_VM_START
    #define OBSIDIUM_VM_END
#else
    #define OBSIDIUM_VM_START __asm _emit 0xEB __asm _emit 0x10 __asm _emit 0x4F __asm _emit 0x42 __asm _emit 0x53 __asm _emit 0x49 __asm _emit 0x44 __asm _emit 0x49 __asm _emit 0x55 __asm _emit 0x4D __asm _emit 0x5F __asm _emit 0x56 __asm _emit 0x4D __asm _emit 0x5F __asm _emit 0x30 __asm _emit 0x31 __asm _emit 0x00
    #define OBSIDIUM_VM_END __asm _emit 0xEB __asm _emit 0x10 __asm _emit 0x4F __asm _emit 0x42 __asm _emit 0x53 __asm _emit 0x49 __asm _emit 0x44 __asm _emit 0x49 __asm _emit 0x55 __asm _emit 0x4D __asm _emit 0x5F __asm _emit 0x56 __asm _emit 0x4D __asm _emit 0x5F __asm _emit 0x30 __asm _emit 0x32 __asm _emit 0x00
#endif

// Obsidium Code Mutation
#ifdef _M_X64
    #define OBSIDIUM_MUTATE_START
    #define OBSIDIUM_MUTATE_END
#else
    #define OBSIDIUM_MUTATE_START __asm _emit 0xEB __asm _emit 0x12 __asm _emit 0x4F __asm _emit 0x42 __asm _emit 0x53 __asm _emit 0x49 __asm _emit 0x44 __asm _emit 0x49 __asm _emit 0x55 __asm _emit 0x4D __asm _emit 0x5F __asm _emit 0x4D __asm _emit 0x55 __asm _emit 0x54 __asm _emit 0x41 __asm _emit 0x54 __asm _emit 0x45 __asm _emit 0x5F __asm _emit 0x30 __asm _emit 0x31
    #define OBSIDIUM_MUTATE_END __asm _emit 0xEB __asm _emit 0x12 __asm _emit 0x4F __asm _emit 0x42 __asm _emit 0x53 __asm _emit 0x49 __asm _emit 0x44 __asm _emit 0x49 __asm _emit 0x55 __asm _emit 0x4D __asm _emit 0x5F __asm _emit 0x4D __asm _emit 0x55 __asm _emit 0x54 __asm _emit 0x41 __asm _emit 0x54 __asm _emit 0x45 __asm _emit 0x5F __asm _emit 0x30 __asm _emit 0x32
#endif

// Obsidium Ultra Protection (Maximum security)
#ifdef _M_X64
    #define OBSIDIUM_ULTRA_START
    #define OBSIDIUM_ULTRA_END
#else
    #define OBSIDIUM_ULTRA_START __asm _emit 0xEB __asm _emit 0x12 __asm _emit 0x4F __asm _emit 0x42 __asm _emit 0x53 __asm _emit 0x49 __asm _emit 0x44 __asm _emit 0x49 __asm _emit 0x55 __asm _emit 0x4D __asm _emit 0x5F __asm _emit 0x55 __asm _emit 0x4C __asm _emit 0x54 __asm _emit 0x52 __asm _emit 0x41 __asm _emit 0x5F __asm _emit 0x30 __asm _emit 0x31 __asm _emit 0x00
    #define OBSIDIUM_ULTRA_END __asm _emit 0xEB __asm _emit 0x12 __asm _emit 0x4F __asm _emit 0x42 __asm _emit 0x53 __asm _emit 0x49 __asm _emit 0x44 __asm _emit 0x49 __asm _emit 0x55 __asm _emit 0x4D __asm _emit 0x5F __asm _emit 0x55 __asm _emit 0x4C __asm _emit 0x54 __asm _emit 0x52 __asm _emit 0x41 __asm _emit 0x5F __asm _emit 0x30 __asm _emit 0x32 __asm _emit 0x00
#endif

// String encryption
#ifdef _M_X64
    #define OBSIDIUM_STR_ENCRYPT_START
    #define OBSIDIUM_STR_ENCRYPT_END
#else
    #define OBSIDIUM_STR_ENCRYPT_START __asm _emit 0xEB __asm _emit 0x0E __asm _emit 0x4F __asm _emit 0x42 __asm _emit 0x53 __asm _emit 0x49 __asm _emit 0x44 __asm _emit 0x49 __asm _emit 0x55 __asm _emit 0x4D __asm _emit 0x5F __asm _emit 0x53 __asm _emit 0x54 __asm _emit 0x52 __asm _emit 0x5F __asm _emit 0x30 __asm _emit 0x31
    #define OBSIDIUM_STR_ENCRYPT_END __asm _emit 0xEB __asm _emit 0x0E __asm _emit 0x4F __asm _emit 0x42 __asm _emit 0x53 __asm _emit 0x49 __asm _emit 0x44 __asm _emit 0x49 __asm _emit 0x55 __asm _emit 0x4D __asm _emit 0x5F __asm _emit 0x53 __asm _emit 0x54 __asm _emit 0x52 __asm _emit 0x5F __asm _emit 0x30 __asm _emit 0x32
#endif

// Anti-debugging
#ifdef _M_X64
    #define OBSIDIUM_CHECK_DEBUGGER
    #define OBSIDIUM_CHECK_VM
    #define OBSIDIUM_CHECK_INTEGRITY
    #define OBSIDIUM_PROTECT_MEMORY
    #define OBSIDIUM_PROTECT_API
    #define OBSIDIUM_ENCRYPT_CODE
    #define OBSIDIUM_ANTI_DUMP
    #define OBSIDIUM_HWID_LOCK
#else
    #define OBSIDIUM_CHECK_DEBUGGER __asm _emit 0xEB __asm _emit 0x10 __asm _emit 0x4F __asm _emit 0x42 __asm _emit 0x53 __asm _emit 0x49 __asm _emit 0x44 __asm _emit 0x49 __asm _emit 0x55 __asm _emit 0x4D __asm _emit 0x5F __asm _emit 0x43 __asm _emit 0x48 __asm _emit 0x4B __asm _emit 0x5F __asm _emit 0x44 __asm _emit 0x42 __asm _emit 0x47
    #define OBSIDIUM_CHECK_VM __asm _emit 0xEB __asm _emit 0x0F __asm _emit 0x4F __asm _emit 0x42 __asm _emit 0x53 __asm _emit 0x49 __asm _emit 0x44 __asm _emit 0x49 __asm _emit 0x55 __asm _emit 0x4D __asm _emit 0x5F __asm _emit 0x43 __asm _emit 0x48 __asm _emit 0x4B __asm _emit 0x5F __asm _emit 0x56 __asm _emit 0x4D
    #define OBSIDIUM_CHECK_INTEGRITY __asm _emit 0xEB __asm _emit 0x11 __asm _emit 0x4F __asm _emit 0x42 __asm _emit 0x53 __asm _emit 0x49 __asm _emit 0x44 __asm _emit 0x49 __asm _emit 0x55 __asm _emit 0x4D __asm _emit 0x5F __asm _emit 0x43 __asm _emit 0x48 __asm _emit 0x4B __asm _emit 0x5F __asm _emit 0x49 __asm _emit 0x4E __asm _emit 0x54 __asm _emit 0x47
    #define OBSIDIUM_PROTECT_MEMORY __asm _emit 0xEB __asm _emit 0x11 __asm _emit 0x4F __asm _emit 0x42 __asm _emit 0x53 __asm _emit 0x49 __asm _emit 0x44 __asm _emit 0x49 __asm _emit 0x55 __asm _emit 0x4D __asm _emit 0x5F __asm _emit 0x50 __asm _emit 0x52 __asm _emit 0x4F __asm _emit 0x54 __asm _emit 0x5F __asm _emit 0x4D __asm _emit 0x45 __asm _emit 0x4D
    #define OBSIDIUM_PROTECT_API __asm _emit 0xEB __asm _emit 0x11 __asm _emit 0x4F __asm _emit 0x42 __asm _emit 0x53 __asm _emit 0x49 __asm _emit 0x44 __asm _emit 0x49 __asm _emit 0x55 __asm _emit 0x4D __asm _emit 0x5F __asm _emit 0x50 __asm _emit 0x52 __asm _emit 0x4F __asm _emit 0x54 __asm _emit 0x5F __asm _emit 0x41 __asm _emit 0x50 __asm _emit 0x49
    #define OBSIDIUM_ENCRYPT_CODE __asm _emit 0xEB __asm _emit 0x11 __asm _emit 0x4F __asm _emit 0x42 __asm _emit 0x53 __asm _emit 0x49 __asm _emit 0x44 __asm _emit 0x49 __asm _emit 0x55 __asm _emit 0x4D __asm _emit 0x5F __asm _emit 0x45 __asm _emit 0x4E __asm _emit 0x43 __asm _emit 0x5F __asm _emit 0x43 __asm _emit 0x4F __asm _emit 0x44 __asm _emit 0x45
    #define OBSIDIUM_ANTI_DUMP __asm _emit 0xEB __asm _emit 0x10 __asm _emit 0x4F __asm _emit 0x42 __asm _emit 0x53 __asm _emit 0x49 __asm _emit 0x44 __asm _emit 0x49 __asm _emit 0x55 __asm _emit 0x4D __asm _emit 0x5F __asm _emit 0x41 __asm _emit 0x4E __asm _emit 0x54 __asm _emit 0x49 __asm _emit 0x5F __asm _emit 0x44 __asm _emit 0x4D __asm _emit 0x50
    #define OBSIDIUM_HWID_LOCK __asm _emit 0xEB __asm _emit 0x10 __asm _emit 0x4F __asm _emit 0x42 __asm _emit 0x53 __asm _emit 0x49 __asm _emit 0x44 __asm _emit 0x49 __asm _emit 0x55 __asm _emit 0x4D __asm _emit 0x5F __asm _emit 0x48 __asm _emit 0x57 __asm _emit 0x49 __asm _emit 0x44 __asm _emit 0x5F __asm _emit 0x4C __asm _emit 0x4F __asm _emit 0x4B
#endif

#ifdef __cplusplus
}
#endif

#endif // __OBSIDIUM_SDK_H__
