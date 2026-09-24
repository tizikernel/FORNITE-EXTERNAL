#pragma once
#include <windows.h>
#include <winternl.h>
#include <string>
#include <thread>
#include <vector>
#include <chrono>

// Core Macros & SDKs (Base Layer)
#include "security_macros.h"
#include "pe_defs.h"

// 2. Advanced Security Headers
#include "advanced/lazy.h"
#define LAZY_IMPORTER_HARDENED_MODULE_CHECKS
#include "advanced/anti_crack.hpp"
#include "advanced/anti_inject.h"
#include "AntiTamper/AntiTamperLib.h"

// 3. Linker Dependencies
#pragma comment(lib, "obsidium64.lib")
/*
#pragma comment(lib, "VMProtectSDK64.lib")
#pragma comment(lib, "VirtualizerSDK64.lib")
#pragma comment(lib, "SecureEngineSDK64.lib")
*/


#include "advanced_protection.hpp"

namespace Security {
    inline void Initialize() {
        PROTECT_VIRTUAL_START;
        
        // 1. Initialize Memory Integrity Watchdog
        InitializeAntiTamper();
        
        // 2. Load Core Protection Logic
        anti_inject();
        AntiCrack::ProtectionController::InitializeProtection();
        
        // 3. Immediate Debugger Check
        if (AntiCrack::AntiDebug::IsDebuggerPresent()) {
            AntiCrack::ProtectionController::TriggerProtection();
            exit(0);
        }
        
        PROTECT_VIRTUAL_END;
    }
}
