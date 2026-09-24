#pragma once

// ================================================================
//  FATAL CUSTOM PROTECTION
//  Specifically designed to defeat:
//    - ScyllaHide / ScyllaHide x64
//    - x64dbg / OllyDbg with any plugin
//    - Process Hacker / Cheat Engine debugger
//    - Hardware breakpoint injectors
// ================================================================

#include <Windows.h>
#include <tlhelp32.h>
#include <winreg.h>
#include <string>
#include <algorithm>
#include <intrin.h>
#include <thread>
#include "oxorany.h"
#pragma comment(lib, "advapi32.lib")

// Define a minimal AX_PEB with only basic types to avoid ANY structural collisions or missing struct errors.
// This is purely based on x64 offsets.
typedef struct _AX_PEB {
    unsigned char Reserved1[2];      // 0x00
    unsigned char BeingDebugged;     // 0x02
    unsigned char Reserved2[21];     // 0x03 -> 0x18
    void* Ldr;                       // 0x18
    void* ProcessParameters;         // 0x20
    void* Reserved3[1];              // 0x28 -> 0x30
    void* ProcessHeap;               // 0x30
    unsigned char Reserved4[132];    // 0x38 -> 0xBC
    unsigned int  NtGlobalFlag;      // 0xBC
} AX_PEB, * PAX_PEB;

// Suppress false-positive NTSTATUS warning
typedef long NTSTATUS;
#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
#endif

// Basic information for ax_detect_debugger_parent
typedef struct _AX_PROCESS_BASIC_INFORMATION {
    void* Reserved1;
    void* PebBaseAddress;
    void* Reserved2[2];
    ULONG_PTR UniqueProcessId;
    void* InheritedFromUniqueProcessId;
} AX_PROCESS_BASIC_INFORMATION;

typedef NTSTATUS(NTAPI* pNtQueryInfoProcess)(HANDLE, UINT, PVOID, ULONG, PULONG);

// ----------------------------------------------------------------
// 1. NTDLL Hook Detection (ScyllaHide's #1 attack vector)
//    ScyllaHide patches NtQueryInformationProcess, NtGetContextThread etc.
//    We verify the function prologues haven't been overwritten with JMPs
// ----------------------------------------------------------------
inline bool ax_detect_ntdll_hooks() {
    HMODULE hNt = GetModuleHandleA(oxorany("ntdll.dll"));
    if (!hNt) return true;

    const char* targets[] = {
        oxorany("NtQueryInformationProcess"),
        oxorany("NtSetInformationThread"),
        oxorany("NtGetContextThread"),
        oxorany("NtSetContextThread"),
        oxorany("NtQueryObject"),
        oxorany("NtClose"),
        // Removed LdrLoadDll and DbgUiRemoteBreakin - commonly hooked by Discord/Steam overlays and AVs
        oxorany("DbgBreakPoint"), 
    };

    for (const auto& name : targets) {
        BYTE* fn = (BYTE*)GetProcAddress(hNt, name);
        if (!fn) continue;

        // JMP rel32 (E9), JMP [rip+x] (FF 25), MOV RAX imm64 + CALL/JMP (48 B8), short JMP (EB)
        if (fn[0] == 0xE9 ||
            (fn[0] == 0xFF && fn[1] == 0x25) ||
            (fn[0] == 0x48 && fn[1] == 0xB8) ||
            fn[0] == 0xEB) {
            return true;
        }
    }
    return false;
}

// ----------------------------------------------------------------
// 2. Raw PEB Flags Check (direct memory — not API-based)
//    ScyllaHide fakes BeingDebugged but often misses NtGlobalFlag
// ----------------------------------------------------------------
inline bool ax_detect_peb_debug() {
    PAX_PEB peb = (PAX_PEB)__readgsqword(0x60); // x64 PEB via GS register
    if (!peb) return false;

    // BeingDebugged at PEB+0x2
    if (peb->BeingDebugged) return true;

    // NtGlobalFlag at PEB+0xBC — set to 0x70 when debugger attached
    if (peb->NtGlobalFlag & 0x70) return true;

    return false;
}

// ----------------------------------------------------------------
// 3. Heap Flags Check (set by the OS when a debugger is attached)
//    ScyllaHide rarely patches these — often overlooked
// ----------------------------------------------------------------
inline bool ax_detect_heap_debug() {
    // Disabled - highly prone to false positives across different Windows versions/AV software
    return false;
}

