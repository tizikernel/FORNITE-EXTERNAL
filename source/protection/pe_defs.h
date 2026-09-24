#pragma once
#include <windows.h>
#include <vector>

#ifndef PE_HEADER_DEFINED
#define PE_HEADER_DEFINED

#define PE_HEADER_SIZE 4096

struct PE_HEADER {
    PIMAGE_DOS_HEADER dosHeader;
    PIMAGE_NT_HEADERS ntHeaders;
    PIMAGE_FILE_HEADER fileHeader;
    PIMAGE_OPTIONAL_HEADER optionalHeader;
    PIMAGE_DATA_DIRECTORY dataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
    std::vector<PIMAGE_SECTION_HEADER> sectionHeaders;
};

struct REMOTE_PE_HEADER : PE_HEADER {
    BYTE rawData[PE_HEADER_SIZE];
    ULONG_PTR remoteBaseAddress;
};

#endif
