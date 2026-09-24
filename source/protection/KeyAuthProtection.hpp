#pragma once
#include <intrin.h>
#include <psapi.h>
#include <string>
#include <thread>
#include <tlhelp32.h>
#include <vector>
#include <windows.h>
#include <winternl.h>
#include "security_error.hpp"

#pragma comment(lib, "ntdll.lib")

namespace KeyAuthProtection {

// Advanced Anti-Debugging using NT API
inline bool CheckDebugPort() {
  DWORD_PTR debugPort = 0;
  NTSTATUS status =
      NtQueryInformationProcess(GetCurrentProcess(), ProcessDebugPort,
                                &debugPort, sizeof(debugPort), NULL);
  return (status == 0 && debugPort != 0);
}

inline bool CheckDebugObject() {
  HANDLE hDebugObject = NULL;
  NTSTATUS status =
      NtQueryInformationProcess(GetCurrentProcess(), (PROCESSINFOCLASS)0x1E,
                                &hDebugObject, sizeof(hDebugObject), NULL);
  return (status == 0 && hDebugObject != NULL);
}

inline bool CheckDebugFlags() {
  DWORD debugFlags = 0;
  NTSTATUS status =
      NtQueryInformationProcess(GetCurrentProcess(), (PROCESSINFOCLASS)0x1F,
                                &debugFlags, sizeof(debugFlags), NULL);
  return (status == 0 && debugFlags == 0);
}

// Hardware Breakpoint Detection
inline bool CheckHardwareBreakpoints() {
  CONTEXT ctx = {0};
  ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
  if (GetThreadContext(GetCurrentThread(), &ctx)) {
    if (ctx.Dr0 != 0 || ctx.Dr1 != 0 || ctx.Dr2 != 0 || ctx.Dr3 != 0) {
      return true;
    }
  }
  return false;
}

// Anti-VM Detection (Advanced with False Positive Protection)
inline bool IsVirtualMachine() {
  int score = 0;

  // Check for common VM files & drivers
  const std::vector<std::string> vmFiles = {
      oxorany("C:\\windows\\System32\\Drivers\\VBoxMouse.sys"),
      oxorany("C:\\windows\\System32\\Drivers\\vmmouse.sys"),
      oxorany("C:\\windows\\System32\\Drivers\\vmmemctl.sys"),
      oxorany("C:\\windows\\System32\\Drivers\\vboxguest.sys"),
      oxorany("C:\\windows\\System32\\Drivers\\vboxvideo.sys"),
      oxorany("C:\\windows\\System32\\Drivers\\vboxsf.sys"),
      oxorany("C:\\windows\\System32\\Drivers\\vmxnet.sys"),
      oxorany("C:\\windows\\System32\\Drivers\\vmcheck.sys")};

  for (const auto &file : vmFiles) {
    if (GetFileAttributesA(file.c_str()) != INVALID_FILE_ATTRIBUTES) {
      score++;
      break; // Only count file match once
    }
  }

  // Check for specific registry keys
  HKEY hKey;
  if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, oxorany("SOFTWARE\\VMware, Inc.\\VMware Tools"),
                    0, KEY_READ, &hKey) == ERROR_SUCCESS) {
    RegCloseKey(hKey);
    score++;
  }
  if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, oxorany("HARDWARE\\ACPI\\DSDT\\VBOX__"), 0,
                    KEY_READ, &hKey) == ERROR_SUCCESS) {
    RegCloseKey(hKey);
    score++;
  }

  // CPUID Hypervisor bit (Standard VM detection)
  int cpuInfo[4];
  __cpuid(cpuInfo, 1);
  if ((cpuInfo[2] >> 31) & 1) {
    score++;
  }

  // Require at least 2 indicators to reduce false positives
  return (score >= 2);
}

// Process Guard (Anti-Attach)
inline void AntiAttach() {
  HMODULE hNtdll = GetModuleHandleA(oxorany("ntdll.dll"));
  if (hNtdll) {
    auto pDbgBreakPoint = GetProcAddress(hNtdll, oxorany("DbgBreakPoint"));
    if (pDbgBreakPoint) {
      DWORD oldProtect;
      VirtualProtect(pDbgBreakPoint, 1, PAGE_EXECUTE_READWRITE, &oldProtect);
      *(unsigned char *)pDbgBreakPoint = 0xC3; // Change to RET
      VirtualProtect(pDbgBreakPoint, 1, oldProtect, &oldProtect);
    }
  }
}

// Check for known suspicious processes
inline std::wstring CheckForPatcherProcesses() {
  HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (hSnapshot == INVALID_HANDLE_VALUE)
    return L"";
 
  PROCESSENTRY32W pe32;
  pe32.dwSize = sizeof(pe32);
 
  std::vector<std::wstring> suspiciousProcesses = {
      oxorany_w(L"x64dbg.exe"),        oxorany_w(L"x32dbg.exe"),       oxorany_w(L"ollydbg.exe"),
      oxorany_w(L"ida.exe"),           oxorany_w(L"ida64.exe"),        oxorany_w(L"cheatengine.exe"),
      oxorany_w(L"processhacker.exe"), oxorany_w(L"dnspy.exe"),        oxorany_w(L"ilspy.exe"),
      oxorany_w(L"dotpeek.exe"),       oxorany_w(L"fiddler.exe"),      oxorany_w(L"wireshark.exe"),
      oxorany_w(L"httpdebugger.exe"),  oxorany_w(L"scylla.exe"),       oxorany_w(L"scylla_x64.exe"),
      oxorany_w(L"scylla_x86.exe"),    oxorany_w(L"protection_id.exe")};
 
  if (Process32FirstW(hSnapshot, &pe32)) {
    do {
      std::wstring exeName = pe32.szExeFile;
      for (const auto &suspicious : suspiciousProcesses) {
        if (exeName.find(suspicious) != std::wstring::npos) {
          CloseHandle(hSnapshot);
          return exeName;
        }
      }
    } while (Process32NextW(hSnapshot, &pe32));
  }
 
  CloseHandle(hSnapshot);
  return L"";
}

// Main Runtime Guard
inline void PerformSecurityCheck() {
  if (IsDebuggerPresent() || CheckDebugPort() || CheckDebugObject() ||
      CheckHardwareBreakpoints()) {
    trigger_protection_alert("Debugger/Hardware Breakpoint Detected (KeyAuth)", "Debugging Tool");
  }

  std::wstring badProc = CheckForPatcherProcesses();
  if (!badProc.empty()) {
    std::string n(badProc.begin(), badProc.end());
    trigger_protection_alert("Unauthorized Patcher Tool Detected", n);
  }

  if (IsVirtualMachine()) {
    trigger_protection_alert("Virtual Machine Detected", "Virtual Machine Environment");
  }
}

// Continuous monitoring thread
inline void StartProtectionMonitor() {
  AntiAttach();
  std::thread([]() {
    while (true) {
      if (IsDebuggerPresent() || CheckDebugPort() ||
          CheckHardwareBreakpoints()) {
        trigger_protection_alert("Debugger/Breakpoint Triggered at Runtime", "Debugger");
      }
      std::wstring badProc = CheckForPatcherProcesses();
      if (!badProc.empty()) {
        std::string n(badProc.begin(), badProc.end());
        trigger_protection_alert("Unauthorized Tool Opened during Session", n);
      }
      Sleep(5000); // Check every 5 seconds
    }
  }).detach();
}
} // namespace KeyAuthProtection
