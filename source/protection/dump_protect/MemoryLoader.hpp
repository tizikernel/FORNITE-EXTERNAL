#pragma once
#include "ILoader.hpp"
#include <vector>

namespace Loader {
    class MemoryLoader : public ILoader {
    public:
        explicit MemoryLoader(const std::vector<unsigned char>& data);
        ~MemoryLoader() override;
        bool Load(ModuleInfo& out) override;
        bool Unload() override;
        void* GetProc(const std::string& name) override;
        std::string GetLastError() const override { return m_lastError; }
    private:
        bool MapHeaders();
        bool MapSections();
        bool HandleRelocations();
        bool HandleImports();
        bool ExecuteTLS();
        bool CallEntryPoint(DWORD reason);
        std::vector<unsigned char> m_rawData;
        void* m_imageMapped = nullptr;
        size_t m_imageSize = 0;
        std::string m_lastError;
        PIMAGE_NT_HEADERS m_ntHeaders = nullptr;
    };
}
