#include "FileLoader.hpp"

namespace Loader {
    FileLoader::FileLoader(const std::wstring& path) : m_path(path) {}
    FileLoader::~FileLoader() { Unload(); }
    bool FileLoader::Load(ModuleInfo& out) {
        if (m_hMod) return true;
        m_hMod = LoadLibraryExW(m_path.c_str(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
        if (!m_hMod) return false;
        out.handle = m_hMod; out.isMapped = true; out.size = 0;
        return true;
    }
    bool FileLoader::Unload() {
        if (m_hMod) { FreeLibrary(m_hMod); m_hMod = nullptr; return true; }
        return false;
    }
    void* FileLoader::GetProc(const std::string& name) {
        return m_hMod ? (void*)GetProcAddress(m_hMod, name.c_str()) : nullptr;
    }
}
