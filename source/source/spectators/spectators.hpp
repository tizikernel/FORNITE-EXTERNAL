#pragma once
#include <vector>
#include <string>

namespace parent {
    class spectators_c {
    public:
        std::vector<std::string> CurrentSpectatorNames;
    };
    inline spectators_c Spectators;
}
