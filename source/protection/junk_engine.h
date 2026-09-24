#pragma once
#include <windows.h>
#include <winternl.h>
#include <vector>
#include <string>
#include <thread>
#include <accctrl.h>
#include <aclapi.h>
#include <psapi.h>
#include <array>
#include <memory>
#include "pe_defs.h"
#include "security_utils.h"

#ifndef FATAL_JUNK_ENGINE_INCLUDED
#define FATAL_JUNK_ENGINE_INCLUDED

// Shared Structures
#ifndef JUNK_STRUCTS_DEFINED
#define JUNK_STRUCTS_DEFINED
struct FUNC {
    const char* name;
    FARPROC addr;
    SIZE_T size;
};
#endif

// Shared Global Variables (inline to allow multiple inclusions across TUs)
inline FUNC funcList[] = {
    { "DbgBreakPoint", 0, 2 },
    { "DbgUiRemoteBreakin", 0, 0x54 },
    { "NtContinue", 0, 0x18 }
};

inline DWORD64 Function_Addressz = 0;
inline volatile int chaos_seed = 10003;

namespace srungoat_signature {
    static const unsigned char SIGNATURE[] = { 0x53, 0x52, 0x55, 0x4E, 0x47, 0x4F, 0x41, 0x54 };
    static const char INFO[] = "Protected by Srungoat";
}

// -------------------------------------------------------------------------
// Helper Functions (Moved from slowdown.h)
// -------------------------------------------------------------------------

static inline bool bDataCompare(const BYTE* pData, const BYTE* bMask, const char* szMask) {
    for (; *szMask; ++szMask, ++pData, ++bMask)
        if (*szMask == 'x' && *pData != *bMask)
            return false;
    return (*szMask) == 0;
}

static inline DWORD64 FindPatternz(BYTE* bMask, const char* szMask) {
    MODULEINFO mi{ };
    GetModuleInformation(GetCurrentProcess(), GetModuleHandleA(NULL), &mi, sizeof(mi));
    DWORD64 dwBaseAddress = DWORD64(mi.lpBaseOfDll);
    const auto dwModuleSize = mi.SizeOfImage;
    for (auto i = 0ul; i < dwModuleSize; i++) {
        if (bDataCompare(PBYTE(dwBaseAddress + i), bMask, szMask))
            return DWORD64(dwBaseAddress + i);
    }
    return NULL;
}

static inline auto check_section_integrityz(const char* section_name, bool fix = false) -> bool {
    const auto hmodule = GetModuleHandle(0);
    if (!hmodule) return true;
    const auto base_0 = reinterpret_cast<std::uintptr_t>(hmodule);
    const auto dos_0 = reinterpret_cast<IMAGE_DOS_HEADER*>(base_0);
    if (dos_0->e_magic != IMAGE_DOS_SIGNATURE) return true;
    const auto nt_0 = reinterpret_cast<IMAGE_NT_HEADERS*>(base_0 + dos_0->e_lfanew);
    if (nt_0->Signature != IMAGE_NT_SIGNATURE) return true;
    auto section_0 = IMAGE_FIRST_SECTION(nt_0);
    wchar_t filename[MAX_PATH];
    DWORD size = MAX_PATH;
    QueryFullProcessImageNameW(GetCurrentProcess(), 0, filename, &size);
    const auto file_handle = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (!file_handle || file_handle == INVALID_HANDLE_VALUE) return true;
    const auto file_mapping = CreateFileMapping(file_handle, 0, PAGE_READONLY, 0, 0, 0);
    if (!file_mapping) { CloseHandle(file_handle); return true; }
    const auto base_1 = reinterpret_cast<std::uintptr_t>(MapViewOfFile(file_mapping, FILE_MAP_READ, 0, 0, 0));
    if (!base_1) { CloseHandle(file_mapping); CloseHandle(file_handle); return true; }
    const auto dos_1 = reinterpret_cast<IMAGE_DOS_HEADER*>(base_1);
    const auto nt_1 = reinterpret_cast<IMAGE_NT_HEADERS*>(base_1 + dos_1->e_lfanew);
    auto section_1 = IMAGE_FIRST_SECTION(nt_1);
    bool patched = false;
    for (auto i = 0; i < nt_1->FileHeader.NumberOfSections; ++i, ++section_0, ++section_1) {
        if (strcmp(reinterpret_cast<char*>(section_0->Name), section_name) || !(section_0->Characteristics & IMAGE_SCN_MEM_EXECUTE)) continue;
        for (auto j = 0u; j < section_0->SizeOfRawData; ++j) {
            const auto old_value = *reinterpret_cast<BYTE*>(base_1 + section_1->PointerToRawData + j);
            const auto current_value = *reinterpret_cast<BYTE*>(base_0 + section_0->VirtualAddress + j);
            if (current_value == old_value) continue;
            if (fix) {
                DWORD new_protect{ PAGE_EXECUTE_READWRITE }, old_protect;
                VirtualProtect(reinterpret_cast<void*>(base_0 + section_0->VirtualAddress + j), sizeof(BYTE), new_protect, &old_protect);
                *reinterpret_cast<BYTE*>(base_0 + section_0->VirtualAddress + j) = old_value;
                VirtualProtect(reinterpret_cast<void*>(base_0 + section_0->VirtualAddress + j), sizeof(BYTE), old_protect, &new_protect);
            }
            patched = true;
        }
        break;
    }
    UnmapViewOfFile(reinterpret_cast<void*>(base_1));
    CloseHandle(file_mapping);
    CloseHandle(file_handle);
    return patched;
}

