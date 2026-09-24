#pragma once
#include <string>
#include <windows.h>

// Global dynamic pointer to route all alerts back to the master handler (protection.h)
// This safely bypasses all circular header inclusion loops.
extern void (*g_SecurityAlertHook)(const std::string& reason);

inline void security_error(const std::string& msg) {
    if (g_SecurityAlertHook) {
        g_SecurityAlertHook(msg);
    } else {
        // Fallback if triggered before WinMain initializes the hook
        MessageBoxA(NULL, msg.c_str(), oxorany("Fatal - Security Violation"), MB_OK | MB_ICONERROR | MB_TOPMOST | MB_SETFOREGROUND);
        ExitProcess(0);
    }
}
