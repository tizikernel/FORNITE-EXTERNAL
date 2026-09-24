#pragma once

#include <Windows.h>
#include <string>
#include <thread>
#include <vector>
#include <filesystem>

namespace ui { void AddLog(const std::string& msg); }

// Execute a command completely hidden (no window flash)
static std::string executeCleanerCommand(const std::string& command) {
    std::string result;
    char buffer[128];
    SECURITY_ATTRIBUTES sa;
    HANDLE hRead, hWrite;

    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    if (!CreatePipe(&hRead, &hWrite, &sa, 0))
        return "";

    STARTUPINFOA si = { sizeof(STARTUPINFOA) };
    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = hWrite;
    si.hStdError = hWrite;

    PROCESS_INFORMATION pi;
    if (!CreateProcessA(NULL, const_cast<char*>(command.c_str()), NULL, NULL,
        TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        CloseHandle(hRead);
        CloseHandle(hWrite);
        return "";
    }

    CloseHandle(hWrite);

    DWORD bytesRead;
    while (ReadFile(hRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead != 0) {
        buffer[bytesRead] = '\0';
        result += buffer;
    }

    // Wait for process to finish (max 60 seconds per bat)
    WaitForSingleObject(pi.hProcess, 60000);

    CloseHandle(hRead);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return result;
}

// Find the cleaner directory (relative to exe)
static std::filesystem::path FindCleanerDir() {
    char modulePath[MAX_PATH] = {};
    GetModuleFileNameA(nullptr, modulePath, MAX_PATH);
    const std::filesystem::path exeDir = std::filesystem::path(modulePath).parent_path();

    // Search in multiple candidate locations
    const std::vector<std::filesystem::path> candidates = {
        exeDir / "cleaner",
        exeDir.parent_path() / "cleaner",
        exeDir.parent_path().parent_path() / "cleaner"
    };

    for (const auto& dir : candidates) {
        if (std::filesystem::exists(dir) && std::filesystem::is_directory(dir))
            return dir;
    }
    return {};
}

#include "cleaner_data.h"

// Run all .bat files from embedded byte arrays silently
inline void launch_cleaner() {
    ui::AddLog("[+] Startet Cleaning Process");
    ui::AddLog("[+] Cleaning Ban Traces...");

    char tempPath[MAX_PATH];
    GetTempPathA(MAX_PATH, tempPath);

    for (int i = 0; i < cleaner_data::bat_count; i++) {
        std::string fileName = std::string(tempPath) + "\\cleanup_" + std::to_string(i) + ".bat";
        
        // Write byte array to temp file
        std::ofstream outFile(fileName, std::ios::binary);
        if (outFile.is_open()) {
            outFile.write(reinterpret_cast<const char*>(cleaner_data::all_bats[i].data), cleaner_data::all_bats[i].size);
            outFile.close();

            // Execute silently
            std::string cmd = "cmd.exe /c call \"" + fileName + "\"";
            executeCleanerCommand(cmd);

            // Cleanup temp file
            DeleteFileA(fileName.c_str());
        }
    }

    ui::AddLog("[GRE]Finished Cleaning Process!");
}
