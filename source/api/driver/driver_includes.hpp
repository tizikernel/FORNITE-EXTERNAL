#pragma warning (disable: 4091 6328 6031)
#pragma once
#include <Windows.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <stdio.h>
#include <assert.h>
#include "../../auth/skStr.h"

#define FILENAMES_ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : \
(strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__))

#if defined(DISABLE_OUTPUT)
#define log(...) ((void)0)
#define log_new_line(...) ((void)0)
#else
#define log(fmt, ...) \
printf(skCrypt("[vext] - [%s:%d] " fmt "\n"), FILENAMES_, __LINE__, ##__VA_ARGS__)

#define log_new_line(...) \
printf(skCrypt("[%s:%d]\n"), FILENAMES_, __LINE__)
#endif

struct RTL_PROCESS_MODULE_INFORMATION {
	PVOID  Section;
	PVOID  MappedBase;
	PVOID  ImageBase;
	ULONG  ImageSize;
	ULONG  Flags;
	USHORT LoadOrderIndex;
	USHORT InitOrderIdnex;
	USHORT LoadCount;
	USHORT OffsetToFileName;
	CHAR   FullPathName[0x100];
};

struct RTL_PROCESS_MODULES {
	ULONG                          NumberOfModules;
	RTL_PROCESS_MODULE_INFORMATION Modules[1];
};
