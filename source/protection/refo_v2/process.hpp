#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <Windows.h>
#include <tchar.h>
#include <TlHelp32.h>
#include "../refo_v2/misc/lazy.h"

// Internal helper for narrow-to-wide conversion to support encrypted narrows from _xor_
inline std::wstring ax_to_wide(const std::string& str) {
    if (str.empty()) return L"";
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

inline std::uintptr_t process_find(const char* name)
{
	if (!name) return 0;
	std::wstring wname = ax_to_wide(name);

	const auto snap = LI_FN(CreateToolhelp32Snapshot).safe()(TH32CS_SNAPPROCESS, 0);
	if (snap == INVALID_HANDLE_VALUE) {
		return 0;
	}

	PROCESSENTRY32W proc_entry{};
	proc_entry.dwSize = sizeof(proc_entry);

	auto found_process = false;
	if (!!LI_FN(Process32FirstW).safe()(snap, &proc_entry)) {
		do {
			if (_wcsicmp(wname.c_str(), proc_entry.szExeFile) == 0) {
				found_process = true;
				break;
			}
		} while (!!LI_FN(Process32NextW).safe()(snap, &proc_entry));
	}

	LI_FN(CloseHandle).safe()(snap);
	return found_process
		? (std::uintptr_t)proc_entry.th32ProcessID
		: 0;
}