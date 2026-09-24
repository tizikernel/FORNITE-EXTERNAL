#pragma once

#include <windows.h>
#include <winternl.h>
#include <string>
#include <vector>
#include <time.h>
#include <tlhelp32.h>
#include <intrin.h>

#pragma comment(lib, "ntdll.lib")

namespace Security {

    inline bool CheckDebuggerStatus() {
        if (IsDebuggerPresent()) return true;
        
        BOOL remote = FALSE;
        CheckRemoteDebuggerPresent(GetCurrentProcess(), &remote);
        if (remote) return true;
        
        return false;
    }

    inline bool CheckPEB() {
        PPEB pPeb = (PPEB)__readgsqword(0x60);
        if (pPeb->BeingDebugged) return true;
        
        DWORD ntGlobalFlag = *(PDWORD)((PBYTE)pPeb + 0xBC);
        if (ntGlobalFlag & 0x70) return true;
        
        return false;
    }

    inline bool CheckDebugPort() {
        typedef NTSTATUS(NTAPI* pNtQueryInformationProcess)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);
        pNtQueryInformationProcess NtQueryInfo = (pNtQueryInformationProcess)GetProcAddress(GetModuleHandleA(oxorany("ntdll.dll")), oxorany("NtQueryInformationProcess"));
        
