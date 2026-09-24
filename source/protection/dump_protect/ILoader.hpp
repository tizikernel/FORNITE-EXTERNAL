#pragma once
#include <windows.h>
#include <string>
#include <vector>

namespace Loader {
    struct ModuleInfo {
        HMODULE handle;
        void* entryPoint;
        size_t size;
        bool isMapped;
    };

    class ILoader {
    public:
        virtual ~ILoader() = default;
        virtual bool Load(ModuleInfo& outInfo) = 0;
        virtual bool Unload() = 0;
        virtual void* GetProc(const std::string& procName) = 0;
        virtual std::string GetLastError() const = 0;
    };
}
