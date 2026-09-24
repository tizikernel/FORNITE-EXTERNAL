#pragma once
#include <string>

namespace parent {
    class values_c {
    public:
        int numSolos = 0, numDuos = 0, numTrios = 0, numSquads = 0;
        void* GWorld = nullptr;
    };
    inline values_c Values;
}
