#pragma once

#include <Windows.h>
#include <thread>
#include "security_macros.h"
#include <filesystem>
#include <winhttp.h>
#include <winternl.h>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <aclapi.h>

#ifndef NTAPI
#define NTAPI __stdcall
#endif

#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
#endif

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "wintrust.lib")
#pragma comment(lib, "crypt32.lib")

namespace fs = std::filesystem;

// --- Forward Declarations ---
#include "security_error.hpp"
inline void cracking(std::string reason, std::string offending_app);
extern void (*g_SecurityAlertHook)(const std::string&);

// --- External Logic & Thread Functions ---
inline bool check_user_banned(const std::string& username) { return false; } // Basic fallback
inline void show_ban() { MessageBoxA(NULL, "Your account is temporarily restricted.", "Fatal", MB_OK | MB_ICONERROR); }
inline void hwid_mismatch() { /* Handle HWID mismatch notification/logging */ }
inline void failed_to_login() { /* Handle login failure logging */ }
inline void logged_in() { /* Handle successful login logging */ }
inline void handle_logged_in() { /* Logic for post-login setup */ }
inline void spoofed_log() { /* Log successful spoofing */ }
inline void spoofed_reg() { /* Registry spoofing logic */ }

// Already defined in serials.h, usb.h, or main.cpp:
// void fetchinfo();
// void handle_usb(); 
// void launch_cleaner();
// void protection_start();

extern bool sign_and_start();
extern bool sign_and_start1();

// --- Dependencies ---
#include "security.hpp"
#include "oxorany.h"
#include "discord.h"
#include "log.h"
// custom_protect.h and KeyAuthProtection.hpp moved below trigger_protection_alert

// --- Diagnostic Logging ---
inline void WriteDiagnosticLog(std::string msg) {
    // Disabled for production stealth — no local traces
    /*
    FILE* f;
    if (fopen_s(&f, "Security.log", "a") == 0) {
        fprintf(f, "[%llu] %s\n", GetTickCount64(), msg.c_str());
        fclose(f);
    }
    */
}

// Minimal JSON string escape helper
inline std::string ax_escape_json(const std::string& s) {
    std::ostringstream o;
    for (auto c = s.cbegin(); c != s.cend(); c++) {
        switch (*c) {
        case '"': o << "\\\""; break;
        case '\\': o << "\\\\"; break;
        case '\b': o << "\\b"; break;
        case '\f': o << "\\f"; break;
        case '\n': o << "\\n"; break;
        case '\r': o << "\\r"; break;
        case '\t': o << "\\t"; break;
        default:
            if ('\x00' <= *c && *c <= '\x1f') {
                o << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (int)*c;
            } else {
                o << *c;
            }
        }
    }
    return o.str();
}

// Authorized singleton for all protection modules
inline void trigger_protection_alert(const std::string& reason, const std::string& app_name) {
    WriteDiagnosticLog("--- SECURITY ALERT TRIAGED ---");
    WriteDiagnosticLog("Reason: " + reason + " | App: " + app_name);

    // 1. Run tracking & webhooks in background (detached)
    std::thread([reason, app_name]() {
        cracking(reason, app_name); 
    }).detach();

    // 2. Start a 6-second auto-kill timer to close the program
    std::thread([]() {
        Sleep(6000);
        SAFE_CALL(ExitProcess).get()(0xDEAD);
    }).detach();

    // 3. Show MessageBox synchronously blocking the process.
    PROTECT_MUTATE_START;
    SAFE_CALL(EnableWindow).get()(GetForegroundWindow(), FALSE);
    std::string msg = "Fatal Security Alert!\nViolation: " + reason + "\nTarget: " + app_name + "\n\nSecurity logs and evidence have been transmitted.\nApplication will terminate automatically.";
    SAFE_CALL(MessageBoxA).get()(nullptr, (LPCSTR)msg.c_str(), (LPCSTR)"Fatal - Security Violation", MB_OK | MB_ICONERROR | MB_TOPMOST | MB_SETFOREGROUND | MB_TASKMODAL);
    PROTECT_MUTATE_END;

    // 4. Terminate process if "OK" is clicked before 6 seconds
    SAFE_CALL(ExitProcess).get()(0xDEAD);
}

