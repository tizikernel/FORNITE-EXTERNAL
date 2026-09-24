#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <thread>
#include <iostream>
#include "security_macros.h"

// 1. Bitwise Rotations
static inline uint32_t rotl32(uint32_t x, unsigned int n) {
    return (x << n) | (x >> (32 - n));
}

static inline char rotl8(char value, unsigned int count) {
    return (value << count) | (value >> (8 - count));
}

// 2. Character Utilities
static inline constexpr char convert_accent(char c) {
    switch (static_cast<unsigned char>(c)) {
    case 0xE0: case 0xE1: case 0xE2: case 0xE3: case 0xE4: case 0xE5: return 'a';
    case 0xE8: case 0xE9: case 0xEA: case 0xEB: return 'e';
    case 0xEC: case 0xED: case 0xEE: case 0xEF: return 'i';
    case 0xF2: case 0xF3: case 0xF4: case 0xF5: case 0xF6: return 'o';
    case 0xF9: case 0xFA: case 0xFB: case 0xFC: return 'u';
    case 0xE7: return 'c';
    case 0xF1: return 'n';
    case 0xFD: case 0xFF: return 'y';
    default: return c;
    }
}

// 3. Security Alerting (Local Fallback)
static inline void security_alert_local(const std::string& message) {
    // 1. Start a 6-second auto-kill timer
    std::thread([]() {
        Sleep(6000);
        ExitProcess(1);
    }).detach();
    
    // 2. Show MessageBox
    std::string msg = "Fatal Security Alert!\nViolation: " + message + "\n\nApplication will terminate automatically.";
    MessageBoxA(nullptr, msg.c_str(), "Fatal - Security Violation", MB_OK | MB_ICONERROR | MB_TOPMOST | MB_SETFOREGROUND);
    
    // 3. Terminate process
    ExitProcess(1);
}

#ifndef SECURITY_UTILS_NO_ERROR_WRAPPER
static inline void error(const std::string& message) {
    security_alert_local(message);
}

static inline void error(const char* message) {
    security_alert_local(std::string(message));
}

template<size_t _size, char _key1, char _key2, typename T>
static inline void error(std::string& message) {
    security_alert_local(message);
}
#endif
