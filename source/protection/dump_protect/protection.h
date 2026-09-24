#pragma once
#include <vector>
#include <string>
#include <windows.h>

namespace Loader {
    class Protection {
    public:
        static void XorData(std::vector<unsigned char>& data, const std::string& key) {
            if (key.empty()) return;
            for (size_t i = 0; i < data.size(); ++i) data[i] ^= key[i % key.length()];
        }
        static bool IsDebuggerAttached() {
            return IsDebuggerPresent();
        }
        static void WipeHeaders(void* base) {
            PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)base;
            PIMAGE_NT_HEADERS nt = (PIMAGE_NT_HEADERS)((size_t)base + dos->e_lfanew);
            
            DWORD old;
            VirtualProtect(base, nt->OptionalHeader.SizeOfHeaders, PAGE_READWRITE, &old);
            
            // Note: We MUST preserve dos->e_magic, dos->e_lfanew, and nt->Signature 
            // for system APIs and the integrity checker to function correctly.
            
            // 1. Wipe everything between the DOS signature and e_lfanew
            SecureZeroMemory((void*)((size_t)base + sizeof(WORD)), offsetof(IMAGE_DOS_HEADER, e_lfanew) - sizeof(WORD));
            
            // 2. Wipe everything between the NT signature and the start of sections
            // We skip the signatures to prevent crashes (like offset 0x3C access).
            size_t nt_header_start = (size_t)base + dos->e_lfanew;
            SecureZeroMemory((void*)(nt_header_start + sizeof(DWORD)), sizeof(IMAGE_NT_HEADERS) - sizeof(DWORD));
            
            // 3. Wipe Section Headers (especially names)
            PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(nt);
            for (WORD i = 0; i < nt->FileHeader.NumberOfSections; i++) {
                SecureZeroMemory(section[i].Name, 8);
                // We keep the VirtualAddress and SizeOfRawData for now to avoid breaking basic PE loading logic
                // but wipe the names to hinder dumpers.
            }
            
            VirtualProtect(base, nt->OptionalHeader.SizeOfHeaders, old, &old);
        }
    };
}
