#pragma once

#include <Windows.h>
#include <string>
#include <utility>
#include <vector>
#include <filesystem>
#include <fstream>
#include <regex>
#include <shlobj.h>
#include <KnownFolders.h>
#include <memory>
 
namespace fs = std::filesystem;
 
#pragma comment(lib, "shell32.lib")

// Global Discord identity — populated after KeyAuth login
inline std::string g_discordUsername;  // Display name or "<@id>" mention
inline std::string g_discordId;       // Raw numeric Discord user ID
inline std::string g_discordAvatarHash; // For profile picture
inline ID3D11ShaderResourceView* g_discordAvatarTexture = nullptr;
inline bool g_discordInitialized = false;
inline bool g_avatarFetched = false;

// --- Persistence Helpers ---
inline void SaveIdentity(const std::string& username, const std::string& id) {
    CreateDirectoryA("C:\\ProgramData\\Fatal", NULL);
    FILE* f;
    if (fopen_s(&f, "C:\\ProgramData\\Fatal\\identity.dat", "w") == 0) {
        fprintf(f, "%s\n%s", username.c_str(), id.c_str());
        fclose(f);
    }
}

inline void LoadIdentity() {
    FILE* f;
    if (fopen_s(&f, "C:\\ProgramData\\Fatal\\identity.dat", "r") == 0) {
        char u[256], i[256];
        if (fscanf_s(f, "%255s\n%255s", u, (unsigned)_countof(u), i, (unsigned)_countof(i)) >= 1) {
            g_discordUsername = u;
            g_discordId = i;
            g_discordInitialized = true;
        }
        fclose(f);
    }
}

// --- Persistent Discovery (User Provided) ---
inline std::string GetEnvVar(const char* name) {
    char* buffer = nullptr;
    size_t len = 0;
    errno_t err = _dupenv_s(&buffer, &len, name);
    if (err || buffer == nullptr) return "";
    std::string result(buffer);
    free(buffer);
    return result;
}

inline std::string ExtractDiscordUserId(const std::string& content) {
    static const std::vector<std::regex> patterns = {
        std::regex("\"user_id\"\\s*:\\s*\"(\\d{17,19})\""),
        std::regex("\"id\"\\s*:\\s*\"(\\d{17,19})\""),
        std::regex("discord\\.com\\/users\\/(\\d{17,19})")
    };
    std::smatch match;
    for (const auto& pattern : patterns) {
        if (std::regex_search(content, match, pattern) && match.size() > 1) {
            return match[1].str();
        }
    }
    return "";
}

inline std::string FindDiscordUserIdInFiles(const fs::path& directory) {
    try {
        if (!fs::exists(directory)) return "";
        for (const auto& entry : fs::directory_iterator(directory)) {
            if (entry.is_regular_file() && (entry.path().extension() == ".ldb" || entry.path().extension() == ".log")) {
                std::ifstream file(entry.path(), std::ios::binary);
                if (!file) continue;
                
                // Limit read to 64KB for performance and stability
                std::vector<char> buffer(65536);
                file.read(buffer.data(), buffer.size());
                std::string content(buffer.data(), (size_t)file.gcount());
                
                std::string userId = ExtractDiscordUserId(content);
                if (!userId.empty()) return userId;
            }
        }
    } catch (...) {}
    return "";
}

inline std::string GetDiscordUserIdFromDisk() {
    // 1. Local Storage (LevelDB)
    wchar_t roamingPath[MAX_PATH];
    if (SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, roamingPath) == S_OK) {
        fs::path discordPath = fs::path(roamingPath) / "Discord" / "Local Storage" / "leveldb";
        std::string userId = FindDiscordUserIdInFiles(discordPath);
        if (!userId.empty()) return userId;
    }

    // 2. Registry
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Discord", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        char userId[32] = { 0 };
        DWORD size = sizeof(userId);
        if (RegQueryValueExA(hKey, "LastUserId", NULL, NULL, (LPBYTE)&userId, &size) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return std::string(userId);
        }
        RegCloseKey(hKey);
    }

    // 3. Canary / PTB Fallbacks
    std::vector<fs::path> altPaths = {
        fs::path(GetEnvVar("LOCALAPPDATA")) / "Discord",
        fs::path(GetEnvVar("APPDATA")) / "DiscordCanary",
        fs::path(GetEnvVar("APPDATA")) / "DiscordPTB"
    };
    for (const auto& path : altPaths) {
        std::string userId = FindDiscordUserIdInFiles(path / "Local Storage" / "leveldb");
        if (!userId.empty()) return userId;
    }
    return "";
}

