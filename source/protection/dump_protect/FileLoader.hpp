#pragma once
#include "ILoader.hpp"

namespace Loader {
    class FileLoader : public ILoader {
    public:
        explicit FileLoader(const std::wstring& path);
        ~FileLoader() override;
        bool Load(ModuleInfo& out) override;
        bool Unload() override;
        void* GetProc(const std::string& name) override;
        std::string GetLastError() const override { return "FError"; }
    private:
        std::wstring m_path;
        HMODULE m_hMod = nullptr;
    };
}