// --- Sub-modules that depend on trigger_protection_alert ---
#include "custom_protect.h"
#include "KeyAuthProtection.hpp"

inline char input[64] = { "" };
inline bool spoofed = false;
typedef int (WINAPI* MessageBoxTimeoutA_t)(HWND, LPCSTR, LPCSTR, UINT, WORD, DWORD);

inline void cracking(std::string reason, std::string offending_app) {
    PROTECT_VIRTUAL_START;
    PROTECT_MUTATE_START;
    WriteDiagnosticLog("Capturing screenshot...");
    std::string screenshot = save_screenshot();
    WriteDiagnosticLog(screenshot.empty() ? "No screenshot captured." : "Screenshot saved to: " + screenshot);

    char processPath[MAX_PATH];
    SAFE_CALL(GetModuleFileNameA).get()(nullptr, processPath, MAX_PATH);
    std::string procName = fs::path(processPath).filename().string();

    // --- Discord Identity & Content ---
    std::string discordUserValue;
    if (!g_discordId.empty()) {
        discordUserValue = g_discordId + " & <@" + g_discordId + ">";
    } else {
        discordUserValue = "N/A & " + (g_discordUsername.empty() ? "Unknown User" : g_discordUsername);
    }

    // --- License Key Formatting (ANSI Red) ---
    std::string licenseValue;
    if (strlen(input) == 0) {
        licenseValue = "```ansi\n\u001b[31mN/A\u001b[0m\n```";
    } else {
        licenseValue = "```ansi\n\u001b[31m" + std::string(input) + "\u001b[0m\n```";
    }

    // --- Dynamic Discord Timestamp ---
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    std::string timeStr = "<t:" + std::to_string(timestamp) + ":F>";

    std::stringstream json;
    json << ("{");
    json << ("\"content\": \"Fatal Security Violation Detected!\",");
    json << ("\"embeds\": [{");
    json << ("\"title\": \"Fatal - Security Logs\",");
    json << ("\"color\": 11901441,");
    json << ("\"fields\": [");
    
    json << ("{ \"name\": \"<:discord1:1479321995727470673> Discord User \", \"value\": \"") << ax_escape_json(discordUserValue) << ("\", \"inline\": false },");
    json << ("{ \"name\": \"<:pcsettings:1480065790043750575> Baseboard Information\", \"value\": \"```ansi\\n\\u001b[33mManufacturer:\\u001b[0m ") << ax_escape_json(find_manufacturer()) << ("\\n\\u001b[33mProduct:\\u001b[0m ") << ax_escape_json(find_product()) << ("\\n```\", \"inline\": false },");
    json << ("{ \"name\": \"<:key1:1479997094545592451> License Key\", \"value\": \"") << ax_escape_json(licenseValue) << ("\", \"inline\": false },");
    json << ("{ \"name\": \"<:alert12:1479300317580496896> Network \", \"value\": \"```\\nPublic IP: ") << ax_escape_json(find_local_ip()) << ("\\nEstimated Location: ") << ax_escape_json(get_location()) << ("\\n```\", \"inline\": false },");
    json << ("{ \"name\": \"<:development:1479322028694569023> Offending Application\", \"value\": \"```ansi\\n\\u001b[31mProcess Name:\\u001b[0m ") << ax_escape_json(offending_app) << ("\\n\\u001b[31mViolation:\\u001b[0m ") << ax_escape_json(reason) << ("\\n```\", \"inline\": false },"); 
    json << ("{ \"name\": \"<:clockk_1:1480096299960373258> Date & Time\", \"value\": \"") << ax_escape_json(timeStr) << ("\", \"inline\": false }");

    json << ("],");
    json << ("\"footer\": { \"text\": \"Server Check - Fatal\", \"icon_url\": \"https://cdn.discordapp.com/attachments/1280958884424450202/1490153633457766440/axiom_logo.png?ex=69d7a277&is=69d650f7&hm=6b1d0d1cdcc15b322d74ed3b7b149fd041c04e305c02d59efd97db17b6c76cbc&\" },");
    if (!screenshot.empty()) json << ("\"image\": { \"url\": \"attachment://screenshot.png\" }");
    json << ("}]");
    json << ("}");
    std::string jsonStr = json.str();
    const std::string boundary = oxorany("------------------------7df3c1a2e1a9f6b0");
    const std::string crlf = "\r\n";
    std::vector<char> body;
    auto append_str = [&](const std::string& s) { body.insert(body.end(), s.begin(), s.end()); };

    append_str(("--") + boundary + crlf);
    append_str(("Content-Disposition: form-data; name=\"payload_json\"") + crlf + crlf);
    append_str(jsonStr + crlf);

    if (!screenshot.empty()) {
        HANDLE hFile = SAFE_CALL(CreateFileA).get()(screenshot.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile != INVALID_HANDLE_VALUE) {
            LARGE_INTEGER liSize{};
            if (SAFE_CALL(GetFileSizeEx).get()(hFile, &liSize) && liSize.QuadPart > 0) {
                std::vector<char> fileBuffer((size_t)liSize.QuadPart);
                DWORD bytesRead = 0;
                SAFE_CALL(ReadFile).get()(hFile, fileBuffer.data(), (DWORD)fileBuffer.size(), &bytesRead, nullptr);
                append_str(("--") + boundary + crlf);
                append_str(("Content-Disposition: form-data; name=\"file\"; filename=\"screenshot.png\"") + crlf);
                append_str(("Content-Type: image/png") + crlf + crlf);
                body.insert(body.end(), fileBuffer.begin(), fileBuffer.end());
                append_str(crlf);
            }
            SAFE_CALL(CloseHandle).get()(hFile);
        }
    }
    append_str(("--") + boundary + ("--") + crlf);

    HINTERNET hSession = WinHttpOpen(oxorany_w(L"Mozilla/5.0"), WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) { WriteDiagnosticLog("WinHttpOpen failed."); return; }
 
    HINTERNET hConnect = WinHttpConnect(hSession, oxorany_w(L"discord.com"), INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) { WriteDiagnosticLog("WinHttpConnect failed."); WinHttpCloseHandle(hSession); return; }
 
    std::wstring wPath = skCrypt(L"/api/webhooks/1494800903969046739/11XLFdFffaRI4fR1oQUSDJQWNonrvzxkb7guwDctwgxuD-x2XiZkfAcDjuQJnosutQSj").decrypt();
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, oxorany_w(L"POST"), wPath.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    
    if (hRequest) {
        std::wstring wHeaders = oxorany_w(L"Content-Type: multipart/form-data; boundary=") + std::wstring(boundary.begin(), boundary.end());
        
        WriteDiagnosticLog("Transmitting Discord Webhook (" + std::to_string(body.size()) + " bytes)...");
        
        // 1. Send Request with total length specified
        if (WinHttpSendRequest(hRequest, wHeaders.c_str(), (DWORD)-1L, WINHTTP_NO_REQUEST_DATA, 0, (DWORD)body.size(), 0)) {
            
            // 2. Write body data in one go (or chunks)
            DWORD bytesWritten = 0;
            if (WinHttpWriteData(hRequest, body.data(), (DWORD)body.size(), &bytesWritten)) {
                
                // 3. Finalize and check response
                if (WinHttpReceiveResponse(hRequest, NULL)) {
                    DWORD statusCode = 0;
                    DWORD dwSize = sizeof(statusCode);
                    WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &dwSize, WINHTTP_NO_HEADER_INDEX);
                    WriteDiagnosticLog("Transmission Complete. Discord HTTP Status: " + std::to_string(statusCode));
                } else {
                    WriteDiagnosticLog("WinHttpReceiveResponse FAILED. Error: " + std::to_string(GetLastError()));
                }
            } else {
                WriteDiagnosticLog("WinHttpWriteData FAILED. Error: " + std::to_string(GetLastError()));
            }
        } else {
            WriteDiagnosticLog("WinHttpSendRequest FAILED. Error: " + std::to_string(GetLastError()));
        }
        WinHttpCloseHandle(hRequest);
    } else {
        WriteDiagnosticLog("WinHttpOpenRequest FAILED. Error: " + std::to_string(GetLastError()));
    }

    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    if (!screenshot.empty()) SAFE_CALL(DeleteFileA)(screenshot.c_str());
    PROTECT_MUTATE_END;
    PROTECT_VIRTUAL_END;
}

