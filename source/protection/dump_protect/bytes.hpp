#pragma once
#include <vector>

namespace Loader {
    inline unsigned char randomModuleData[] = {
        0x00 // Here put the bytes of your loader. Added 0x00 to fix compilation error.
    };
    inline size_t randomModuleSize = sizeof(randomModuleData);
    inline std::vector<unsigned char> GetEmbeddedModule() {
        return std::vector<unsigned char>(randomModuleData, randomModuleData + randomModuleSize);
    }
}
