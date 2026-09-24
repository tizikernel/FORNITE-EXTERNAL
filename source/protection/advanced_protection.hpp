#pragma once
#include <Windows.h>
#include <thread>
#include "advanced/Fatal Security.hpp"
#include "advanced/protect.h"
#include "advanced/anti_dump.h"
#include "advanced/anti_attach.h"
#include "advanced/killprocess.h"
#include "vmp/VMProtectSDK.h"
#include "oxorany.h"
#include "security_error.hpp" // Restored, we need this for the global hook routing!

namespace Fatal {
    namespace AdvancedProtection {
        
        inline void Initialize() {
            // 1. Initial High-Speed Checks
            Security::PatchRemoteBreak();
            Security::HideThread(GetCurrentThread());
            
            // 2. Clear PE Header to complicate dumping early
            // Security::ErasePEHeader(); 
            
            // 3. Start Anti-Attach Logic (hooks DbgBreakPoint etc)
            // std::thread(AntiAttach).detach();
        }

         inline void RunCycle() {
            PROTECT_VIRTUAL_START;
            // 1. Core Security Checks (SECURITY BREACH NIGGAAA)
            if (!Security::PerformChecks()) {
                security_error("Heuristic Security Violation - BRANDNAME HERE");
            }

            // 2. Anti-Dump PEB Check
            anti_dump();

            // 3. Process Surveillance (Detecting blacklisted tools)
            // find_exe_title(); // Integrated via its own thread for persistence
            PROTECT_VIRTUAL_END;
        }

        // --- Multi-Layer String Obfuscation ---
        // Combines oxorany with VMProtect
        // Note: nesting them requires careful literal handling. 
        // We use them in a way that both protectors see the call site.
        #define AX_STR(str) VMProtectDecryptStringA(oxorany(str))
        
        // Simplified version that specifically uses VMP for the literal
        #define VMP_XOR(str) VMProtectDecryptStringA(oxorany(str))
    }
}