// [unused utility functions removed]


// [unused notification functions removed]

// [show_ban removed]

inline void close_explorer() {

    while (true) {
        HWND hwnd = FindWindowA(oxorany("CabinetWClass"), NULL);
        while (hwnd) {
            PostMessage(hwnd, WM_CLOSE, 0, 0);
            hwnd = FindWindowA(oxorany("CabinetWClass"), NULL);
        }

        std::this_thread::sleep_for(std::chrono::microseconds(50));
    }
}


inline std::string get_executable_name_from_hwnd(HWND hwnd) {
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == 0) return "";

    HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (!hProc) return "";

    char exePath[MAX_PATH] = { 0 };
    DWORD sz = MAX_PATH;
    std::string exeName;

    if (QueryFullProcessImageNameA(hProc, 0, exePath, &sz)) {
        char* exeNamePtr = strrchr(exePath, '\\');
        if (exeNamePtr) {
            exeName = exeNamePtr + 1;
        }
        else {
            exeName = exePath;
        }
    }

    CloseHandle(hProc);
    return exeName;
}

inline BOOL CALLBACK hwnd_lookup(HWND hwnd, LPARAM lParam) {
    std::vector<std::string> searchPatterns = {
        oxorany("hacker"),
        oxorany("dexz"),
        oxorany("RuntimePatcher.exe"),
        oxorany("HxD"),
        oxorany("HxD.exe"),
        oxorany("diwnessx"),
        oxorany("myu"),
        oxorany("ProcessHacker"),
        oxorany("WindowsForms10.Window.8.app.0.378734a"),
        oxorany("BrocessRacker"),
        oxorany("Apophenia"),
        oxorany("TFormMain"),
        oxorany("Disastra"),
        oxorany("RuntimePatcher.exe (5212): unnamed thread (10644)"),
        oxorany("The BinFbs"),
        oxorany("Qt5QWindowIcon"),
        oxorany("Qt5QWindowIcon myu [diwnessx]"),
        oxorany("Qt682QTQWindowIcon"),
        oxorany("The BinF"),
        oxorany("The BinFb"),
        oxorany("BinFbs")
        // Removed extremely loose patterns like "The", "Bin", "5212", "10644"
    };

    char wndTitle[256] = { 0 };
    char wndClassName[256] = { 0 };

    if (!GetWindowTextA(hwnd, wndTitle, sizeof(wndTitle)) ||
        !GetClassNameA(hwnd, wndClassName, sizeof(wndClassName))) {
        return TRUE;
    }

    std::string windowTitle = wndTitle;
    std::string windowClassName = wndClassName;

    for (const auto& pattern : searchPatterns) {
        if (!pattern.empty()) {
            if (windowTitle.find(pattern) != std::string::npos ||
                windowClassName.find(pattern) != std::string::npos) {
                std::string exeName = get_executable_name_from_hwnd(hwnd);
                trigger_protection_alert("Blacklisted Window Detected: " + windowTitle, exeName);
                return TRUE;
            }
        }
    }

    if (windowClassName.find(oxorany("Qt5QWindowIcon")) != std::string::npos ||
        windowClassName.find(oxorany("Qt5QWindowIcon myu [diwnessx]")) != std::string::npos ||
        windowClassName.find(oxorany("Qt682QTQWindowIcon")) != std::string::npos) {

        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);
        if (pid == 0) return TRUE;

        HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if (!hProc) return TRUE;

        wchar_t buf[MAX_PATH] = { 0 };
        DWORD sz = _countof(buf);
        std::wstring exePath;

        if (QueryFullProcessImageNameW(hProc, 0, buf, &sz)) {
            exePath.assign(buf, sz);
        }
        CloseHandle(hProc);

        if (!exePath.empty()) {
            std::wstring processDir = exePath.substr(0, exePath.find_last_of(L'\\'));

            std::vector<std::wstring> debuggerFiles = {
                oxorany_w(L"x64dbg.dll"),
                oxorany_w(L"x32dbg.dll"),
                oxorany_w(L"x96dbg.dll"),
                oxorany_w(L"ida.dll"),
                oxorany_w(L"ida64.dll"),
            };

            for (const auto& debuggerFile : debuggerFiles) {
                std::wstring debuggerPath = processDir + L"\\" + debuggerFile;
                DWORD fileAttr = GetFileAttributesW(debuggerPath.c_str());
                if (fileAttr != INVALID_FILE_ATTRIBUTES && !(fileAttr & FILE_ATTRIBUTE_DIRECTORY)) {
                    std::string exeName = get_executable_name_from_hwnd(hwnd);
                    trigger_protection_alert("Debugger DLL Loaded: " + windowClassName, exeName);
                    return TRUE;
                }
            }
        }
    }

    return TRUE;
}