// ----------------------------------------------------------------
// 4. ProcessDebugPort check via NtQueryInformationProcess
//    Returns non-null port handle if a kernel debugger is attached
// ----------------------------------------------------------------
inline bool ax_detect_debug_port() {
    HMODULE hNt = GetModuleHandleA(oxorany("ntdll.dll"));
    if (!hNt) return false;

    auto NtQIP = (pNtQueryInfoProcess)GetProcAddress(hNt, oxorany("NtQueryInformationProcess"));
    if (!NtQIP) return false;

    HANDLE debugPort = nullptr;
    NTSTATUS st = NtQIP(GetCurrentProcess(), 0x7 /*ProcessDebugPort*/, &debugPort, sizeof(debugPort), nullptr);
    if (st == STATUS_SUCCESS && debugPort != nullptr) return true;

    return false;
}

// ----------------------------------------------------------------
// 5. ProcessDebugObjectHandle check (ProcessDebugObjectHandle = 0x1E)
//    Separate from DebugPort — harder for ScyllaHide to fake
// ----------------------------------------------------------------
inline bool ax_detect_debug_object() {
    HMODULE hNt = GetModuleHandleA(oxorany("ntdll.dll"));
    if (!hNt) return false;

    auto NtQIP = (pNtQueryInfoProcess)GetProcAddress(hNt, oxorany("NtQueryInformationProcess"));
    if (!NtQIP) return false;

    HANDLE hDbgObj = nullptr;
    NTSTATUS st = NtQIP(GetCurrentProcess(), 0x1E /*ProcessDebugObjectHandle*/, &hDbgObj, sizeof(hDbgObj), nullptr);
    if (st == STATUS_SUCCESS && hDbgObj != nullptr) {
        CloseHandle(hDbgObj);
        return true;
    }
    return false;
}

// ----------------------------------------------------------------
// 6. RDTSC Timing Attack (defeats single-step debugging)
//    A debugger introduces massive delays between instructions
// ----------------------------------------------------------------
inline bool ax_detect_rdtsc() {
    volatile UINT64 t1 = __rdtsc();
    volatile DWORD acc = 0;
    for (volatile int k = 0; k < 1000; k++) acc += k * 3;
    volatile UINT64 t2 = __rdtsc();

    // Increased threshold to 20M to account for system load/stutters on slow hardware
    if ((t2 - t1) > 20000000ULL) return true;
    return false;
}

// ----------------------------------------------------------------
// 7. Parent Process Name Check
//    Returns the name of the suspicious parent process if found
inline std::string ax_detect_debugger_parent() {
    HMODULE hNt = GetModuleHandleA(oxorany("ntdll.dll"));
    if (!hNt) return "";
 
    auto NtQIP = (pNtQueryInfoProcess)GetProcAddress(hNt, oxorany("NtQueryInformationProcess"));
    if (!NtQIP) return "";
 
    AX_PROCESS_BASIC_INFORMATION pbi = {};
    if (NtQIP(GetCurrentProcess(), 0, &pbi, sizeof(pbi), nullptr) != STATUS_SUCCESS)
        return "";
 
    DWORD parentPid = (DWORD)(ULONG_PTR)pbi.InheritedFromUniqueProcessId;
    HANDLE hParent = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, parentPid);
    if (!hParent) return "";
 
    char buf[MAX_PATH] = {};
    DWORD sz = MAX_PATH;
    QueryFullProcessImageNameA(hParent, 0, buf, &sz);
    CloseHandle(hParent);
 
    std::string n = buf;
    std::transform(n.begin(), n.end(), n.begin(), ::tolower);
 
    const char* badParents[] = {
        oxorany("x64dbg"), oxorany("x32dbg"), oxorany("ollydbg"), oxorany("idaq"), oxorany("ida64"),
        oxorany("ida.exe"), oxorany("windbg"), oxorany("processhacker"), oxorany("procexp"),
        oxorany("cheatengine"), oxorany("scylla"), oxorany("dnspy"), oxorany("httpdebug"),
        oxorany("fiddler"), oxorany("wireshark"), oxorany("dbgview"),
    };
 
    for (const auto& bad : badParents) {
        if (n.find(bad) != std::string::npos) {
            // Return the basename
            return n.substr(n.find_last_of("\\/") + 1);
        }
    }
    return "";
}

// ----------------------------------------------------------------
// 8. Injected Module Scan
//    Enumerate ALL modules loaded into our process and check for blacklist
inline std::string ax_detect_all_injected_modules() {
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, GetCurrentProcessId());
    if (hSnap == INVALID_HANDLE_VALUE) return "";
 
    MODULEENTRY32W me; me.dwSize = sizeof(me);
    if (Module32FirstW(hSnap, &me)) {
        do {
            std::wstring wName = me.szModule;
            std::string n(wName.begin(), wName.end());
            std::transform(n.begin(), n.end(), n.begin(), ::tolower);
 
            const char* blacklist[] = {
                oxorany("scylla"), oxorany("hooklibrary"), oxorany("blackbone"), oxorany("extreme"), oxorany("injector"),
                oxorany("cheatengine"), oxorany("speedhack"), oxorany("frida"), oxorany("x64dbg"), oxorany("windbg"),
                oxorany("processhacker"), oxorany("procexp"), oxorany("wireshark"), oxorany("fiddler"), oxorany("charles")
                // Removed wow64log - legitimate system component
            };
 
            for (const auto& b : blacklist) {
                if (n.find(b) != std::string::npos) {
                    CloseHandle(hSnap);
                    return n;
                }
            }
        } while (Module32NextW(hSnap, &me));
    }
    CloseHandle(hSnap);
    return "";
}

