#include "MemoryLoader.hpp"
#include <windows.h>
#include <winnt.h>

namespace Loader {
    MemoryLoader::MemoryLoader(const std::vector<unsigned char>& data) : m_rawData(data) {}
    MemoryLoader::~MemoryLoader() { Unload(); }

    bool MemoryLoader::Load(ModuleInfo& outInfo) {
        if (m_rawData.empty()) return false;
        if (!MapHeaders() || !MapSections() || !HandleRelocations() || !HandleImports() || !ExecuteTLS()) return false;

        outInfo.handle = (HMODULE)m_imageMapped;
        outInfo.isMapped = true;
        outInfo.size = m_imageSize;
        outInfo.entryPoint = (void*)((size_t)m_imageMapped + m_ntHeaders->OptionalHeader.AddressOfEntryPoint);

        if (m_ntHeaders->OptionalHeader.AddressOfEntryPoint != 0) {
            if (!CallEntryPoint(DLL_PROCESS_ATTACH)) return false;
        }
        return true;
    }

    bool MemoryLoader::MapHeaders() {
        PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)m_rawData.data();
        if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) return false;
        m_ntHeaders = (PIMAGE_NT_HEADERS)(m_rawData.data() + dosHeader->e_lfanew);
        m_imageSize = m_ntHeaders->OptionalHeader.SizeOfImage;
        m_imageMapped = VirtualAlloc(NULL, m_imageSize, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
        if (!m_imageMapped) return false;
        memcpy(m_imageMapped, m_rawData.data(), m_ntHeaders->OptionalHeader.SizeOfHeaders);
        m_ntHeaders = (PIMAGE_NT_HEADERS)((size_t)m_imageMapped + dosHeader->e_lfanew);
        return true;
    }

    bool MemoryLoader::MapSections() {
        PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(m_ntHeaders);
        for (WORD i = 0; i < m_ntHeaders->FileHeader.NumberOfSections; i++, section++) {
            if (section->SizeOfRawData == 0) continue;
            memcpy((void*)((size_t)m_imageMapped + section->VirtualAddress), (void*)(m_rawData.data() + section->PointerToRawData), section->SizeOfRawData);
        }
        return true;
    }

    bool MemoryLoader::HandleRelocations() {
        size_t delta = (size_t)m_imageMapped - m_ntHeaders->OptionalHeader.ImageBase;
        if (delta == 0) return true;
        auto& relocDir = m_ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
        if (relocDir.Size == 0) return true;
        PIMAGE_BASE_RELOCATION reloc = (PIMAGE_BASE_RELOCATION)((size_t)m_imageMapped + relocDir.VirtualAddress);
        while (reloc->VirtualAddress != 0) {
            DWORD count = (reloc->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
            WORD* list = (WORD*)((size_t)reloc + sizeof(IMAGE_BASE_RELOCATION));
            for (DWORD i = 0; i < count; i++) {
                if (list[i] == 0) continue;
                size_t* ptr = (size_t*)((size_t)m_imageMapped + reloc->VirtualAddress + (list[i] & 0xFFF));
                int type = list[i] >> 12;
                if (type == IMAGE_REL_BASED_DIR64) *ptr += delta;
                else if (type == IMAGE_REL_BASED_HIGHLOW) *(DWORD*)ptr += (DWORD)delta;
            }
            reloc = (PIMAGE_BASE_RELOCATION)((size_t)reloc + reloc->SizeOfBlock);
        }
        return true;
    }

    bool MemoryLoader::HandleImports() {
        auto& importDir = m_ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
        if (importDir.Size == 0) return true;
        PIMAGE_IMPORT_DESCRIPTOR desc = (PIMAGE_IMPORT_DESCRIPTOR)((size_t)m_imageMapped + importDir.VirtualAddress);
        while (desc->Name != 0) {
            HMODULE hLib = LoadLibraryA((const char*)((size_t)m_imageMapped + desc->Name));
            if (!hLib) return false;
            PIMAGE_THUNK_DATA thunk = (PIMAGE_THUNK_DATA)((size_t)m_imageMapped + desc->FirstThunk);
            PIMAGE_THUNK_DATA orig = (PIMAGE_THUNK_DATA)((size_t)m_imageMapped + desc->OriginalFirstThunk);
            while (thunk->u1.AddressOfData != 0) {
                size_t lookup = orig ? orig->u1.AddressOfData : thunk->u1.AddressOfData;
                if (IMAGE_SNAP_BY_ORDINAL(lookup)) thunk->u1.Function = (size_t)GetProcAddress(hLib, (LPCSTR)IMAGE_ORDINAL(lookup));
                else thunk->u1.Function = (size_t)GetProcAddress(hLib, ((PIMAGE_IMPORT_BY_NAME)((size_t)m_imageMapped + lookup))->Name);
                if (thunk->u1.Function == 0) return false;
                thunk++; if (orig) orig++;
            }
            desc++;
        }
        return true;
    }

    bool MemoryLoader::ExecuteTLS() {
        auto& tlsDir = m_ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS];
        if (tlsDir.Size == 0) return true;
        PIMAGE_TLS_DIRECTORY tls = (PIMAGE_TLS_DIRECTORY)((size_t)m_imageMapped + tlsDir.VirtualAddress);
        PIMAGE_TLS_CALLBACK* cb = (PIMAGE_TLS_CALLBACK*)tls->AddressOfCallBacks;
        if (cb) { while (*cb) { (*cb)(m_imageMapped, DLL_PROCESS_ATTACH, NULL); cb++; } }
        return true;
    }

    bool MemoryLoader::CallEntryPoint(DWORD reason) {
        typedef BOOL(WINAPI* DllMainPtr)(HINSTANCE, DWORD, LPVOID);
        return ((DllMainPtr)((size_t)m_imageMapped + m_ntHeaders->OptionalHeader.AddressOfEntryPoint))((HINSTANCE)m_imageMapped, reason, NULL);
    }

    bool MemoryLoader::Unload() {
        if (m_imageMapped) {
            if (m_ntHeaders->OptionalHeader.AddressOfEntryPoint != 0) CallEntryPoint(DLL_PROCESS_DETACH);
            VirtualFree(m_imageMapped, 0, MEM_RELEASE);
            m_imageMapped = nullptr;
            return true;
        }
        return false;
    }

    void* MemoryLoader::GetProc(const std::string& name) {
        if (!m_imageMapped) return nullptr;
        auto& expDir = m_ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
        if (expDir.Size == 0) return nullptr;
        PIMAGE_EXPORT_DIRECTORY exports = (PIMAGE_EXPORT_DIRECTORY)((size_t)m_imageMapped + expDir.VirtualAddress);
        DWORD* names = (DWORD*)((size_t)m_imageMapped + exports->AddressOfNames);
        WORD* ords = (WORD*)((size_t)m_imageMapped + exports->AddressOfNameOrdinals);
        DWORD* funcs = (DWORD*)((size_t)m_imageMapped + exports->AddressOfFunctions);
        for (DWORD i = 0; i < exports->NumberOfNames; i++) {
            if (name == (const char*)((size_t)m_imageMapped + names[i])) return (void*)((size_t)m_imageMapped + funcs[ords[i]]);
        }
        return nullptr;
    }
}