inline void hide_thread() {
    typedef NTSTATUS(NTAPI* pNtSetInformationThread)(HANDLE, UINT, PVOID, ULONG);
    HMODULE hNtdll = GetModuleHandleA(oxorany("ntdll.dll"));

    if (hNtdll) {
        pNtSetInformationThread NtSetInformationThread =
            (pNtSetInformationThread)GetProcAddress(hNtdll, oxorany("NtSetInformationThread"));

        if (NtSetInformationThread) {
            NtSetInformationThread(GetCurrentThread(), 0x11, NULL, 0);
        }
    }
}

inline void handle_process_driver() {

    std::vector<const wchar_t*> Events =
    {
        oxorany_w(L"Global\\ProcessHacker"),
        oxorany_w(L"Global\\SystemInformer"),
    };

    for (const auto& Handle : Events)
    {
        HANDLE eventHandle = OpenEventW(EVENT_ALL_ACCESS, FALSE, Handle);
        DWORD dwError = GetLastError();
        if (dwError == ERROR_INVALID_HANDLE || dwError == ERROR_SUCCESS)
        {
            trigger_protection_alert("Process Hacker Engine Detected", "System Informer / Process Hacker");
        }
    }
}

__forceinline void unpage_pe()
{
    SYSTEM_INFO systemInfo;
    GetSystemInfo(&systemInfo);
    size_t page_size = static_cast<size_t>(systemInfo.dwPageSize);
    void* memory = VirtualAlloc(nullptr, page_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    DWORD oldProtect;
    VirtualProtect(memory, page_size, PAGE_NOACCESS, &oldProtect);
}

__forceinline void patch_certificate()
{
    HANDLE handle = GetCurrentProcess();
    DWORD aclSize = sizeof(ACL) + sizeof(ACCESS_DENIED_ACE) + GetSidLengthRequired(1);
    PACL pDacl = (PACL)new BYTE[aclSize];
    ZeroMemory(pDacl, aclSize);
    InitializeAcl(pDacl, aclSize, ACL_REVISION);
    PSID pSpecificSid = NULL;
    SID_IDENTIFIER_AUTHORITY SIDAuth = SECURITY_NT_AUTHORITY;
    AllocateAndInitializeSid(&SIDAuth, 1, SECURITY_LOCAL_SYSTEM_RID, 0, 0, 0, 0, 0, 0, 0, &pSpecificSid);
    AddAccessDeniedAce(pDacl, ACL_REVISION, DELETE | READ_CONTROL | WRITE_DAC | WRITE_OWNER | SYNCHRONIZE, pSpecificSid);
    FreeSid(pSpecificSid);
    SECURITY_DESCRIPTOR sd;
    InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorDacl(&sd, TRUE, pDacl, FALSE);
    PSID integritySid = NULL;
    SID_IDENTIFIER_AUTHORITY integrityAuthority = SECURITY_MANDATORY_LABEL_AUTHORITY;
    AllocateAndInitializeSid(&integrityAuthority, 1, SECURITY_MANDATORY_MEDIUM_RID, 0, 0, 0, 0, 0, 0, 0, &integritySid);
    DWORD saclSize = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(integritySid);
    PACL pSacl = (PACL)new BYTE[saclSize];
    ZeroMemory(pSacl, saclSize);
    InitializeAcl(pSacl, saclSize, ACL_REVISION);
    AddAccessAllowedAce(pSacl, ACL_REVISION, PROCESS_ALL_ACCESS, integritySid);
    SetSecurityDescriptorSacl(&sd, TRUE, pSacl, FALSE);
    SetKernelObjectSecurity(handle, DACL_SECURITY_INFORMATION | LABEL_SECURITY_INFORMATION, &sd);
    delete[] pDacl;
    delete[] pSacl;
    FreeSid(integritySid);
    DebugActiveProcessStop((GetCurrentProcessId)());
    if (fs::exists(oxorany("certificate.crt")) || fs::exists(oxorany("bypassed.exe")))
    {
        trigger_protection_alert("Patch Certificate/Bypass File Detected", "File System Integrity");
    }
}

inline bool close_invalid_handle()
{
    __try { CloseHandle((HANDLE)0x99999999ULL); }
    __except (EXCEPTION_EXECUTE_HANDLER) { return TRUE; }
    return FALSE;
}

inline bool raise_dbg_control()
{
    __try { RaiseException(DBG_CONTROL_C, 0, 0, NULL); return true; }
    __except (GetExceptionCode() == DBG_CONTROL_C ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) { return false; }
}

inline auto no_stepping()
{
    // [DISABLED] - Highly prone to false positives due to compiler padding (0xCC) 
    // and environmental factors in standard Release/Debug builds.
    return;
}






static const GUID WINTRUST_ACTION_GENERIC_VERIFY_V2 =
{ 0x00aac56b, 0xcd44, 0x11d0, { 0x8c, 0xc2, 0x00, 0xc0, 0x4f, 0xc2, 0x95, 0xee } };

inline BOOL CALLBACK Specter_Module(HWND hwnd, LPARAM lParam) {
    std::vector<std::string> windowTitles = {
        oxorany("hacker"),
        oxorany("dexz"),
        oxorany("RuntimePatcher.exe"),
        oxorany("unnamed thread"),
        oxorany("HxD"),
        oxorany("HxD.exe"),
        oxorany("diwnessx"),
        oxorany("myu"),
    };

    std::vector<std::string> windowClassNames = {
        oxorany("ProcessHacker"),
        oxorany("WindowsForms10.Window.8.app.0.378734a"),
        oxorany("BrocessRacker"),
        oxorany("Apophenia"),
        oxorany("TFormMain"),
        oxorany("diwnessx"),
        oxorany("myu"),
        oxorany("Disastra"),
    };

    char wndTitle[256] = { 0 };
    char wndClassName[256] = { 0 };

    if (!(GetWindowTextA)(hwnd, wndTitle, sizeof(wndTitle)) ||
        !(GetClassNameA)(hwnd, wndClassName, sizeof(wndClassName))) {
        return TRUE;
    }

    std::string windowTitle = wndTitle;
    std::string windowClassName = wndClassName;


    if (windowClassName.find(oxorany("ConsoleWindowClass")) != std::string::npos) {
        if (windowTitle.find(oxorany("RuntimePatcher.exe (5212): unnamed thread (10644)")) != std::string::npos ||
            windowTitle.find(oxorany("The BinFbs")) != std::string::npos ||
            windowTitle.find(oxorany("Disastra")) != std::string::npos) {
            trigger_protection_alert("Suspicious Window Title Pattern: " + windowTitle, "Blacklisted Tool");
            SendMessageA(hwnd, WM_CLOSE, 0, 0);
            return TRUE;
        }

        if (windowTitle.find(oxorany("RuntimePatcher")) != std::string::npos ||
            windowTitle.find(oxorany("Qt5QWindowIcon")) != std::string::npos) {

            std::vector<std::string> conservativeMatches = {
                oxorany("The BinFbs"),
                oxorany("BinFbs")
            };

            for (const auto& partial : conservativeMatches) {
                if (windowTitle.find(partial) != std::string::npos) {
                    trigger_protection_alert("Blacklisted Task Title Pattern: " + windowTitle, "Blacklisted Tool");

                    SendMessageA(hwnd, WM_CLOSE, 0, 0);
                    return TRUE;
                }
            }
        }
        return TRUE;
    }

    for (const auto& title : windowTitles) {
        if (!title.empty() && windowTitle.find(title) != std::string::npos) {
            trigger_protection_alert("Blacklisted Tool Title: " + windowTitle, "Blacklisted Application");
            SendMessageA(hwnd, WM_CLOSE, 0, 0);
            return TRUE;
        }
    }

    for (const auto& className : windowClassNames) {
        if (!className.empty() && windowClassName.find(className) != std::string::npos) {
            trigger_protection_alert("Blacklisted Tool Class: " + windowClassName, "Blacklisted Application");
            SendMessageA(hwnd, WM_CLOSE, 0, 0);
            return TRUE;
        }
    }

    if (windowClassName.find(oxorany("Apophenia")) != std::string::npos) {
        trigger_protection_alert("Apophenia Debugger Class Detected", "Apophenia");
        SendMessageA(hwnd, WM_CLOSE, 0, 0);
        return TRUE;
    }

    if (windowClassName.find(oxorany("Qt5QWindowIcon")) != std::string::npos ||
        windowClassName.find(oxorany("Qt5QWindowIcon myu [diwnessx]")) != std::string::npos ||
        windowClassName.find(oxorany("Qt682QTQWindowIcon")) != std::string::npos) {

        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);
        if (pid == 0) return TRUE;

        std::wstring exePath;
        HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if (!hProc) return TRUE;

        wchar_t buf[MAX_PATH] = { 0 };
        DWORD sz = _countof(buf);
        if (QueryFullProcessImageNameW(hProc, 0, buf, &sz)) {
            exePath.assign(buf, sz);
        }

        CloseHandle(hProc);

        if (!exePath.empty()) {
            std::wstring processDir = exePath.substr(0, exePath.find_last_of(L'\\'));

            std::vector<std::wstring> debuggerFiles = {
                oxorany_w(L"x64dbg.dll"),
                oxorany_w(L"x32dbg.dll"),
                oxorany_w(L"x96dbg.dll"),
                oxorany_w(L"ida.dll"),
                oxorany_w(L"ida64.dll"),
            };

            for (const auto& debuggerFile : debuggerFiles) {
                std::wstring debuggerPath = processDir + L"\\" + debuggerFile;
                DWORD fileAttr = GetFileAttributesW(debuggerPath.c_str());
                if (fileAttr != INVALID_FILE_ATTRIBUTES && !(fileAttr & FILE_ATTRIBUTE_DIRECTORY)) {
                    trigger_protection_alert("Debugger Runtime DLL in Search Path: " + windowTitle, "Blacklisted Tool");
                    SendMessageA(hwnd, WM_CLOSE, 0, 0);
                    return TRUE;
                }
            }
        }
    }

    return TRUE;
}



