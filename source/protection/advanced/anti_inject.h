#pragma once
#include <Windows.h>
#include "../security_macros.h"


inline void anti_inject() {
    // Prevents loading DLLs that are not signed by Microsoft.
    // This blocks almost all common game cheats/injectors and untrusted DLLs.
    HMODULE hKernel32 = GetModuleHandleA(oxorany("kernel32.dll"));
    if (hKernel32) {
        typedef BOOL(WINAPI* PSET_PROCESS_MITIGATION_POLICY)(PROCESS_MITIGATION_POLICY, PVOID, SIZE_T);
        PSET_PROCESS_MITIGATION_POLICY pSetProcessMitigationPolicy =
            (PSET_PROCESS_MITIGATION_POLICY)GetProcAddress(hKernel32, oxorany("SetProcessMitigationPolicy"));

        if (pSetProcessMitigationPolicy) {
            // Block common hook pathways like AppInit_DLLs (SAFE)
            PROCESS_MITIGATION_EXTENSION_POINT_DISABLE_POLICY ep = {};
            ep.DisableExtensionPoints = 1;
            pSetProcessMitigationPolicy(ProcessExtensionPointDisablePolicy, &ep, sizeof(ep));

            // DISABLED: MicrosoftSignedOnly (TOO RESTRICTIVE for unsigned cheat loaders)
            /*
            PROCESS_MITIGATION_BINARY_SIGNATURE_POLICY bsp = {};
            bsp.MicrosoftSignedOnly = 1; 
            pSetProcessMitigationPolicy(ProcessSignaturePolicy, &bsp, sizeof(bsp));
            */

            // DISABLED: Restrictive Image Load Policy (TOO RESTRICTIVE for local third-party libs)
            /*
            PROCESS_MITIGATION_IMAGE_LOAD_POLICY ilp = {};
            ilp.NoRemoteImages = 1;
            ilp.NoLowMandatoryLabelImages = 1;
            ilp.PreferSystem32Images = 1;
            pSetProcessMitigationPolicy(ProcessImageLoadPolicy, &ilp, sizeof(ilp));
            */
        }
    }
}
