#ifndef ANTI_CRACK_HPP
#define ANTI_CRACK_HPP

#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <algorithm>
#include <iterator>
#include <mutex>
#include <atomic>
#include "../security_macros.h"


#pragma comment(lib, "psapi.lib")

namespace AntiCrack {
    
    // Anti-Debug Detection
    class AntiDebug {
    private:
        static inline std::atomic<bool> debugger_detected{false};
        static inline std::mutex debug_mutex;
        
    public:
        static bool IsDebuggerPresent() {
            if (debugger_detected.load()) return true;
            
            std::lock_guard<std::mutex> lock(debug_mutex);
            
            // Method 1: IsDebuggerPresent API
            if (::IsDebuggerPresent()) {
                debugger_detected.store(true);
                return true;
            }
            
            // Method 2: CheckRemoteDebuggerPresent
            BOOL isDebuggerPresent = FALSE;
            if (CheckRemoteDebuggerPresent(GetCurrentProcess(), &isDebuggerPresent) && isDebuggerPresent) {
                debugger_detected.store(true);
                return true;
            }
            
            // Method 3: PEB BeingDebugged flag (simplified)
            #ifdef _WIN64
            BOOLEAN* pBeingDebugged = (BOOLEAN*)(__readgsqword(0x60) + 0x02);
            #else
            BOOLEAN* pBeingDebugged = (BOOLEAN*)(__readfsdword(0x30) + 0x02);
            #endif
            
            if (pBeingDebugged && *pBeingDebugged) {
                debugger_detected.store(true);
                return true;
            }
            
            return false;
        }
        