inline bool HardwareBreakpoints() {
    CONTEXT ctx = { 0 };
    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    HANDLE hThread = GetCurrentThread();
    if (GetThreadContext(hThread, &ctx)) {
        if (ctx.Dr0 != 0 || ctx.Dr1 != 0 || ctx.Dr2 != 0 || ctx.Dr3 != 0) {
            return true;
        }
    }
    return false;
}

inline bool ax_pre_launch_check() {
    // Basic PEB check
    if (IsDebuggerPresent()) return true;

    // Remote debugger check
    BOOL isDebuggerPresent = FALSE;
    if (CheckRemoteDebuggerPresent(GetCurrentProcess(), &isDebuggerPresent) && isDebuggerPresent) return true;

    // Hardware breakpoints
    if (HardwareBreakpoints()) return true;

    // Fast check for common debuggers using window names
    std::vector<const char*> fastPatterns = {
        "x64dbg", "x32dbg", "ida64", "ida", "Process Hacker", "Cheat Engine", "Scylla"
    };

    for (const auto& pattern : fastPatterns) {
        if (FindWindowA(NULL, pattern)) return true;
    }

    return false;
}

inline void Anti_Debug() {
    // Check basic PEB flag
    if (IsDebuggerPresent()) {
        trigger_protection_alert("Debugger Attached (PEB Check)", "Debugger");
    }

    // Check Remote Debugger
    BOOL isDebuggerPresent = FALSE;
    if (CheckRemoteDebuggerPresent(GetCurrentProcess(), &isDebuggerPresent) && isDebuggerPresent) {
        trigger_protection_alert("Remote Debugger Attached (RPC Check)", "Remote Debugger");
    }

    if (HardwareBreakpoints()) {
        trigger_protection_alert("Hardware Breakpoints Set (KeyAuth Logic)", "Debugger");
    }
}

