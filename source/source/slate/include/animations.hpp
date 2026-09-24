#pragma once
#include <map>
#include <string>
#include "imgui.h"
#include "imgui_internal.h"
#include "types.h"

inline float anim(float current, float target_if_false, float target_if_true, bool condition) {
    float target = condition ? target_if_true : target_if_false;
    return ImLerp(current, target, ImGui::GetIO().DeltaTime * 17.f);
}

template<typename T>
T& anim_obj(const char* label, int id, T default_val) {
    static std::map<std::string, T> anims;
    std::string key;
    if (label) key += label;
    key += std::to_string(id);
    if (anims.find(key) == anims.end()) {
        anims[key] = default_val;
    }
    return anims[key];
}

inline pcolor col_anim(pcolor a, pcolor b, float t) {
    return pcolor(
        ImLerp((float)a.r, (float)b.r, t),
        ImLerp((float)a.g, (float)b.g, t),
        ImLerp((float)a.b, (float)b.b, t),
        ImLerp((float)a.a, (float)b.a, t)
    );
}

inline ImVec4 col_anim(ImVec4 a, ImVec4 b, float t) {
    return ImVec4(
        ImLerp(a.x, b.x, t),
        ImLerp(a.y, b.y, t),
        ImLerp(a.z, b.z, t),
        ImLerp(a.w, b.w, t)
    );
}