        if (NtQueryInfo) {
            DWORD_PTR debugPort = 0;
            if (NtQueryInfo(GetCurrentProcess(), (PROCESSINFOCLASS)7, &debugPort, sizeof(debugPort), NULL) == 0) {
                if (debugPort != 0) return true;
            }
            
            HANDLE debugObject = NULL;
            if (NtQueryInfo(GetCurrentProcess(), (PROCESSINFOCLASS)30, &debugObject, sizeof(debugObject), NULL) == 0) {
                if (debugObject != NULL) return true;
            }
            
            DWORD debugFlags = 0;
            if (NtQueryInfo(GetCurrentProcess(), (PROCESSINFOCLASS)31, &debugFlags, sizeof(debugFlags), NULL) == 0) {
                if (debugFlags == 0) return true;
            }
        }
        return false;
    }

    inline bool CheckHardwareBreakpoints() {
        CONTEXT ctx = { 0 };
        ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
        if (GetThreadContext(GetCurrentThread(), &ctx)) {
            if (ctx.Dr0 != 0 || ctx.Dr1 != 0 || ctx.Dr2 != 0 || ctx.Dr3 != 0) return true;
        }
        return false;
    }

    inline bool CheckLoadedModules() {
        HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
        if (hSnap == INVALID_HANDLE_VALUE) return false;
        MODULEENTRY32W me = { sizeof(me) };
        if (Module32FirstW(hSnap, &me)) {
            do {
                std::wstring name = me.szModule;
                for (auto& c : name) c = towlower(c);
                if (name.find(L"frida") != std::wstring::npos || 
                    name.find(L"x64dbg") != std::wstring::npos || 
                    name.find(L"cheatengine") != std::wstring::npos ||
                    name.find(L"scylla") != std::wstring::npos ||
                    name.find(L"titanhide") != std::wstring::npos ||
                    name.find(L"ksylla") != std::wstring::npos) {
                    CloseHandle(hSnap);
                    return true;
                }
            } while (Module32NextW(hSnap, &me));
        }
        CloseHandle(hSnap);
        return false;
    }

    inline bool CheckProcesses() {
        HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnap == INVALID_HANDLE_VALUE) return false;
        PROCESSENTRY32W pe = { sizeof(pe) };
        if (Process32FirstW(hSnap, &pe)) {
            do {
                std::wstring name = pe.szExeFile;
                for (auto& c : name) c = towlower(c);
                if (name.find(L"x64dbg.exe") != std::wstring::npos ||
                    name.find(L"x32dbg.exe") != std::wstring::npos ||
                    name.find(L"wireshark.exe") != std::wstring::npos ||
                    name.find(L"fiddler.exe") != std::wstring::npos ||
                    name.find(L"httpdebugger.exe") != std::wstring::npos ||
                    name.find(L"processhacker.exe") != std::wstring::npos ||
                    name.find(L"ida64.exe") != std::wstring::npos ||
                    name.find(L"ida.exe") != std::wstring::npos) {
                    CloseHandle(hSnap);
                    return true;
                }
            } while (Process32NextW(hSnap, &pe));
        }
        CloseHandle(hSnap);
        return false;
    }

    inline bool CheckExceptions() {
        __try {
            RaiseException(DBG_CONTROL_C, 0, 0, NULL);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    inline void HideThread(HANDLE hThread) {
        typedef NTSTATUS(NTAPI* pNtSetInformationThread)(HANDLE, THREADINFOCLASS, PVOID, ULONG);
        pNtSetInformationThread NtSetInfo = (pNtSetInformationThread)GetProcAddress(GetModuleHandleA(oxorany("ntdll.dll")), oxorany("NtSetInformationThread"));
        if (NtSetInfo) {
            NtSetInfo(hThread, (THREADINFOCLASS)0x11, NULL, 0);
        }
    }

    inline void ErasePEHeader() {
        DWORD old;
        PVOID base = GetModuleHandleA(NULL);
        VirtualProtect(base, 4096, PAGE_READWRITE, &old);
        SecureZeroMemory(base, 4096);
        VirtualProtect(base, 4096, old, &old);
    }

    inline void PatchRemoteBreak() {
        HMODULE hNt = GetModuleHandleA(oxorany("ntdll.dll"));
        if (hNt) {
            PVOID pDBP = GetProcAddress(hNt, oxorany("DbgBreakPoint"));
            if (pDBP) {
                DWORD old;
                VirtualProtect(pDBP, 1, PAGE_EXECUTE_READWRITE, &old);
                *(PBYTE)pDBP = 0xC3;
                VirtualProtect(pDBP, 1, old, &old);
            }
        }
    }

    inline bool IsVMDetection() {
        HKEY hKey;
        const char* keys[] = { oxorany("HARDWARE\\ACPI\\DSDT\\VBOX__"), oxorany("HARDWARE\\ACPI\\FADT\\VBOX__"), oxorany("HARDWARE\\ACPI\\RSDT\\VBOX__"), oxorany("SOFTWARE\\Oracle\\VirtualBox Guest Additions"), oxorany("SYSTEM\\CurrentControlSet\\Enum\\PCI\\VEN_80EE&DEV_CAFE") };
        for (int i = 0; i < 5; i++) {
            if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, keys[i], 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
                RegCloseKey(hKey);
                return true;
            }
        }
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, oxorany("HARDWARE\\Description\\System\\CentralProcessor\\0"), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            char data[256];
            DWORD size = sizeof(data);
            if (RegQueryValueExA(hKey, oxorany("ProcessorNameString"), NULL, NULL, (LPBYTE)data, &size) == ERROR_SUCCESS) {
                std::string s(data);
                if (s.find(oxorany("Virtual")) != std::string::npos || s.find(oxorany("VMware")) != std::string::npos || s.find(oxorany("HVM")) != std::string::npos) {
                    RegCloseKey(hKey);
                    return true;
                }
            }
            RegCloseKey(hKey);
        }
        return false;
    }

    inline std::string ComputeHash(const std::string& data, const std::string& key) {
        std::string combined = data + key;
        unsigned int hash = 0;
        for (size_t i = 0; i < combined.length(); i++) {
            hash = ((hash << 5) - hash) + (unsigned char)combined[i];
        }
        char hex[16];
        sprintf_s(hex, "%08x", hash);
        return std::string(hex);
    }

    inline bool PerformChecks() {
        PatchRemoteBreak();
        HideThread(GetCurrentThread());
        if (CheckDebuggerStatus()) return false;
        if (CheckPEB()) return false;
        if (CheckDebugPort()) return false;
        if (CheckHardwareBreakpoints()) return false;
        if (CheckLoadedModules()) return false;
        if (CheckProcesses()) return false;
        if (CheckExceptions()) return false;
        if (IsVMDetection()) return false;
        return true;
    }

    inline std::string ProcessData(const std::string& data, const std::string& key) {
        std::string res = data;
        size_t kl = key.length();
        if (kl == 0) return res;
        for (size_t i = 0; i < res.length(); i++) {
            res[i] ^= key[i % kl];
            res[i] ^= (unsigned char)(i % 256);
        }
        return res;
    }

    inline std::string GetURL() {
        return oxorany("https://link/connect");
    }
}