// ----------------------------------------------------------------
// 9. DbgUiRemoteBreakin Patch Check
//    ScyllaHide and x64dbg patch this function to prevent break-in
//    A clean system has it start with 48 83 EC (SUB RSP, imm8)
// ----------------------------------------------------------------
inline bool ax_detect_breakin_patch() {
    HMODULE hNt = GetModuleHandleA(oxorany("ntdll.dll"));
    if (!hNt) return false;

    BYTE* fn = (BYTE*)GetProcAddress(hNt, oxorany("DbgUiRemoteBreakin"));
    if (!fn) return false;

    // Patched with RET (C3) or INT3 (CC) = suspicious
    if (fn[0] == 0xC3 || fn[0] == 0xCC) return true;
    // JMP at start = hooked by ScyllaHide
    if (fn[0] == 0xE9 || (fn[0] == 0xFF && fn[1] == 0x25)) return true;

    return false;
}

// ----------------------------------------------------------------
// 10. OutputDebugString trick
//     When a debugger is present, GetLastError() returns 0 after
//     OutputDebugStringA — on a clean system it stays non-zero
// ----------------------------------------------------------------
inline bool ax_detect_output_debug_string() {
    // Disabled - unreliable; logging tools and some AVs often trigger this check
    return false;
}

// ----------------------------------------------------------------
// 11. Named Mutex Detection (from Armadillo debug-blocker scripts)
//     Armadillo's debug-blocker uses OpenMutexA on specific names.
//     We flip it: look for mutexes that known debugger tools create.
// ----------------------------------------------------------------
inline bool ax_detect_debugger_mutexes() {
    const wchar_t* mutexNames[] = {
        L"OLLYDBG_MUTEX",
        L"x64dbg_mutex",
        L"_x32dbg_mutex_",
        L"IDA_Mutex_",
        // Removed DBWinMutex - triggers false positives with system logging tools
    };

    for (const auto& name : mutexNames) {
        HANDLE h = OpenMutexW(SYNCHRONIZE, FALSE, name);
        if (h) {
            CloseHandle(h);
            return true;
        }
    }
    return false;
}

// ----------------------------------------------------------------
// 12. Code Section Writability Check (from Armadillo VirtualProtect analysis)
//     Scripts reveal Armadillo calls VirtualProtect to change code pages.
//     We detect if our own .text has been made writable — sign of a live patcher.
// ----------------------------------------------------------------
inline bool ax_detect_code_section_writable() {
    HMODULE hSelf = GetModuleHandleA(nullptr);
    if (!hSelf) return false;

    BYTE* base = (BYTE*)hSelf;
    PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)base;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return false;

    PIMAGE_NT_HEADERS nt = (PIMAGE_NT_HEADERS)(base + dos->e_lfanew);
    PIMAGE_SECTION_HEADER sec = IMAGE_FIRST_SECTION(nt);

    for (WORD i = 0; i < nt->FileHeader.NumberOfSections; i++, sec++) {
        if (memcmp(sec->Name, ".text", 5) != 0) continue;

        MEMORY_BASIC_INFORMATION mbi = {};
        if (!VirtualQuery(base + sec->VirtualAddress, &mbi, sizeof(mbi))) break;

        // .text should never be writable at runtime
        if (mbi.Protect & PAGE_EXECUTE_READWRITE ||
            mbi.Protect & PAGE_READWRITE ||
            mbi.Protect & PAGE_WRITECOPY ||
            mbi.Protect & PAGE_EXECUTE_WRITECOPY) {
            return true;
        }
        break;
    }
    return false;
}

// ----------------------------------------------------------------
// 13. Code Integrity Hash Check (from ASProtect CRC technique)
//     ASProtect uses CRC to detect patching — we do the same.
//     First call snapshots the .text CRC; subsequent calls compare it.
// ----------------------------------------------------------------
inline DWORD ax_text_crc        = 0;
inline bool  ax_text_crc_ready  = false;

inline DWORD ax_simple_crc(const BYTE* data, size_t len) {
    DWORD crc = 0xDEADBEEF;
    for (size_t i = 0; i < len; i++)
        crc = (crc >> 8) ^ ((crc ^ data[i]) * 0x6B2A9F3DUL);
    return crc ^ 0xFFFFFFFF;
}

