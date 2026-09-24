#pragma once
#include <Windows.h>


#define DbgBreakPoint_FUNC_SIZE 0x2
#define DbgUiRemoteBreakin_FUNC_SIZE 0x54
#define NtContinue_FUNC_SIZE 0x18

struct ANTI_ATTACH_FUNC {
	const char* name;
	FARPROC addr;
	SIZE_T size;
};

static ANTI_ATTACH_FUNC antiAttachFuncs[] = {
	{ "DbgBreakPoint", 0, DbgBreakPoint_FUNC_SIZE },
	{ "DbgUiRemoteBreakin", 0, DbgUiRemoteBreakin_FUNC_SIZE },
	{ "NtContinue", 0, NtContinue_FUNC_SIZE }
};


inline int AntiAttach() {
	while (true)
	{
		DWORD pid = GetCurrentProcessId();

		WCHAR modName[MAX_PATH] = { 0 };
		HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, 0, pid);

		HMODULE hMod = LoadLibraryA(_xor_("ntdll.dll"));
		for (int i = 0; i < _countof(antiAttachFuncs); ++i) {
			antiAttachFuncs[i].addr = GetProcAddress(hMod, antiAttachFuncs[i].name);
		}

		bool result = false;
		auto base_address = GetModuleHandleA(0);
		if (wcsstr((WCHAR*)base_address, L"ntdll") || wcsstr((WCHAR*)base_address, L"NTDLL")) {
			for (int i = 0; i < _countof(antiAttachFuncs); ++i) {
				DWORD dwOldProtect;

				VirtualProtectEx(hProcess, antiAttachFuncs[i].addr, antiAttachFuncs[i].size, PAGE_EXECUTE_READWRITE, &dwOldProtect);
				result = WriteProcessMemory(hProcess, antiAttachFuncs[i].addr, antiAttachFuncs[i].addr, antiAttachFuncs[i].size, NULL);
				VirtualProtectEx(hProcess, antiAttachFuncs[i].addr, antiAttachFuncs[i].size, dwOldProtect, NULL);

				if (!result) break;
			}
		}

		CloseHandle(hProcess);

		return 0;
	}
}