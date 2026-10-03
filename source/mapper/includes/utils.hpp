#pragma once

#include <Windows.h>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>

inline void mapper_log_write(const std::wstring& w) {
	static std::ofstream f;
	if (!f.is_open()) {
		char path[MAX_PATH]{};
		GetModuleFileNameA(nullptr, path, MAX_PATH);
		std::string p(path);
		auto pos = p.find_last_of("\\/");
		if (pos != std::string::npos) p = p.substr(0, pos + 1);
		p += "mapper_log.txt";
		f.open(p, std::ios::out | std::ios::app);
	}
	if (!f.is_open()) return;
	std::string s;
	s.reserve(w.size());
	for (wchar_t ch : w) s.push_back(static_cast<char>(ch & 0xFF));
	SYSTEMTIME st{};
	GetLocalTime(&st);
	char ts[32];
	sprintf_s(ts, "[%02d:%02d:%02d] ", st.wHour, st.wMinute, st.wSecond);
	f << ts << s;
	f.flush();
}

struct MapperLogStream {
	std::wstringstream ss;
	template<typename T>
	MapperLogStream& operator<<(const T& v) { ss << v; return *this; }
	MapperLogStream& operator<<(std::wostream& (*manip)(std::wostream&)) {
		if (manip == static_cast<std::wostream& (*)(std::wostream&)>(std::endl)) {
			ss << L"\n";
			mapper_log_write(ss.str());
#ifndef DISABLE_OUTPUT
			std::wcout << ss.str();
#endif
			ss.str(L"");
			ss.clear();
		}
		return *this;
	}
};

#if defined(DISABLE_OUTPUT)
#define Log(content) do { MapperLogStream() << content; } while(0)
#else
#define Log(content) do { MapperLogStream() << content; } while(0)
#endif

namespace utils
{
	std::wstring GetFullTempPath();
	bool ReadFileToMemory(const std::wstring& file_path, std::vector<BYTE>* out_buffer);
	bool CreateFileFromMemory(const std::wstring& desired_file_path, const char* address, size_t size);
	uint64_t GetKernelModuleAddress(const std::string& module_name);
	BOOLEAN bDataCompare(const BYTE* pData, const BYTE* bMask, const char* szMask);
	PVOID FindSection(const char* sectionName, uintptr_t modulePtr, PULONG size);
	std::wstring GetCurrentAppFolder();
}
