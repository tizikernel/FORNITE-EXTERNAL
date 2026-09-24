#pragma once
#include <windows.h>
#include <vector>
#include <string>

namespace Loader {
    class ProcessHollower {
    public:
        static bool Execute(const std::vector<unsigned char>& bytes, const std::wstring& host = L"");
    private:
        static std::wstring GetDefaultHost();
    };
}
