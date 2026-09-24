#pragma once
#include <Windows.h>
#include <winternl.h>
#include <string>
#include <thread>
#include "../refo_v2/encryption/xor.h"
#include "../refo_v2/process.hpp"

extern "C" {
    NTSTATUS NTAPI RtlAdjustPrivilege(ULONG, BOOLEAN, BOOLEAN, PBOOLEAN);
    NTSTATUS NTAPI NtRaiseHardError(NTSTATUS, ULONG, ULONG, PULONG_PTR, ULONG, PULONG);
}

inline void kill_process()
{
	system(_xor_("taskkill /f /im HTTPDebuggerUI.exe >nul 2>&1"));
	system(_xor_("taskkill /f /im HTTPDebuggerSvc.exe >nul 2>&1"));
	system(_xor_("sc stop HTTPDebuggerPro >nul 2>&1"));
	system(_xor_("taskkill /FI \"IMAGENAME eq cheatengine*\" /IM * /F /T >nul 2>&1"));
	system(_xor_("taskkill /FI \"IMAGENAME eq httpdebugger*\" /IM * /F /T >nul 2>&1"));
	system(_xor_("taskkill /FI \"IMAGENAME eq processhacker*\" /IM * /F /T >nul 2>&1"));
}

inline void blue_screen()
{
	BOOLEAN bluescr;
	ULONG cevap;
	RtlAdjustPrivilege(19, TRUE, FALSE, &bluescr);
	NtRaiseHardError(STATUS_ASSERTION_FAILURE, 0, 0, NULL, 6, &cevap);
}


inline void find_exe_title()
{
	while (true) {
		if (process_find(_xor_("KsDumperClient.exe")))
		{
			blue_screen();
		}
		else if (process_find(_xor_("HTTPDebuggerUI.exe")))
		{
			blue_screen();
		}
		else if (process_find(_xor_("HTTPDebuggerSvc.exe")))
		{
			blue_screen();
		}
		else if (process_find(_xor_("FolderChangesView.exe")))
		{
			blue_screen();
		}
		else if (process_find(_xor_("ProcessHacker.exe")))
		{
			blue_screen();
		}
		else if (process_find(_xor_("procmon.exe")))
		{
			blue_screen();
		}
		else if (process_find(_xor_("idaq.exe")))
		{
			blue_screen();
		}
		else if (process_find(_xor_("idaq64.exe")))
		{
			blue_screen();
		}
		else if (process_find(_xor_("Wireshark.exe")))
		{
			blue_screen();
		}
		else if (process_find(_xor_("Fiddler.exe")))
		{
			blue_screen();
		}
		else if (process_find(_xor_("Xenos64.exe")))
		{
			blue_screen();
		}
		else if (process_find(_xor_("Cheat Engine.exe")))
		{
			blue_screen();
		}
		else if (process_find(_xor_("HTTP Debugger Windows Service (32 bit).exe")))
		{
			blue_screen();
		}
		else if (process_find(_xor_("KsDumper.exe")))
		{
			blue_screen();
		}
		else if (process_find(_xor_("x64dbg.exe")))
		{
			blue_screen();
		}
		else if (process_find(_xor_("ProcessHacker.exe")))
		{
			blue_screen();
		}
		else if (FindWindowA(0, _xor_("IDA: Quick start")))
		{
			blue_screen();
		}

		else if (FindWindowA(0, _xor_("Memory Viewer")))
		{
			blue_screen();
		}
		else if (FindWindowA(0, _xor_("Process List")))
		{
			blue_screen();
		}
		else if (FindWindowA(0, _xor_("KsDumper")))
		{
			blue_screen();
		}
		else if (FindWindowA(0, _xor_("HTTP Debugger")))
		{
			blue_screen();
		}
		else if (FindWindowA(0, _xor_("OllyDbg")))
		{
			blue_screen();
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(5000));

	}
}