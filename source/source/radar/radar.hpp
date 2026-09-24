#pragma once
#include <vector>
#include <cmath>

// Forward declare fvector from sdk.h
struct fvector;

namespace parent {
    struct RadarPlayer {
        float rel_x = 0;
        float rel_y = 0;
        bool visible = false;
        bool enemy = true;
    };

    class radar_c {
    public:
        std::vector<RadarPlayer> players;

        void Clear() {
            players.clear();
        }

        void AddPlayer(const fvector& rel_pos, bool visible, bool enemy) {
            RadarPlayer p;
            // fvector has x, y, z as doubles
            p.rel_x = static_cast<float>(*reinterpret_cast<const double*>(&rel_pos));
            p.rel_y = static_cast<float>(*(reinterpret_cast<const double*>(&rel_pos) + 1));
            p.visible = visible;
            p.enemy = enemy;
            players.push_back(p);
        }

        void RenderRadar() {}
    };
    inline radar_c Radar;
}