inline void protection_start() {
    PROTECT_MUTATE_START;
    
    // Init Discord Identity for Webhooks (Try IPC pipe first, fallback to disk scanning)
    if (!g_discordInitialized) {
        if (!FetchDiscordIdFromIPC()) {
            std::string d_id = GetDiscordUserIdFromDisk();
            SetDiscordIdentity("Unknown", d_id);
        }
    }

    SAFE_CALL(SetThreadPriority).get()(SAFE_CALL(GetCurrentThread).get()(), THREAD_PRIORITY_TIME_CRITICAL);
    SAFE_CALL(SetThreadExecutionState).get()(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_AWAYMODE_REQUIRED);
    hide_thread();

    // Init and Start Advanced Protection (Anti-Attach, PE-Header, etc)
    Fatal::AdvancedProtection::Initialize();
    
    // Initial debug check
    Anti_Debug();
    
    // patch_certificate();
    // unpage_pe();


    while (true) {
        // Multi-layered heartbeat checks
        no_stepping();
        DebugActiveProcessStop((GetCurrentProcessId)());
        no_stepping();

        if (close_invalid_handle()) { trigger_protection_alert("Invalid Handle Exception Hook", "Debugger Tool"); }
        if (raise_dbg_control()) { trigger_protection_alert("Debug Control Character Hook", "Debugger Tool"); }

        EnumWindows(Specter_Module, 0);
        handle_process_driver();

        // New Advanced Protection Cycle (Debug + Anti-Dump)
        Fatal::AdvancedProtection::RunCycle();

        // Advanced custom protection cycles
        ax_run_protection_cycle();
        ax_run_timing_checks();

        // KeyAuth specific checks
        if (KeyAuthProtection::CheckDebugPort()) trigger_protection_alert("KeyAuth: Debug Port Open", "Kernel Debugger");
        if (KeyAuthProtection::CheckDebugObject()) trigger_protection_alert("KeyAuth: Debug Object Present", "Debugger");
        if (KeyAuthProtection::CheckHardwareBreakpoints()) trigger_protection_alert("KeyAuth: Hardware Breakpoints Found", "Debugger/Patcher");

        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    PROTECT_MUTATE_END;
}