static inline bool LockMemAccessz() {
    bool bSuccess = false;
    HANDLE hToken = nullptr;
    PTOKEN_USER pTokenUser = nullptr;
    DWORD cbBufferSize = 0;
    PACL pACL = nullptr;
    DWORD cbACL = 0;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) return false;
    GetTokenInformation(hToken, TokenUser, nullptr, 0, &cbBufferSize);
    pTokenUser = static_cast<PTOKEN_USER>(malloc(cbBufferSize));
    if (!pTokenUser) { CloseHandle(hToken); return false; }
    if (!GetTokenInformation(hToken, TokenUser, pTokenUser, cbBufferSize, &cbBufferSize)) { free(pTokenUser); CloseHandle(hToken); return false; }
    cbACL = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(pTokenUser->User.Sid);
    pACL = static_cast<PACL>(malloc(cbACL));
    if (pACL) {
        if (InitializeAcl(pACL, cbACL, ACL_REVISION)) {
            if (AddAccessAllowedAce(pACL, ACL_REVISION, SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_TERMINATE, pTokenUser->User.Sid)) {
                bSuccess = ERROR_SUCCESS == SetSecurityInfo(GetCurrentProcess(), SE_KERNEL_OBJECT, DACL_SECURITY_INFORMATION, nullptr, nullptr, pACL, nullptr);
            }
        }
        free(pACL);
    }
    free(pTokenUser);
    CloseHandle(hToken);
    return bSuccess;
}

static inline bool checkAcceleratorIntegrityz() {
    HMODULE hModule = GetModuleHandle(NULL);
    HRSRC hRsrc = FindResource(hModule, MAKEINTRESOURCE(1), RT_ACCELERATOR);
    return hRsrc == NULL;
}

// -------------------------------------------------------------------------
// Main Thread Functions
// -------------------------------------------------------------------------

static inline void anti_attach() {
    while (true) {
        if (IsDebuggerPresent()) error(OBF("Debugger detected!"));
        Sleep(1000);
    }
}

static inline void ________________________________() {
    while (true) {
        if (!checkAcceleratorIntegrityz()) error(OBF("Resource tampering detected!"));
        if (check_section_integrityz(".text", false)) error(OBF("Memory tampering detected!"));
        if (!LockMemAccessz()) error(OBF("Memory protection failure!"));
        if (Function_Addressz == NULL) {
            BYTE pattern[] = "\x48\x89\x74\x24\x00\x57\x48\x81\xec\x00\x00\x00\x00\x49\x8b\xf0";
            Function_Addressz = FindPatternz(pattern, "xxxx?xxxx????xxx") - 0x5;
        }
        if (Function_Addressz && *(BYTE*)Function_Addressz == 0xE9) error(OBF("Code flow compromised!"));
        Sleep(5000);
    }
}

#define START_INTEGRITY_CHECK std::thread([]() { ________________________________(); }).detach()
#define START_ANTI_ATTACH std::thread([]() { anti_attach(); }).detach()

// Restoration of legacy macros used in main.cpp
#define START_ANTI_DEBUG START_ANTI_ATTACH
#define BUG_IDA
#define ADD_FAKE_AUTH_STRINGS
#define JUNK_FUCK_IDA
#define START_ANTI_REVERSE_FILES

#endif