        static void AntiDebugLoop() {
            std::thread([&]() {
                while (true) {
                    if (IsDebuggerPresent()) {
                        // Trigger protection
                        //ProtectionController::TriggerProtection();
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }).detach();
        }
    };
    
    // Anti-Attach Protection
    class AntiAttach {
    private:
        static inline std::atomic<bool> protection_triggered{false};
        
    public:
        static void BlockDebuggerAttach() {
            if (protection_triggered.load()) return;
            
            // Set process flags to prevent debugging
            HANDLE hProcess = GetCurrentProcess();
            
            // Method 1: SetProcessDEPPolicy
            HMODULE hKernel32 = GetModuleHandleA(oxorany("kernel32.dll"));
            if (hKernel32) {
                typedef BOOL(WINAPI* pSetProcessDEPPolicy)(DWORD);
                pSetProcessDEPPolicy SetProcessDEPPolicy = (pSetProcessDEPPolicy)GetProcAddress(hKernel32, oxorany("SetProcessDEPPolicy"));
                if (SetProcessDEPPolicy) {
                    SetProcessDEPPolicy(0x00000001); // DEP_ENABLE
                }
            }
            
            // Method 2: SetProcessShutdownParameters
            SetProcessShutdownParameters(0x3FF, SHUTDOWN_NORETRY);
            
            // Method 3: SetProcessPriorityClass
            SetPriorityClass(hProcess, HIGH_PRIORITY_CLASS);
            
            protection_triggered.store(true);
        }
        
        static void MonitorProcessCreation() {
            std::thread([&]() {
                while (true) {
                    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
                    if (hSnapshot != INVALID_HANDLE_VALUE) {
                        PROCESSENTRY32 pe32;
                        pe32.dwSize = sizeof(PROCESSENTRY32);
                        
                        if (Process32First(hSnapshot, &pe32)) {
                            do {
                                // Check for known debugger processes
#ifdef UNICODE
                                std::wstring processNameW = pe32.szExeFile;
                                std::string processName;
                                processName.reserve(processNameW.size());
                                std::transform(processNameW.begin(), processNameW.end(), std::back_inserter(processName), [](wchar_t ch) {
                                    return static_cast<char>(ch);
                                });
#else
                                std::string processName = pe32.szExeFile;
#endif
                                std::transform(processName.begin(), processName.end(), processName.begin(), ::tolower);
                                
                                if (processName.find(oxorany("ollydbg")) != std::string::npos ||
                                    processName.find(oxorany("x64dbg")) != std::string::npos ||
                                    processName.find(oxorany("windbg")) != std::string::npos ||
                                    processName.find(oxorany("ida")) != std::string::npos ||
                                    processName.find(oxorany("ghidra")) != std::string::npos ||
                                    processName.find(oxorany("cheatengine")) != std::string::npos ||
                                    processName.find(oxorany("processhacker")) != std::string::npos) {
                                    
                                    // Kill the debugger process
                                    HANDLE hDebugger = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
                                    if (hDebugger) {
                                        TerminateProcess(hDebugger, 0);
                                        CloseHandle(hDebugger);
                                    }
                                }
                            } while (Process32Next(hSnapshot, &pe32));
                        }
                        CloseHandle(hSnapshot);
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(2500)); // PREVENT CPU KERNEL HANG
                }
            }).detach();
        }
    };
    
    // Anti-Reverse Engineering
    class AntiReverse {
    private:
        static inline std::atomic<bool> analysis_detected{false};
        static inline std::mutex analysis_mutex;
        
    public:
        static bool DetectAnalysisTools() {
            if (analysis_detected.load()) return true;
            
            std::lock_guard<std::mutex> lock(analysis_mutex);
            
            // Check for analysis tools in memory
            HMODULE hModules[1024];
            DWORD cbNeeded;
            if (EnumProcessModules(GetCurrentProcess(), hModules, sizeof(hModules), &cbNeeded)) {
                for (unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
                    char szModName[MAX_PATH];
                    if (GetModuleBaseNameA(GetCurrentProcess(), hModules[i], szModName, sizeof(szModName))) {
                        std::string moduleName = szModName;
                        std::transform(moduleName.begin(), moduleName.end(), moduleName.begin(), ::tolower);
                        
                        if (moduleName.find(oxorany("scylla")) != std::string::npos ||
                            moduleName.find(oxorany("peid")) != std::string::npos ||
                            moduleName.find(oxorany("detectit")) != std::string::npos ||
                            moduleName.find(oxorany("protection_id")) != std::string::npos) {
                            analysis_detected.store(true);
                            return true;
                        }
                    }
                }
            }
            
            return false;
        }
        
        static void ObfuscateStrings() {
            // Simple string obfuscation
            std::vector<std::string> strings = {"debug", "analysis", "reverse", "dump"};
            for (auto& str : strings) {
                for (char& c : str) {
                    c = c ^ 0xAA;
                }
            }
        }
    };
    
    // Anti-Dump Protection
    class AntiDump {
    private:
        static inline std::atomic<bool> dump_protection_active{false};
        
    public:
        static void EnableDumpProtection() {
            if (dump_protection_active.load()) return;
            
            // Method 1: SetProcessDEPPolicy
            HMODULE hKernel32 = GetModuleHandleA(oxorany("kernel32.dll"));
            if (hKernel32) {
                typedef BOOL(WINAPI* pSetProcessDEPPolicy)(DWORD);
                pSetProcessDEPPolicy SetProcessDEPPolicy = (pSetProcessDEPPolicy)GetProcAddress(hKernel32, oxorany("SetProcessDEPPolicy"));
                if (SetProcessDEPPolicy) {
                    SetProcessDEPPolicy(0x00000001);
                }
            }
            
            // Method 2: VirtualProtect to make sections non-readable
            HMODULE hModule = GetModuleHandle(NULL);
            if (hModule) {
                PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)hModule;
                PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS)((BYTE*)hModule + pDosHeader->e_lfanew);
                PIMAGE_SECTION_HEADER pSectionHeader = (PIMAGE_SECTION_HEADER)((BYTE*)pNtHeaders + sizeof(IMAGE_NT_HEADERS));
                
                for (int i = 0; i < pNtHeaders->FileHeader.NumberOfSections; i++) {
                    if (strcmp((char*)pSectionHeader[i].Name, ".text") == 0) {
                        DWORD oldProtect;
                        VirtualProtect((LPVOID)((BYTE*)hModule + pSectionHeader[i].VirtualAddress),
                                     pSectionHeader[i].Misc.VirtualSize,
                                     PAGE_EXECUTE_READ, &oldProtect);
                    }
                }
            }
            
            dump_protection_active.store(true);
        }
        
