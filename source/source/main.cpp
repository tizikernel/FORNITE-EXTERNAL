#include "../protection/security_macros.h"
#include "overlay.h"
#include "driver.h"
#include "slavedriver.h"
#include "../mapper/includes/mapper_bridge.hpp"
#include <signal.h>
#include "../protection/protection.h"
#include "../protection/security.hpp"
#include "../protection/Backend/Backend.h"
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
#include <thread>

void LaunchMapperSilently(const std::string& exePath, const std::string& sysPath) {
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi = { 0 };

    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;

    HANDLE hNull = CreateFileA("NUL", GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    si.hStdOutput = hNull;
    si.hStdError = hNull;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

    std::string cmd = exePath + " " + sysPath;

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

int loadcheat() {
    system("cls");
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

    const wchar_t* target_process = L"FortniteClient-Win64-Shipping.exe";

    while (Kernel->m_process_id == 0) {
        try {
            if (!Kernel->attach(target_process)) {
                Sleep(800);
            }
            else {
                break;
            }
        }
        catch (...) {
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

    SetConsoleColor(10);
    std::cout << "\n\n\n  [+] ";
    SetConsoleColor(7);
    std::cout << "Press [ENTER] in Lobby!";

    while (!(GetAsyncKeyState(VK_RETURN) & 0x8000)) {
        Sleep(10);
    }

    PROTECT_VIRTUAL_END;
    overlay::start();

    return 0;
}

void RandomTitleThread() {
    srand((unsigned)time(NULL));
    const char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!@#$%^&*()";

    while (true) {
        std::string title;
        for (int i = 0; i < 20; ++i) {
            title += alphanum[rand() % (sizeof(alphanum) - 1)];
        }
        SetConsoleTitleA(title.c_str());
        Sleep(150);
    }
}

std::string GetConfigPath() {
    char* appdata = nullptr;
    size_t len = 0;
    _dupenv_s(&appdata, &len, "APPDATA");
    std::string path = "loader_auth.txt";
    if (appdata != nullptr) {
        path = std::string(appdata) + "\\UnknownTeamFN\\key.txt";
        free(appdata);
        CreateDirectoryA((std::string(getenv("APPDATA") ? getenv("APPDATA") : ".") + "\\UnknownTeamFN").c_str(), nullptr);
    }
    return path;
}

bool LoadSavedKey(std::string& licenseKey) {
    std::ifstream file(GetConfigPath());
    if (file.is_open()) {
        std::getline(file, licenseKey);
        file.close();
        return !licenseKey.empty();
    }
    return false;
}

void SaveKey(const std::string& licenseKey) {
    char* appdata = nullptr;
    size_t len = 0;
    _dupenv_s(&appdata, &len, "APPDATA");
    if (appdata) {
        std::string dir = std::string(appdata) + "\\UnknownTeamFN";
        CreateDirectoryA(dir.c_str(), nullptr);
        free(appdata);
    }
    std::ofstream file(GetConfigPath());
    if (file.is_open()) {
        file << licenseKey;
        file.close();
    }
}

int main(int, char**)
{
    SPOOF_FUNC;
    g_SecurityAlertHook = [](const std::string& msg) {
        trigger_protection_alert(msg, ("Zenith"));
        };

    Security::Initialize();
    std::thread(protection_start).detach();

    PROTECT_VIRTUAL_START;
    SetConsoleOutputCP(CP_UTF8);
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

        SetConsoleColor(10);
        std::cout << ("\n\n\n  [+] ");
        SetConsoleColor(7);
        std::cout << ("Initializing, Doing Auth Checks....\n");

        std::string licenseKey;
        bool autoLoginSuccess = false;

        if (LoadSavedKey(licenseKey)) {
            SetConsoleColor(10);
            std::cout << "  [+] ";
            SetConsoleColor(7);
            std::cout << "Saved key found! Validating...\n";

            if (Backend::ValidateKeyOnline(licenseKey)) {
                autoLoginSuccess = true;
            }
            else {
                SetConsoleColor(4);
                std::cout << "  [!] Auto-login failed: " << Backend::LastError() << "\n";
                SetConsoleColor(7);
                licenseKey.clear();
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
            std::cout << "Verifying License with Server...";

            if (!Backend::ValidateKeyOnline(licenseKey)) {
                SetConsoleColor(4);
                std::cout << "\n  [!] " << Backend::LastError() << "\n";
                SetConsoleColor(7);
                Sleep(3000);
                exit(1);
            }
        }

        SetConsoleColor(10);
        std::cout << "\n  [+] ";
        SetConsoleColor(7);
        std::cout << "License Verified Successfully!\n";
        SaveKey(licenseKey);
        Sleep(1000);

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