inline bool ax_detect_code_integrity() {
    HMODULE hSelf = GetModuleHandleA(nullptr);
    if (!hSelf) return false;

    BYTE* base = (BYTE*)hSelf;
    PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)base;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return false;

    PIMAGE_NT_HEADERS nt = (PIMAGE_NT_HEADERS)(base + dos->e_lfanew);
    PIMAGE_SECTION_HEADER sec = IMAGE_FIRST_SECTION(nt);

    for (WORD i = 0; i < nt->FileHeader.NumberOfSections; i++, sec++) {
        if (memcmp(sec->Name, ".text", 5) != 0) continue;

        BYTE* textStart = base + sec->VirtualAddress;
        size_t textSize = sec->Misc.VirtualSize;

        if (!ax_text_crc_ready) {
            ax_text_crc       = ax_simple_crc(textStart, textSize);
            ax_text_crc_ready = true;
            return false; // First call: snapshot only
        }

        DWORD currentCrc = ax_simple_crc(textStart, textSize);
        if (currentCrc != ax_text_crc) return true; // Code was patched!
        break;
    }
    return false;
}

inline std::string ax_get_parent_process_path() {
    DWORD ppid = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W pe; pe.dwSize = sizeof(pe);
        DWORD myPid = GetCurrentProcessId();
        if (Process32FirstW(hSnap, &pe)) {
            do {
                if (pe.th32ProcessID == myPid) {
                    ppid = pe.th32ParentProcessID;
                    break;
                }
            } while (Process32NextW(hSnap, &pe));
        }
        CloseHandle(hSnap);
    }
    if (ppid == 0) return "Unknown";
    HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, ppid);
    if (!hProc) return "Access Denied";
    char buf[MAX_PATH] = { 0 }; DWORD sz = MAX_PATH;
    std::string path = QueryFullProcessImageNameA(hProc, 0, buf, &sz) ? buf : "Query Failed";
    CloseHandle(hProc); return path;
}

// ----------------------------------------------------------------
// 14. Kernel Debugger Detection (via SharedUserData)
//     Extremely hard to spoof without a driver-level bypass
// ----------------------------------------------------------------
inline bool ax_detect_kernel_debug() {
    // 0x7FFE02D4 is the offset for KdDebuggerEnabled in SharedUserData
    // On x64 systems, this is a stable address.
    return (*(unsigned char*)0x7FFE02D4 & 0x1) || (*(unsigned char*)0x7FFE02D4 & 0x2);
}

inline void ax_run_protection_cycle() {
    std::string triggered;
    std::string appName = "Debugger/Tool";

    auto chk = [&](bool result, const char* name) -> bool {
        if (result) {
            if (!triggered.empty()) triggered += ", ";
            triggered += name;
        }
        return result;
    };

    std::string injected = ax_detect_all_injected_modules();
    if (!injected.empty()) {
        trigger_protection_alert("Malicious Injected DLL Detected: " + injected, injected);
        return;
    }
 
    bool detected = false;
    // detected |= chk(ax_detect_ntdll_hooks(),            "NTDLL Hook"); // Disabled: Conflicts with VMProtect/Themida API hooks
    detected |= chk(ax_detect_peb_debug(),              "PEB Debug Flag");
    detected |= chk(ax_detect_heap_debug(),             "Heap Debug Flag");
    detected |= chk(ax_detect_debug_port(),             "Debug Port");
    detected |= chk(ax_detect_debug_object(),           "Debug Object");
    // detected |= chk(ax_detect_breakin_patch(),          "DbgUiRemoteBreakin Patch"); // Disabled: VMProtect patches this to prevent attaching
    detected |= chk(ax_detect_output_debug_string(),    "OutputDebugString");
    detected |= chk(ax_detect_debugger_mutexes(),       "Debugger Mutex");
    // detected |= chk(ax_detect_code_section_writable(),  "Code Section Writable"); // Disabled: Packers modify section characteristics
    detected |= chk(ax_detect_kernel_debug(),           "Kernel Debugger");

    if (detected) {
        trigger_protection_alert("Aggressive Anti-Debug Indicators: " + triggered, triggered);
    }
}

// TIMING CHECK
inline void ax_run_timing_checks() {
    static DWORD s_lastCheck = 0;
    DWORD now = GetTickCount();
    if ((now - s_lastCheck) < 5000) return;
    s_lastCheck = now;

    std::string badParent = ax_detect_debugger_parent();
    if (!badParent.empty()) {
        trigger_protection_alert("Suspicious Parent Process Detected", badParent);
        return;
    }

    bool detected = false;
    detected |= ax_detect_rdtsc();
    detected |= ax_detect_code_integrity();

    if (detected) {
        trigger_protection_alert("Timing Manipulation / Integrity Violation", "Debugger/Patcher");
    }
}