        static void MonitorMemoryAccess() {
            std::thread([&]() {
                while (true) {
                    MEMORY_BASIC_INFORMATION mbi;
                    LPVOID address = 0;
                    
                    while (VirtualQuery(address, &mbi, sizeof(mbi))) {
                        if (mbi.State == MEM_COMMIT && mbi.Protect == PAGE_EXECUTE_READWRITE) {
                            // Suspicious memory region detected
                            DWORD oldProtect;
                            VirtualProtect(mbi.BaseAddress, mbi.RegionSize, PAGE_EXECUTE_READ, &oldProtect);
                        }
                        address = (LPVOID)((BYTE*)mbi.BaseAddress + mbi.RegionSize);
                    }
                    
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                }
            }).detach();
        }
    };
    
    // KeyAuth Integration
    class KeyAuthProtection {
    private:
        static inline std::atomic<bool> auth_verified{false};
        static inline std::string license_key;
        static inline std::mutex auth_mutex;
        
    public:
        static bool VerifyLicense(const std::string& key) {
            if (auth_verified.load()) return true;
            
            std::lock_guard<std::mutex> lock(auth_mutex);
            
            // Simple license verification (replace with actual KeyAuth implementation)
            if (key.length() >= 16 && key.find("FN") != std::string::npos) {
                license_key = key;
                auth_verified.store(true);
                return true;
            }
            
            return false;
        }
        
        static bool IsLicenseValid() {
            return auth_verified.load();
        }
        
        static void ValidateLicensePeriodically() {
            std::thread([&]() {
                while (true) {
                    if (!auth_verified.load()) {
                        // License expired or invalid
                        //ProtectionController::TriggerProtection();
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(30000)); // Check every 30 seconds
                }
            }).detach();
        }
    };
    
    // Main Protection Controller
    class ProtectionController {
    private:
        static inline std::atomic<bool> protection_active{false};
        static inline std::mutex controller_mutex;
        
    public:
        static void InitializeProtection() {
            if (protection_active.load()) return;
            
            std::lock_guard<std::mutex> lock(controller_mutex);
            
            // Initialize all protection modules
            AntiDebug::AntiDebugLoop();
            AntiAttach::BlockDebuggerAttach();
            AntiAttach::MonitorProcessCreation();
            AntiReverse::ObfuscateStrings();
            AntiDump::EnableDumpProtection();
            AntiDump::MonitorMemoryAccess();
            
            protection_active.store(true);
        }
        
        static void TriggerProtection() {
            // Implement protection measures
            std::thread([&]() {
                // Corrupt memory
                volatile int* ptr = new int[1000];
                for (int i = 0; i < 1000; i++) {
                    ptr[i] = (int)((i * 0x12345678) ^ 0xDEADBEEF);
                }
                delete[] ptr;
                
                // Exit process
                ExitProcess(0);
            }).detach();
        }
        
        static bool IsProtectionActive() {
            return protection_active.load();
        }
    };
    
    // Auto-initialization
    namespace {
        struct AutoInit {
            AutoInit() {
                ProtectionController::InitializeProtection();
            }
        } auto_init;
    }
}

// Protection macros
#define ENABLE_PROTECTION() AntiCrack::ProtectionController::InitializeProtection()
#define VERIFY_LICENSE(key) AntiCrack::KeyAuthProtection::VerifyLicense(key)
#define IS_PROTECTED() AntiCrack::ProtectionController::IsProtectionActive()

#endif // ANTI_CRACK_HPP