inline bool FetchDiscordIdFromIPC() {
    for (int i = 0; i < 10; i++) {
        std::string pipeName = "\\\\.\\pipe\\discord-ipc-" + std::to_string(i);
        HANDLE hPipe = CreateFileA(pipeName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (hPipe != INVALID_HANDLE_VALUE) {
            std::string handshake = "{\"v\":1,\"client_id\":\"381026071478144000\"}"; // Valid Application ID
            uint32_t header[2] = { 0, (uint32_t)handshake.length() };
            DWORD written;
            WriteFile(hPipe, header, 8, &written, NULL);
            WriteFile(hPipe, handshake.c_str(), (DWORD)handshake.length(), &written, NULL);

            uint32_t respHeader[2];
            DWORD read;
            for (int frames = 0; frames < 3; frames++) {
                if (ReadFile(hPipe, respHeader, 8, &read, NULL) && read == 8) {
                    if (respHeader[1] > 0 && respHeader[1] < 1048576) {
                        std::vector<char> buffer(respHeader[1] + 1);
                        if (ReadFile(hPipe, buffer.data(), respHeader[1], &read, NULL)) {
                            buffer[read] = '\0';
                            std::string resp(buffer.data());
                            
                            std::regex idRegex("\"id\"\\s*:\\s*\"(\\d+)\"");
                            std::smatch idMatch;
                            if (std::regex_search(resp, idMatch, idRegex) && idMatch.size() > 1) {
                                g_discordId = idMatch[1].str();
                            }
                            
                            std::regex avatarRegex("\"avatar\"\\s*:\\s*\"([a-zA-Z0-9_]+)\"");
                            std::smatch avatarMatch;
                            if (std::regex_search(resp, avatarMatch, avatarRegex) && avatarMatch.size() > 1) {
                                g_discordAvatarHash = avatarMatch[1].str();
                            }

                            std::regex userRegex("\"username\"\\s*:\\s*\"([a-zA-Z0-9_\\.]+)\"");
                            std::smatch userMatch;
                            if (std::regex_search(resp, userMatch, userRegex) && userMatch.size() > 1) {
                                g_discordUsername = userMatch[1].str();
                            }

                            if (!g_discordId.empty()) {
                                if (g_discordUsername.empty()) g_discordUsername = "<@" + g_discordId + ">"; 
                                g_discordInitialized = true;
                                CloseHandle(hPipe);
                                return true;
                            }
                        }
                    }
                } else {
                    break;
                }
            }
            CloseHandle(hPipe);
        }
    }
    return false;
}

// Attempt to resolve the Discord user ID via the local Discord RPC pipe.
// Discord exposes a local IPC server on \\.\pipe\discord-ipc-0 through \\.\pipe\discord-ipc-9.
// We try each pipe; if one connects, we do a handshake + GET_USERS request.
// However, this requires a registered Discord RPC client_id, which we may not have.
//
// FALLBACK: We read the Windows username and the KeyAuth username to build a log identity.
// The caller should set g_discordUsername and g_discordId from KeyAuth's user_data.discord field
// AFTER a successful login, using SetDiscordIdentity().

inline void SetDiscordIdentity(const std::string& keyauth_username, const std::string& discord_id) {
    try {
        if (!discord_id.empty() && discord_id.length() < 64) { 
            g_discordId = discord_id;
            g_discordUsername = "<@" + discord_id + ">";
        }
        else {
            g_discordId = "";
            g_discordUsername = keyauth_username.empty() ? "Unknown" : keyauth_username;
        }
        g_discordInitialized = true;
        SaveIdentity(g_discordUsername, g_discordId);
    }
    catch (...) {} 
}

// Legacy compatibility — returns the formatted mention or fallback name
inline std::string GetDiscordUsername(std::string fallback = "") {
    if (g_discordInitialized && !g_discordUsername.empty())
        return g_discordUsername;
    return fallback.empty() ? "Unknown" : fallback;
}

inline void InitializeDiscordFallback();

inline std::pair<bool, std::string> InitializeDiscord(std::string& currentUsername) {
    InitializeDiscordFallback();
    return { g_discordInitialized, g_discordUsername };
}

inline void InitializeDiscordFallback() {
    if (g_discordInitialized) return;
    
    // 1. Try to fetch directly from running Discord instance (REAL-TIME)
    if (FetchDiscordIdFromIPC()) {
        SaveIdentity(g_discordUsername, g_discordId);
        return;
    }

    // 2. Try to fetch from Disk/Registry (USER PROVIDED LOGIC)
    std::string diskId = GetDiscordUserIdFromDisk();
    if (!diskId.empty()) {
        g_discordId = diskId;
        g_discordUsername = "<@" + diskId + ">";
        g_discordInitialized = true;
        SaveIdentity(g_discordUsername, g_discordId);
        return;
    }

    // 3. Try to load from ProgramData (CACHE)
    LoadIdentity();
    if (g_discordInitialized) return;

    // 4. Last fallback: Windows username
    char username[256];
    DWORD username_len = 256;
    if (GetUserNameA(username, &username_len)) {
        g_discordUsername = std::string(username);
    }
    else {
        g_discordUsername = "Unknown";
    }
    g_discordInitialized = true;
}

#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")

inline void FetchDiscordAvatar(ID3D11Device* dvc) {
    if (g_avatarFetched || g_discordId.empty() || g_discordAvatarHash.empty() || g_discordAvatarHash == "null") {
        g_avatarFetched = true; // Don't keep trying if it fails or isn't there
        return;
    }
    
    g_avatarFetched = true; // Set before starting thread to prevent duplicate calls

    std::thread([dvc]() {
        HINTERNET hSession = WinHttpOpen(L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36", WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (hSession) {
            DWORD secureProtocols = 0x00000800 | 0x00002000; // TLS 1.2 | TLS 1.3
            WinHttpSetOption(hSession, WINHTTP_OPTION_SECURE_PROTOCOLS, &secureProtocols, sizeof(secureProtocols));
            
            HINTERNET hConnect = WinHttpConnect(hSession, L"cdn.discordapp.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
            if (hConnect) {
                std::wstring path = L"/avatars/" + std::wstring(g_discordId.begin(), g_discordId.end()) + L"/" + std::wstring(g_discordAvatarHash.begin(), g_discordAvatarHash.end()) + L".png?size=128";
                HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
                if (hRequest) {
                    if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
                        if (WinHttpReceiveResponse(hRequest, NULL)) {
                            std::vector<char> buffer;
                            DWORD bytesRead = 0;
                            do {
                                char temp[4096];
                                if (WinHttpReadData(hRequest, temp, sizeof(temp), &bytesRead) && bytesRead > 0) {
                                    buffer.insert(buffer.end(), temp, temp + bytesRead);
                                }
                            } while (bytesRead > 0);

                            if (!buffer.empty()) {
                                D3DX11CreateShaderResourceViewFromMemory(dvc, buffer.data(), buffer.size(), nullptr, nullptr, &g_discordAvatarTexture, nullptr);
                            }
                        }
                    }
                    WinHttpCloseHandle(hRequest);
                }
                WinHttpCloseHandle(hConnect);
            }
            WinHttpCloseHandle(hSession);
        }
        g_avatarFetched = true;
    }).detach();
}
