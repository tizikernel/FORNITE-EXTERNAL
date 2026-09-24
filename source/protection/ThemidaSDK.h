#ifndef __THEMIDA_SDK_H__
#define __THEMIDA_SDK_H__

// Themida/WinLicense SDK Macros
// These macros are used to mark code sections for protection
// Note: x64 compatible version - markers will be recognized by Themida packer

#ifdef __cplusplus
extern "C" {
#endif

// VM Macros - Virtualize code sections
#ifdef _M_X64
    #define VM_START __pragma(section(".themida_vm", read, execute))
    #define VM_END
#else
    #define VM_START __asm _emit 0xEB __asm _emit 0x10 __asm _emit 0x56 __asm _emit 0x4D __asm _emit 0x50 __asm _emit 0x72 __asm _emit 0x6F __asm _emit 0x74 __asm _emit 0x65 __asm _emit 0x63 __asm _emit 0x74 __asm _emit 0x20 __asm _emit 0x62 __asm _emit 0x65 __asm _emit 0x67 __asm _emit 0x69 __asm _emit 0x6E __asm _emit 0x00
    #define VM_END __asm _emit 0xEB __asm _emit 0x0E __asm _emit 0x56 __asm _emit 0x4D __asm _emit 0x50 __asm _emit 0x72 __asm _emit 0x6F __asm _emit 0x74 __asm _emit 0x65 __asm _emit 0x63 __asm _emit 0x74 __asm _emit 0x20 __asm _emit 0x65 __asm _emit 0x6E __asm _emit 0x64 __asm _emit 0x00
#endif

// Mutate Macros - Code mutation for obfuscation
#ifdef _M_X64
    #define MUTATE_START
    #define MUTATE_END
#else
    #define MUTATE_START __asm _emit 0xEB __asm _emit 0x0F __asm _emit 0x4D __asm _emit 0x75 __asm _emit 0x74 __asm _emit 0x61 __asm _emit 0x74 __asm _emit 0x65 __asm _emit 0x20 __asm _emit 0x53 __asm _emit 0x74 __asm _emit 0x61 __asm _emit 0x72 __asm _emit 0x74 __asm _emit 0x00
    #define MUTATE_END __asm _emit 0xEB __asm _emit 0x0D __asm _emit 0x4D __asm _emit 0x75 __asm _emit 0x74 __asm _emit 0x61 __asm _emit 0x74 __asm _emit 0x65 __asm _emit 0x20 __asm _emit 0x45 __asm _emit 0x6E __asm _emit 0x64 __asm _emit 0x00
#endif

// STR_ENCRYPT - Encrypt strings
#ifdef _M_X64
    #define STR_ENCRYPT_START
    #define STR_ENCRYPT_END
#else
    #define STR_ENCRYPT_START __asm _emit 0xEB __asm _emit 0x0B __asm _emit 0x53 __asm _emit 0x54 __asm _emit 0x52 __asm _emit 0x5F __asm _emit 0x45 __asm _emit 0x4E __asm _emit 0x43 __asm _emit 0x52 __asm _emit 0x59 __asm _emit 0x50 __asm _emit 0x54 __asm _emit 0x00
    #define STR_ENCRYPT_END __asm _emit 0xEB __asm _emit 0x09 __asm _emit 0x53 __asm _emit 0x54 __asm _emit 0x52 __asm _emit 0x5F __asm _emit 0x45 __asm _emit 0x4E __asm _emit 0x44 __asm _emit 0x00
#endif

// CLEAR_START/END - Clear code from memory after execution
#ifdef _M_X64
    #define CLEAR_START
    #define CLEAR_END
#else
    #define CLEAR_START __asm _emit 0xEB __asm _emit 0x0B __asm _emit 0x43 __asm _emit 0x4C __asm _emit 0x45 __asm _emit 0x41 __asm _emit 0x52 __asm _emit 0x5F __asm _emit 0x53 __asm _emit 0x54 __asm _emit 0x41 __asm _emit 0x52 __asm _emit 0x54 __asm _emit 0x00
    #define CLEAR_END __asm _emit 0xEB __asm _emit 0x09 __asm _emit 0x43 __asm _emit 0x4C __asm _emit 0x45 __asm _emit 0x41 __asm _emit 0x52 __asm _emit 0x5F __asm _emit 0x45 __asm _emit 0x4E __asm _emit 0x44 __asm _emit 0x00
#endif

// CHECK_PROTECTION - Verify protection is active
#ifdef _M_X64
    #define CHECK_PROTECTION
#else
    #define CHECK_PROTECTION __asm _emit 0xEB __asm _emit 0x0F __asm _emit 0x43 __asm _emit 0x48 __asm _emit 0x45 __asm _emit 0x43 __asm _emit 0x4B __asm _emit 0x5F __asm _emit 0x50 __asm _emit 0x52 __asm _emit 0x4F __asm _emit 0x54 __asm _emit 0x45 __asm _emit 0x43 __asm _emit 0x54 __asm _emit 0x00
#endif

// CHECK_DEBUGGER - Check for debugger
#ifdef _M_X64
    #define CHECK_DEBUGGER
#else
    #define CHECK_DEBUGGER __asm _emit 0xEB __asm _emit 0x0D __asm _emit 0x43 __asm _emit 0x48 __asm _emit 0x45 __asm _emit 0x43 __asm _emit 0x4B __asm _emit 0x5F __asm _emit 0x44 __asm _emit 0x45 __asm _emit 0x42 __asm _emit 0x55 __asm _emit 0x47 __asm _emit 0x00
#endif

// CHECK_VIRTUAL_PC - Check for virtual machine
#ifdef _M_X64
    #define CHECK_VIRTUAL_PC
#else
    #define CHECK_VIRTUAL_PC __asm _emit 0xEB __asm _emit 0x0F __asm _emit 0x43 __asm _emit 0x48 __asm _emit 0x45 __asm _emit 0x43 __asm _emit 0x4B __asm _emit 0x5F __asm _emit 0x56 __asm _emit 0x49 __asm _emit 0x52 __asm _emit 0x54 __asm _emit 0x55 __asm _emit 0x41 __asm _emit 0x4C __asm _emit 0x00
#endif

// Themida Macros (alternative names)
#define TIGER_START VM_START
#define TIGER_END VM_END
#define FISH_START MUTATE_START
#define FISH_END MUTATE_END
#define SHARK_START STR_ENCRYPT_START
#define SHARK_END STR_ENCRYPT_END

#ifdef __cplusplus
}
#endif

#endif // __THEMIDA_SDK_H__
