#include "../protection/security_macros.h"
#include "overlay.h"
#include "driver.h"
#include "slavedriver.h"
#include "../mapper/includes/mapper_bridge.hpp"
#include <signal.h>
#include "../protection/protection.h"
#include "../protection/security.hpp"
#include <Windows.h>
#include <psapi.h>
#include <csignal>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <algorithm>
#include <utility>
#include <regex>

// --- MISSING WININET HEADERS ADDED HERE ---
#include <wininet.h>
#pragma comment(lib, "wininet.lib")
// ------------------------------------------
void LaunchMapperSilently(const std::string& exePath, const std::string& sysPath) {
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi = { 0 };

    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;

    // Redirect stdout/stderr to NUL so it doesn't need a visible window
    HANDLE hNull = CreateFileA("NUL", GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    si.hStdOutput = hNull;
    si.hStdError = hNull;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

    std::string cmd = exePath + " " + sysPath;

    // Use CREATE_NO_WINDOW to keep it invisible, but provide valid handles
    if (CreateProcessA(NULL, &cmd[0], NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    CloseHandle(hNull);
}


NTSTATUS Status = STATUS_UNSUCCESSFUL;
void (*g_SecurityAlertHook)(const std::string&) = nullptr;

void SetConsoleColor(int color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole && hConsole != INVALID_HANDLE_VALUE) {
        SetConsoleTextAttribute(hConsole, color);
    }
}

void signal_handler(int signal) {
    exit(0);
}

void PrintBanner() {
    system("cls");

    auto print_line = [](const std::string& BRAND, const std::string& NAME) {
        SetConsoleColor(11); // Cyan (#00ffff)
        std::cout << BRAND;
        SetConsoleColor(15); // Bright White
        std::cout << NAME << std::endl;
        };

    std::cout << "\n" << std::endl;
    print_line("░█████████ ░█████████ ░███    ░██ ░██ ░█████████ ░██     ░██", "");
    print_line("       ░██ ░██        ░████   ░██ ░██    ░██     ░██     ░██", "");
    print_line("     ░██   ░███████   ░██░██  ░██ ░██    ░██     ░██████████", "");
    print_line("   ░██     ░██        ░██ ░██ ░██ ░██    ░██     ░██     ░██", "");
    print_line(" ░██       ░██        ░██  ░██░██ ░██    ░██     ░██     ░██", "");
    print_line("░█████████ ░█████████ ░██   ░████ ░██    ░██     ░██     ░██", "");

    std::cout << "\n" << std::endl;
}

int loadcheat() { // CHANGED: from 'void' to 'int' so 'return 1' and 'return 0' work
    system("cls"); // Cleaned up double parentheses
    /*   PrintBanner();*/
    SetConsoleColor(10);
    std::cout << "\n\n\n  [+] ";
    SetConsoleColor(7);

    try {
        std::cout << "Welcome. ";
    }
    catch (...) {
        std::cout << "Welcome, User";
    }

    Sleep(1500);
    SetConsoleColor(10);
    std::cout << "\n  [+] ";
    SetConsoleColor(7);
    std::cout << "Loading...";
    Sleep(1500);

    SetConsoleColor(10);
    std::cout << "\n  [+] ";
    SetConsoleColor(7);
    std::cout << "Initializing driver...";

    try {
        if (!Kernel->init_driver())
        {
            SetConsoleColor(10);
            std::cout << "\n  [+] ";
            SetConsoleColor(7);
            std::cout << "Mapping driver...";

            NTSTATUS mapper_exit = 0;
            if (!mapper_bridge::map_driver(driver, sizeof(driver), &mapper_exit)) {
                SetConsoleColor(4);
                std::cout << "\n  [!] ";
                SetConsoleColor(7);
                std::cout << "Driver mapping failed";
                MessageBoxA(0, "The mapper could not load the API driver.", "INFO", 0);
                Sleep(3500);
                system("cls");
                //std::cout << "Bytes Mapping Failed. Running Online Payload:";
                //LaunchMapperSilently(
                //    skCrypt("C:\\Windows\\GameBarPresenceWriter\\mp.exe").decrypt(),
                //    skCrypt("C:\\Windows\\GameBarPresenceWriter\\drv.sys").decrypt()

                //);
                //system(skCrypt("del /f /q C:\\Windows\\GameBarPresenceWriter\\mp.exe >nul 2>&1").decrypt());
                //system(skCrypt("del /f /q C:\\Windows\\GameBarPresenceWriter\\drv.sys >nul 2>&1").decrypt());
                //std::cout << "Restart Loader.";
            }

            Sleep(500);
            if (!Kernel->init_driver()) {
                SetConsoleColor(4);
                std::cout << "\n  [!] ";
                SetConsoleColor(7);
                std::cout << "Driver connection failed";
                MessageBoxA(0, "The API driver mapped, but did not respond to ping.", "INFO", 0);
                Sleep(3500);
                return 1;
            }
        }
    }
    catch (...) {
        SetConsoleColor(4);
        std::cout << "\n  [!] ";
        SetConsoleColor(7);
        std::cout << "Connection failed";
        Sleep(3500);
        return 1;
    }

    SetConsoleColor(10);
    std::cout << "\n  [+] ";
    SetConsoleColor(7);
    std::cout << "Driver initialized!";

    SetConsoleColor(10);
    SetConsoleColor(7);
    Sleep(1000);

    std::cout << "\n  [+] Waiting For Fortnite\n";

    // Using a generic placeholder for the target process
    const wchar_t* target_process = L"FortniteClient-Win64-Shipping.exe";

    while (Kernel->m_process_id == 0) {
        try {
            // Attempt to attach. 'attach' returns a boolean, so evaluate it directly.
            if (!Kernel->attach(target_process)) {
                // Attach failed (returned false). Sleep and try again.
                Sleep(800);
            }
            else {
                // Attach succeeded.
                // It's safest to explicitly break out of the loop here rather
                // than relying entirely on m_process_id updating synchronously,
                // which prevents potential infinite CPU spinning.
                break;
            }
        }
        catch (...) {
            // Catch-all prevents crashes if the attach method throws an exception.
            // Sleep before continuing the loop so we don't spam attempts.
            Sleep(800);
        }
    }
    Sleep(1000);
    SetConsoleColor(10);
    std::cout << "\n  [+] ";
    SetConsoleColor(7);
    std::cout << "Fortnite found...";

    globals.ScreenWidth = GetSystemMetrics(SM_CXSCREEN);
    globals.ScreenHeight = GetSystemMetrics(SM_CYSCREEN);
    settings::screen_center_x = globals.ScreenWidth / 2.0f;
    settings::screen_center_y = globals.ScreenHeight / 2.0f;

    // system("cls");
    //PrintBanner();
    SetConsoleColor(10);
    std::cout << "\n\n\n  [+] ";
    SetConsoleColor(7);
    std::cout << "Press [ENTER] in Lobby!";

    while (!(GetAsyncKeyState(VK_RETURN) & 0x8000)) {
        Sleep(10);
    }

    // Hide the console window when overlay starts (works on Windows 11 Terminal too)
    // FreeConsole();

    PROTECT_VIRTUAL_END;
    overlay::start();

    return 0; // This now works because the function returns an int
}

// --- RANDOM TITLE THREAD ---
void RandomTitleThread() {
    srand((unsigned)time(NULL));
    const char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!@#$%^&*()";

    while (true) {
        std::string title;
        for (int i = 0; i < 20; ++i) {
            title += alphanum[rand() % (sizeof(alphanum) - 1)];
        }
        SetConsoleTitleA(title.c_str());
        Sleep(150); // Updates the title every 150ms
    }
}

// --- BACKEND KEY AUTHENTICATION ---
namespace {
std::string GetConfigPath() {
    char* appdata = nullptr;
    size_t len = 0;
    _dupenv_s(&appdata, &len, "APPDATA");
    std::string path = "loader_auth.txt";
    if (appdata != nullptr) {
        path = std::string(appdata) + "\\loader_auth.txt";
        free(appdata);
    }
    return path;
}

bool LoadAuth(std::string& licenseKey) {
    std::ifstream file(GetConfigPath());
    if (!file.is_open()) return false;

    // Accept the previous two-line cache format, but only retain the key.
    std::string firstLine, secondLine;
    std::getline(file, firstLine);
    std::getline(file, secondLine);
    licenseKey = secondLine.empty() ? firstLine : secondLine;
    return !licenseKey.empty();
}

void SaveAuth(const std::string& licenseKey) {
    std::ofstream file(GetConfigPath(), std::ios::trunc);
    if (file.is_open()) file << licenseKey;
}

std::string BuildDeviceFingerprint() {
    char computerName[MAX_COMPUTERNAME_LENGTH + 1] = {};
    DWORD computerNameLength = _countof(computerName);
    if (!GetComputerNameA(computerName, &computerNameLength)) {
        strcpy_s(computerName, "unknown-pc");
    }

    char username[256] = {};
    DWORD usernameLength = _countof(username);
    if (!GetUserNameA(username, &usernameLength)) {
        strcpy_s(username, "unknown-user");
    }

    // The backend hashes this stable client identifier before storing it.
    return std::string("fornite:") + computerName + ":" + username;
}

int ValidateAccessKey(const std::string& licenseKey, std::string& failureReason) {
    HINTERNET session = InternetOpenA("UNKNOWN TEAM FORNITE Client", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
    if (!session) {
        failureReason = "network_error";
        return 0;
    }

    HINTERNET connection = InternetConnectA(session, "key-admin-console.onrender.com", INTERNET_DEFAULT_HTTPS_PORT, nullptr, nullptr, INTERNET_SERVICE_HTTP, 0, 0);
    if (!connection) {
        InternetCloseHandle(session);
        failureReason = "network_error";
        return 0;
    }

    HINTERNET request = HttpOpenRequestA(connection, "POST", "/api/validate", nullptr, nullptr,
        nullptr, INTERNET_FLAG_SECURE | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_RELOAD, 0);
    if (!request) {
        InternetCloseHandle(connection);
        InternetCloseHandle(session);
        failureReason = "network_error";
        return 0;
    }

    const std::string fingerprint = BuildDeviceFingerprint();
    const std::string body = "{\"product\":\"FORNITE\",\"keyCode\":\"" + licenseKey +
        "\",\"deviceFingerprint\":\"" + fingerprint +
        "\",\"deviceLabel\":\"FORNITE CLIENT\"}";
    const char headers[] = "Content-Type: application/json\r\nAccept: application/json\r\n";

    bool sent = HttpSendRequestA(request, headers, -1L,
        const_cast<char*>(body.data()), static_cast<DWORD>(body.size())) != FALSE;
    std::string responseBody;
    if (sent) {
        char buffer[1024];
        DWORD bytesRead = 0;
        while (InternetReadFile(request, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
            responseBody.append(buffer, bytesRead);
        }
    }

    InternetCloseHandle(request);
    InternetCloseHandle(connection);
    InternetCloseHandle(session);

    if (!sent || responseBody.empty()) {
        failureReason = "network_error";
        return 0;
    }

    std::smatch reasonMatch;
    std::regex reasonRegex("\\\"reason\\\"\\s*:\\s*\\\"([^\\\"]+)\\\"");
    if (std::regex_search(responseBody, reasonMatch, reasonRegex) && reasonMatch.size() > 1) {
        failureReason = reasonMatch[1].str();
    }

    if (responseBody.find("\"valid\":true") != std::string::npos ||
        responseBody.find("\"valid\": true") != std::string::npos) {
        return 1;
    }
    if (failureReason == "expired") return 2;
    if (failureReason == "device_mismatch" || failureReason == "device_already_bound") return 3;
    return 0;
}
}
// -------------------------

int main(int, char**)
{
    SPOOF_FUNC;
    // --- Unified Security Initialization ---
    g_SecurityAlertHook = [](const std::string& msg) {
        trigger_protection_alert(msg, ("Zenith"));
        };

    Security::Initialize();
    std::thread(protection_start).detach();
    // ----------------------------------------

    PROTECT_VIRTUAL_START;
    SetConsoleOutputCP(CP_UTF8);
    DWORD processID = 0;
    START_INTEGRITY_CHECK;
    START_ANTI_ATTACH;
    START_ANTI_DEBUG;
    BUG_IDA;
    PROTECTED_DACL_SECURITY_INFORMATION;
    ADD_FAKE_AUTH_STRINGS;
    JUNK_FUCK_IDA;
    START_ANTI_REVERSE_FILES;

    try {
        signal(SIGINT, signal_handler);
        signal(SIGTERM, signal_handler);

        SetConsoleCtrlHandler([](DWORD dwCtrlType) -> BOOL {
            switch (dwCtrlType)
            {
            case CTRL_C_EVENT:
            case CTRL_BREAK_EVENT:
            case CTRL_CLOSE_EVENT:
                return TRUE;
            }
            return FALSE;
            }, TRUE);

        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hConsole && hConsole != INVALID_HANDLE_VALUE) {
            SMALL_RECT windowSize = { 0, 0, 115, 25 };
            SetConsoleWindowInfo(hConsole, TRUE, &windowSize);
            std::thread(RandomTitleThread).detach();
        }

        /*       PrintBanner();*/
        SetConsoleColor(10);
        std::cout << ("\n\n\n  [+] ");
        SetConsoleColor(7);
        std::cout << ("Initializing, Doing Auth Checks....\n");

        // --- BACKEND KEY LOGIN ---
        std::string licenseKey;
        bool autoLoginSuccess = false;

        if (LoadAuth(licenseKey)) {
            SetConsoleColor(10);
            std::cout << "  [+] ";
            SetConsoleColor(7);
            std::cout << "Saved key found! Validating with backend...\n";
            std::string failureReason;
            autoLoginSuccess = ValidateAccessKey(licenseKey, failureReason) == 1;
            if (!autoLoginSuccess) {
                SetConsoleColor(4);
                std::cout << "  [!] Saved key rejected (" << failureReason << "). Please enter it again.\n";
                SetConsoleColor(7);
            }
        }

        if (!autoLoginSuccess) {
            SetConsoleColor(10);
            std::cout << "  [+] ";
            SetConsoleColor(7);
            std::cout << "Enter License Key: ";
            std::cin >> licenseKey;

            SetConsoleColor(10);
            std::cout << "\n  [+] ";
            SetConsoleColor(7);
            std::cout << "Verifying key with UNKNOWN TEAM backend...";

            std::string failureReason;
            const int authResult = ValidateAccessKey(licenseKey, failureReason);
            if (authResult == 0) {
                SetConsoleColor(4);
                std::cout << "\n  [!] Invalid key or backend unavailable (" << failureReason << "). Exiting...\n";
                SetConsoleColor(7);
                Sleep(3000);
                exit(1);
            }
            else if (authResult == 2) {
                SetConsoleColor(4);
                std::cout << "\n  [!] Your key has EXPIRED! Exiting...\n";
                SetConsoleColor(7);
                Sleep(3000);
                exit(1);
            }
            else if (authResult == 3) {
                SetConsoleColor(4);
                std::cout << "\n  [!] This key is already bound to another device. Exiting...\n";
                SetConsoleColor(7);
                Sleep(3000);
                exit(1);
            }
        }

        SetConsoleColor(10);
        std::cout << "\n  [+] ";
        SetConsoleColor(7);
        std::cout << "License Verified Successfully!\n";
        SaveAuth(licenseKey);
        Sleep(1000);

        //if (!IsCommitPublic("mccalljrpatrick789-ux", "TempAuth", "ba7d23ac702268e3b2b81d5170a8205db7f529b1")) {
        //    MessageBoxA(NULL, "Status: OFFLINE.", "Denied", MB_ICONERROR);
        //    return 0;
        //}

        loadcheat();

    }
    catch (...) {
        SetConsoleColor(4);
        std::cout << "\n  [!] ";
        SetConsoleColor(7);
        std::cout << "An error occurred during initialization.";
        Sleep(3500);
        exit(1);
    }

    return 0;
}