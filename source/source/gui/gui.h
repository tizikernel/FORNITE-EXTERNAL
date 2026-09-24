#pragma once
#include "../../imgui/imgui.h"

// GUI stub - the old ui:: namespace is replaced by Slate framework
// This file exists for compatibility with includes in actorloop.h

namespace ui {
    // These are now handled by slate->initialize() and slate->draw()
    inline void initialize_fonts() {}
    inline void initialize_images() {}
    inline void initialize_tabs() {}
    inline ImVec2 size = ImVec2(650, 600);
    inline void render_background() {}
    inline void render_tabs() {}
    inline void render_title() {}
    inline void render_tabs_content() {}
    inline void render_outline() {}
}
