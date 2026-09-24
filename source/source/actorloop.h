#define IMGUI_DEFINE_MATH_OPERATORS
#define NOMINMAX

#include <D3DX11core.h>
#include <D3DX11tex.h>
#include <d3d9.h>
#include <Uxtheme.h>
#include <Windows.h>
#include <psapi.h>
#include <csignal>
#include <fstream>
#include <D3D11.h>
#include <stdio.h>
#include <thread>
#include <string>
#include <vector>
#include <algorithm>
#include <dwmapi.h>
#include <cfloat>
#include <cmath>
#include <mutex>
#include <chrono>
#include <unordered_map>
#include "icons/ranked_icons.h"
#include "icons/platform_icons.h"
extern ID3D11Device* d3d_device;
#include <array>

#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "imgui_internal.h"
#include "imgui.h"
#include "driver.h"
#include "offsets.h"
#include "drawing.h"
#include "sdk.h"
#include "settings.h"
#include "radar/radar.hpp"
#include "spectators/spectators.hpp"
#include "gui/gui.h"
#include "slate/include/slate.h"
#include "slate/include/font.h"

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

inline std::mutex base;
inline std::vector<entity> entity_list;
inline std::vector<entity> temporary_entity_list;
struct loot_entity { fvector location; std::string name; int rarity; };
inline std::vector<loot_entity> loot_list;
inline std::vector<loot_entity> temporary_loot_list;
inline float     TargetDistance = FLT_MAX;
inline uintptr_t TargetEntity = NULL;
inline uintptr_t DesyncTargetEntity = NULL;
static uintptr_t lockedTarget = NULL;
static bool wasKeyPressed = false;
inline int dbg_loot_levels = 0;
inline int dbg_loot_actors = 0;

// Global variables for the new aimbot keybind system
static bool aimbot_keybind_enabled = false;
static int aimbot_key = 0;
static bool aimbot_active = false;



inline std::string read_ftext(uintptr_t ftext_ptr) {
    if (!ftext_ptr) return "";
    uintptr_t fdata = Kernel->read<uintptr_t>(ftext_ptr + 0x20);
    int flength = Kernel->read<int>(ftext_ptr + 0x28);
    if (flength > 0 && flength < 100) {
        std::vector<wchar_t> buffer(static_cast<size_t>(flength) + 1, L'\0');
        //Kernel->read_physical((void*)fdata, (PVOID)buffer.data(), flength * sizeof(wchar_t));
        Kernel->read_memory(fdata, (PVOID)buffer.data(), flength * sizeof(wchar_t));
        return user_driver::narrow(buffer.data());
    }
    return "";
}

inline std::string get_weapon_name(uintptr_t current_actor) {
    if (!current_actor) return OBF("No Item");

    uintptr_t current_weapon = Kernel->read<uintptr_t>(current_actor + CURRENT_WEAPON);
    if (!current_weapon) return OBF("No Item");

    uintptr_t weapon_data = Kernel->read<uintptr_t>(current_weapon + WEAPON_DATA);
    if (!weapon_data) return OBF("No Item");

    uintptr_t item_name = Kernel->read<uintptr_t>(weapon_data + ITEM_NAME);
    if (!item_name) return OBF("No Item");

    uintptr_t fdata = Kernel->read<uintptr_t>(item_name + 0x20);
    int flength = Kernel->read<int>(item_name + 0x28);

    if (flength > 0 && flength < 50) {
        std::vector<wchar_t> weapon_buffer(static_cast<size_t>(flength) + 1, L'\0');
        //Kernel->read_physical((void*)fdata, (PVOID)weapon_buffer.data(), flength * sizeof(wchar_t));
        Kernel->read_memory(fdata, (PVOID)weapon_buffer.data(), flength * sizeof(wchar_t));
        std::string weapon_name = user_driver::narrow(weapon_buffer.data());
        if (!weapon_name.empty()) {
            return weapon_name;
        }
    }
    return OBF("No Item");
}

inline std::string get_rank_name(int tier)
{
    switch (tier)
    {
    case 0: return OBF("Bronze 1");
    case 1: return OBF("Bronze 2");
    case 2: return OBF("Bronze 3");
    case 3: return OBF("Silver 1");
    case 4: return OBF("Silver 2");
    case 5: return OBF("Silver 3");
    case 6: return OBF("Gold 1");
    case 7: return OBF("Gold 2");
    case 8: return OBF("Gold 3");
    case 9: return OBF("Platinum 1");
    case 10: return OBF("Platinum 2");
    case 11: return OBF("Platinum 3");
    case 12: return OBF("Diamond 1");
    case 13: return OBF("Diamond 2");
    case 14: return OBF("Diamond 3");
    case 15: return OBF("Elite");
    case 16: return OBF("Champion");
    case 17: return OBF("Unreal");
    default: return OBF("Unranked");
    }
}

inline ImVec4 get_rank_color(int tier)
{
    switch (tier)
    {
    case 0: return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    case 1:
    case 2: return ImVec4(0.902f, 0.580f, 0.227f, 1.0f);
    case 3:
    case 4:
    case 5: return ImVec4(0.843f, 0.843f, 0.843f, 1.0f);
    case 6:
    case 7:
    case 8: return ImVec4(1.0f, 0.871f, 0.0f, 1.0f);
    case 9:
    case 10:
    case 11: return ImVec4(0.0f, 0.7f, 0.7f, 1.0f);
    case 12:
    case 13:
    case 14: return ImVec4(0.1686f, 0.3294f, 0.8235f, 1.0f);
    case 15: return ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
    case 16: return ImVec4(1.0f, 0.6f, 0.0f, 1.0f);
    case 17: return ImVec4(0.6f, 0.0f, 0.6f, 1.0f);
    default: return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    }
}

inline bool IsBot(uintptr_t player_state)
{
    uintptr_t username_component = Kernel->read<uintptr_t>(player_state + USERNAME_COMPONENT);
    return (username_component == 0 || username_component == NULL);
}

static std::unordered_map<uintptr_t, std::string> platform_cache;
static std::unordered_map<uintptr_t, int32_t> rank_cache;
static int icon_reads_this_frame = 0;

inline void reset_icon_reads() { icon_reads_this_frame = 0; }

inline int32_t get_rank(uintptr_t player_state)
{
    auto it = rank_cache.find(player_state);
    if (it != rank_cache.end()) return it->second;
    if (icon_reads_this_frame >= 3) return -1; // throttle: max 3 new reads per frame

    int32_t result = -1;
    uintptr_t habenaro = Kernel->read<uintptr_t>(player_state + HABANERO_COMPONENT);
    if (habenaro) {
        int32_t ranked_progress = Kernel->read<int32_t>(habenaro + RANKED_PROGRESS);
        if (ranked_progress >= 0 && ranked_progress <= 18) {
            result = ranked_progress;
        }
    }
    rank_cache[player_state] = result;
    icon_reads_this_frame++;
    return result;
}

inline std::string get_platform(uintptr_t player_state)
{
    auto it = platform_cache.find(player_state);
    if (it != platform_cache.end()) return it->second;
    if (icon_reads_this_frame >= 3) return "";

    // Platform is a direct FString (StrProperty) on FortPlayerState
    std::string platform_str = Kernel->read_w_string(player_state + PLATFORM);
    std::string result = "";

    if (platform_str.find(OBF("WIN")) != std::string::npos) result = OBF("Windows");
    else if (platform_str.find(OBF("PSN")) != std::string::npos || platform_str.find(OBF("PS5")) != std::string::npos) result = OBF("PlayStation");
    else if (platform_str.find(OBF("XBL")) != std::string::npos || platform_str.find(OBF("XSX")) != std::string::npos || platform_str.find(OBF("XSS")) != std::string::npos) result = OBF("Xbox");
    else if (platform_str.find(OBF("SWT")) != std::string::npos) result = OBF("Switch");
    else if (platform_str.find(OBF("AND")) != std::string::npos) result = OBF("Android");
    else if (platform_str.find(OBF("IOS")) != std::string::npos) result = OBF("iOS");

    platform_cache[player_state] = result;
    icon_reads_this_frame++;
    return result;
}

static inline float clampf(float v, float lo, float hi) {
    return std::max(lo, std::min(v, hi));
}
static inline ImU32 AdjustAlpha(ImU32 col, float alpha01) {
    ImVec4 c = ImGui::ColorConvertU32ToFloat4(col);
    c.w = clampf(alpha01, 0.0f, 1.0f);
    return ImGui::ColorConvertFloat4ToU32(c);
}

static inline float AlphaFromThickness(float t) { return clampf(t / 10.0f, 0.0f, 1.0f); }
static inline float StrokeFromThickness(float t) { return clampf(t, 0.5f, 10.0f); }
static inline float TextScaleFromThickness(float t) { return clampf(t, 0.5f, 3.0f); }

inline bool memory_event(fvector newpos)
{
    Kernel->write<fvector>(CachePointers.PlayerController + RotationInput, newpos);
    return true;
}

inline fvector GetLocation(fvector Location)
{
    fvector Loc = fvector(Location.x, Location.y, Location.z);
    return Loc;
}

inline fvector CalcRotation(fvector& zaz, fvector& daz) {
    fvector dalte = zaz - daz;
    fvector ongle;
    float hpm = sqrtf(dalte.x * dalte.x + dalte.y * dalte.y);
    ongle.y = atan(dalte.y / dalte.x) * 57.295779513082f;
    ongle.x = (atan(dalte.z / hpm) * 57.295779513082f) * -1.f;
    if (dalte.x >= 0.f) ongle.y += 180.f;
    return ongle;
}

inline void memory_move(fvector head3d)
{
    fvector2d head2d = game_helper.ProjectWorldToScreen(head3d, camera_postion, cache::view_matrix);
    fvector2d target{};
    const float screencx = static_cast<float>(globals.ScreenWidth) * 0.5f;
    const float screency = static_cast<float>(globals.ScreenHeight) * 0.5f;
    float deltaX = 0.0f;
    float deltaY = 0.0f;
    if (head2d.x != 0) {
        if (head2d.x > screencx) { deltaX = -(screencx - head2d.x); }
        else if (head2d.x < screencx) { deltaX = head2d.x - screencx; }
    }
    if (head2d.y != 0) {
        if (head2d.y > screency) { deltaY = -(screency - head2d.y); }
        else if (head2d.y < screency) { deltaY = head2d.y - screency; }
    }
    float adjustedSmooth = aimbot.Smooth + 3.0f;
    if (adjustedSmooth < 4.0f) adjustedSmooth = 4.0f;
    if (adjustedSmooth > 23.0f) adjustedSmooth = 23.0f;
    float baseSmooth = adjustedSmooth;
    float distance = sqrtf(deltaX * deltaX + deltaY * deltaY);
    float distanceMultiplier = 1.0f;
    if (distance > 0.0f) {
        if (distance < 50.0f) { distanceMultiplier = 0.6f + (50.0f - distance) / 100.0f; }
        else if (distance > 100.0f) {
            distanceMultiplier = 1.3f + (distance - 100.0f) / 500.0f;
            if (distanceMultiplier > 2.0f) distanceMultiplier = 2.0f;
        }
    }
    float effectiveSmooth = baseSmooth * distanceMultiplier;
    target.x = deltaX / effectiveSmooth;
    target.y = deltaY / effectiveSmooth;
    static fvector2d lastTarget = { 0.0f, 0.0f };
    static fvector2d velocity = { 0.0f, 0.0f };
    static fvector2d acceleration = { 0.0f, 0.0f };
    float dynamicSmoothFactor = 0.4f / (adjustedSmooth * 0.1f + 0.5f);
    if (dynamicSmoothFactor > 0.4f) dynamicSmoothFactor = 0.4f;
    if (dynamicSmoothFactor < 0.05f) dynamicSmoothFactor = 0.05f;
    fvector2d targetVelocity = { (target.x - lastTarget.x) * dynamicSmoothFactor, (target.y - lastTarget.y) * dynamicSmoothFactor };
    acceleration.x = (targetVelocity.x - velocity.x) * 0.3f;
    acceleration.y = (targetVelocity.y - velocity.y) * 0.3f;
    velocity.x = velocity.x * 0.85f + acceleration.x;
    velocity.y = velocity.y * 0.85f + acceleration.y;
    target.x = lastTarget.x + (target.x - lastTarget.x) * dynamicSmoothFactor + velocity.x * 0.4f;
    target.y = lastTarget.y + (target.y - lastTarget.y) * dynamicSmoothFactor + velocity.y * 0.4f;
    lastTarget = target;
    float maxMovement = 30.0f + (distance / 20.0f);
    if (maxMovement > 60.0f) maxMovement = 60.0f;
    target.x = clampf(target.x, -maxMovement, maxMovement);
    target.y = clampf(target.y, -maxMovement, maxMovement);
    if (target.x + screencx > screencx * 2) target.x = 0;
    if (target.x + screencx < 0) target.x = 0;
    if (target.y + screency > screency * 2) target.y = 0;
    if (target.y + screency < 0) target.y = 0;
    float sensitivityScale = 2.0f + (adjustedSmooth * 0.8f);
    memory_event(fvector(-target.y / sensitivityScale, target.x / sensitivityScale, 0.0f));
}

inline void cache_loop() {
    temporary_entity_list.clear();
    std::lock_guard<std::mutex> lock(base);
    {
        uintptr_t uengine = Kernel->read<std::uintptr_t>(Kernel->m_base_address + ENGINE_OFFSET);//Updates every update
        uintptr_t viewport = Kernel->read<std::uintptr_t>(uengine + VIEWPORT_OFFSET);//Rarely update

        uintptr_t uworld = Kernel->read<uintptr_t>(viewport + 0x78);//never updates
        if (!uworld || uworld < 0x100000000ULL || uworld > 0x7FFFFFFFFFFFULL) return;
        
        CachePointers.UWorld = uworld;
        cache::uworld = uworld;
    }
    CachePointers.GameInstance = Kernel->read<uintptr_t>(CachePointers.UWorld + GAME_INSTANCE);
    CachePointers.LocalPlayer = Kernel->read<uintptr_t>(Kernel->read<uintptr_t>(CachePointers.GameInstance + LOCAL_PLAYERS));
    CachePointers.PlayerController = Kernel->read<uintptr_t>(CachePointers.LocalPlayer + PLAYER_CONTROLLER);
    cache::player_controller = CachePointers.PlayerController;
    CachePointers.AcknownledgedPawn = Kernel->read<uintptr_t>(CachePointers.PlayerController + ACKNOWLEDGED_PAWN);
    CachePointers.Mesh = CachePointers.AcknownledgedPawn ? Kernel->read<uintptr_t>(CachePointers.AcknownledgedPawn + MESH) : 0;
    CachePointers.RootComponent = CachePointers.AcknownledgedPawn ? Kernel->read<uintptr_t>(CachePointers.AcknownledgedPawn + ROOT_COMPONENT) : 0;
    CachePointers.GameState = Kernel->read<uintptr_t>(CachePointers.UWorld + GAME_STATE);
    if (!CachePointers.GameState) return;

    CachePointers.PlayerArray = Kernel->read<uintptr_t>(CachePointers.GameState + PLAYER_ARRAY);
    if (!CachePointers.PlayerArray) return;

    CachePointers.PlayerArraySize = Kernel->read<int>(CachePointers.GameState + (PLAYER_ARRAY + sizeof(uintptr_t)));
    if (CachePointers.PlayerArraySize <= 0 || CachePointers.PlayerArraySize > 1000) return;

    CachePointers.cameraIDK = Kernel->read<uintptr_t>(CachePointers.PlayerController + PLAYERCAMERAMANAGER);
    CachePointers.TeamIndex = 0;
    for (int i = 0; i < CachePointers.PlayerArraySize; ++i) {
        uintptr_t ps = Kernel->read<uintptr_t>(CachePointers.PlayerArray + i * sizeof(uintptr_t));
        if (!ps) continue;
        uintptr_t pawn_private = Kernel->read<uintptr_t>(ps + PAWN_PRIVATE);
        if (pawn_private == CachePointers.AcknownledgedPawn) {
            CachePointers.TeamIndex = Kernel->read<char>(ps + TEAM_INDEX);
            break;
        }
    }
    entity_list.clear();
    entity_list = temporary_entity_list;

    if (parent::Settings.loot_esp) {
        static auto last_loot_scan = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();

        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_loot_scan).count() > 2000) {
            last_loot_scan = now;
            temporary_loot_list.clear();

            uintptr_t u_persistent_level = Kernel->read<uintptr_t>(CachePointers.UWorld + 0x38);
            uintptr_t levels_ptr = Kernel->read<uintptr_t>(CachePointers.UWorld + LEVELS);
            int levels_count = Kernel->read<int>(CachePointers.UWorld + LEVELS + 0x8);
            
            dbg_loot_levels = levels_count;
            dbg_loot_actors = 0;

            std::vector<uintptr_t> levels_to_scan;
            if (u_persistent_level) levels_to_scan.push_back(u_persistent_level);
            if (levels_count > 0 && levels_count < 1000) {
                for (int i = 0; i < levels_count; i++) {
                    uintptr_t level = Kernel->read<uintptr_t>(levels_ptr + (i * 0x8));
                    if (level && level != u_persistent_level) levels_to_scan.push_back(level);
                }
            }

            for (uintptr_t level : levels_to_scan) {
                uintptr_t actors = Kernel->read<uintptr_t>(level + 0xA0);
                int actors_count = Kernel->read<int>(level + 0xA0 + 0x8);
                
                if (actors_count <= 0 || actors_count > 60000) continue;
                dbg_loot_actors += actors_count;

                for (int j = 0; j < actors_count; j++) {
                    uintptr_t actor = Kernel->read<uintptr_t>(actors + (j * 0x8));
                    if (!actor) continue;

                    // Identify Pickup (FortPickupAthena)
                    uintptr_t item_entry = actor + PRIMARY_PICKUP_ITEM_ENTRY;
                    uintptr_t item_def = Kernel->read<uintptr_t>(item_entry + ITEM_DEFINITION);
                    if (!item_def) continue;

                    // Filter: Weapons (20, 21, 22) and Consumables/Heals (6)
                    uint8_t item_type = Kernel->read<uint8_t>(item_def + ITEM_TYPE);
                    if (item_type != 20 && item_type != 21 && item_type != 22 && item_type != 6) continue;
                    
                    uintptr_t root = Kernel->read<uintptr_t>(actor + ROOT_COMPONENT);
                    if (!root) continue;

                    fvector loc = Kernel->read<fvector>(root + RELATIVE_LOCATION);
                    uint8_t rarity = Kernel->read<uint8_t>(item_def + RARITY);
                    
                    std::string name = read_ftext(Kernel->read<uintptr_t>(item_def + ITEM_NAME));
                    if (name.empty()) name = read_ftext(Kernel->read<uintptr_t>(item_def + 0x40));
                    if (name.empty()) name = (item_type == 20 || item_type == 21 || item_type == 22) ? OBF("Weapon") : OBF("Heal");

                    temporary_loot_list.push_back({ loc, name, 0 });
                    dbg_loot_actors++; // Count successful detections
                }
            }
            loot_list = temporary_loot_list;
        }
    }
}

inline void render_crosshair() {
    if (!aimbot.crosshair) return;

    ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
    ImVec2 screen_center(globals.ScreenWidth / 2.0f, globals.ScreenHeight / 2.0f);
    ImU32 crosshair_color = IM_COL32(255, 255, 255, 255);
    float size = 8.0f;
    float thickness = 1.5f;

    switch (settings::crosshair_type) {
    case 0:
    {
        draw_list->AddLine(
            ImVec2(screen_center.x - size, screen_center.y),
            ImVec2(screen_center.x + size, screen_center.y),
            crosshair_color, thickness
        );
        draw_list->AddLine(
            ImVec2(screen_center.x, screen_center.y - size),
            ImVec2(screen_center.x, screen_center.y + size),
            crosshair_color, thickness
        );
        break;
    }
    case 1:
    {
        draw_list->AddCircleFilled(screen_center, size * 0.3f, crosshair_color);
        draw_list->AddCircle(screen_center, size * 0.3f, IM_COL32(0, 0, 0, 150), 12, thickness * 0.5f);
        break;
    }
    case 2:
    {
        float gap = size * 0.5f;
        draw_list->AddLine(
            ImVec2(screen_center.x, screen_center.y - size),
            ImVec2(screen_center.x, screen_center.y - gap),
            crosshair_color, thickness
        );
        draw_list->AddLine(
            ImVec2(screen_center.x, screen_center.y + gap),
            ImVec2(screen_center.x, screen_center.y + size),
            crosshair_color, thickness
        );
        draw_list->AddLine(
            ImVec2(screen_center.x - size, screen_center.y),
            ImVec2(screen_center.x - gap, screen_center.y),
            crosshair_color, thickness
        );
        draw_list->AddLine(
            ImVec2(screen_center.x + gap, screen_center.y),
            ImVec2(screen_center.x + size, screen_center.y),
            crosshair_color, thickness
        );
        break;
    }
    }
}
struct FOVCircleAnimation {
    float current_radius = 0.0f;
    float fade_alpha = 0.0f;
    bool is_active = false;

    void update(float delta_time, bool enabled, float target_radius) {
        if (enabled) {
            is_active = true;
            fade_alpha = std::min(fade_alpha + delta_time * 4.0f, 1.0f);
            
            const float lerp_speed = 12.0f;
            float lerp_factor = 1.0f - exp(-lerp_speed * delta_time);
            current_radius += (target_radius - current_radius) * lerp_factor;
        }
        else {
            fade_alpha = std::max(fade_alpha - delta_time * 6.0f, 0.0f);
            if (fade_alpha <= 0.0f) {
                is_active = false;
                current_radius = 0.0f;
            }
        }
    }
};
static FOVCircleAnimation fov_anim{};

inline void actor_loop() {
    PROTECT_MUTATE_START;
    static auto GetCatmullRomSpline = [](const std::vector<ImVec2>& points, int segmentsPerCurve) -> std::vector<ImVec2> {
        std::vector<ImVec2> result;
        if (points.size() < 2) return result;
        std::vector<ImVec2> pts;
        pts.push_back(points.front());
        pts.insert(pts.end(), points.begin(), points.end());
        pts.push_back(points.back());
        for (size_t i = 0; i + 3 < pts.size(); i++) {
            ImVec2 p0 = pts[i]; ImVec2 p1 = pts[i + 1]; ImVec2 p2 = pts[i + 2]; ImVec2 p3 = pts[i + 3];
            for (int j = 0; j <= segmentsPerCurve; j++) {
                float t = (float)j / (float)segmentsPerCurve;
                float t2 = t * t; float t3 = t2 * t;
                float x = 0.5f * ((2.0f * p1.x) + (-p0.x + p2.x) * t + (2.0f * p0.x - 5.0f * p1.x + 4.0f * p2.x - p3.x) * t2 + (-p0.x + 3.0f * p1.x - 3.0f * p2.x + p3.x) * t3);
                float y = 0.5f * ((2.0f * p1.y) + (-p0.y + p2.y) * t + (2.0f * p0.y - 5.0f * p1.y + 4.0f * p2.y - p3.y) * t2 + (-p0.y + 3.0f * p1.y - 3.0f * p2.y + p3.y) * t3);
                result.push_back(ImVec2(x, y));
            }
        }
        return result;
    };

    ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
    auto get_color = [](float col[4]) { return IM_COL32((int)(col[0] * 255), (int)(col[1] * 255), (int)(col[2] * 255), (int)(col[3] * 255)); };
    auto get_color_alpha = [](float col[4], float alpha) { return IM_COL32((int)(col[0] * 255), (int)(col[1] * 255), (int)(col[2] * 255), (int)(col[3] * 255 * alpha)); };
    parent::Radar.Clear();
    fvector my_location = camera_postion.location;
    bool isAimbotKeyHeld = (GetAsyncKeyState(parent::Settings.aimbot_keybind_1) & 0x8000) != 0;
    static auto last_time = std::chrono::high_resolution_clock::now();
    auto current_time = std::chrono::high_resolution_clock::now();
    float delta_time = std::chrono::duration<float>(current_time - last_time).count();
    last_time = current_time;

    // --- Performance Optimization: Camera Cache ---
    Camera cam = camera_postion;
    D3DMATRIX cam_mat = to_matrix(cam.rotation);
    fvector cam_vx(cam_mat.m[0][0], cam_mat.m[0][1], cam_mat.m[0][2]);
    fvector cam_vy(cam_mat.m[1][0], cam_mat.m[1][1], cam_mat.m[1][2]);
    fvector cam_vz(cam_mat.m[2][0], cam_mat.m[2][1], cam_mat.m[2][2]);
    float cam_cx = (float)globals.ScreenWidth * 0.5f;
    float cam_cy = (float)globals.ScreenHeight * 0.5f;
    float cam_fov_scale = (cam_cx > 0.0f && cam.fov > 0.0f) ? cam_cx / tanf(cam.fov * (float)M_PI / 360.f) : 1.0f;

    auto project = [&](fvector v) -> fvector2d {//i would use game_helper.ProjectWorldToScreen tbh
        fvector d(v.x - cam.location.x, v.y - cam.location.y, v.z - cam.location.z);
        float tx = (float)(d.x * cam_vy.x + d.y * cam_vy.y + d.z * cam_vy.z);
        float ty = (float)(d.x * cam_vz.x + d.y * cam_vz.y + d.z * cam_vz.z);
        float tz = (float)(d.x * cam_vx.x + d.y * cam_vx.y + d.z * cam_vx.z);
        if (tz < 1.0f) tz = 1.0f;
        return fvector2d(cam_cx + tx * cam_fov_scale / tz, cam_cy - ty * cam_fov_scale / tz);
    };
    ImVec2 closest_target_pos{ 0, 0 };
    float closest_distance = FLT_MAX;
    bool target_found = false;
    bool is_shooting = false;
    if (!isAimbotKeyHeld && wasKeyPressed) { lockedTarget = NULL; }
    wasKeyPressed = isAimbotKeyHeld;
    for (int i = 0; i < CachePointers.PlayerArraySize; i++) {
        uintptr_t player_state = Kernel->read<uintptr_t>(CachePointers.PlayerArray + i * sizeof(uintptr_t));
        if (!player_state) continue;
        uintptr_t current_actor = Kernel->read<uintptr_t>(player_state + PAWN_PRIVATE);
        if (!current_actor) continue;
        uintptr_t skeletalmesh = Kernel->read<uintptr_t>(current_actor + MESH);
        if (!skeletalmesh) continue;
        if (current_actor == CachePointers.AcknownledgedPawn) { continue; }

        int actor_team_id = Kernel->read<int>(player_state + TEAM_INDEX);
        if (parent::Settings.enemies_only && (int)CachePointers.TeamIndex == actor_team_id) continue;

        bool is_bot = (Kernel->read<uint8_t>(player_state + bIsABot) >> 3) & 1;
        if (parent::Settings.ignore_bots && is_bot) continue;

        bool isDying = (Kernel->read<uint8_t>(current_actor + ISDYING) & 0x01); // often bit 0 or similar
        bool isKnocked = (Kernel->read<uint8_t>(current_actor + bIsDBNO) & 0x01);
        if (parent::Settings.aimbot_ignore_knocked && (isDying || isKnocked)) continue;

        // --- Performance Optimization: Bone Array & CTW Cache ---
        int32_t ActiveTransforms = Kernel->read<int32_t>(skeletalmesh + BONE_ARRAY + 0x68);
        DWORD_PTR bonearray = skeletalmesh + BONE_ARRAY + (ActiveTransforms * BONE_ARRAY_PADDING);

        uintptr_t BoneDataPtr = Kernel->read<uintptr_t>(bonearray);
        int32_t BoneCount = Kernel->read<int32_t>(bonearray + 8);
        if (BoneCount == 0) continue;

        /*int32_t ActiveTransforms = Kernel->read<int32_t>(skeletalmesh + BONE_ARRAY + 0x68);
        uintptr_t bone_array = Kernel->read<uintptr_t>(skeletalmesh + BONE_ARRAY);
        if (bone_array == 0) bone_array = Kernel->read<uintptr_t>(skeletalmesh + BONE_ARRAY + (ActiveTransforms * BONE_ARRAY_PADDING));
        if (!bone_array) continue;*/

        FTransform ctw_t = Kernel->read<FTransform>(skeletalmesh + COMPONENT_TO_WORLD);
        D3DMATRIX  ctw_m = ctw_t.to_matrix_with_scale();

        auto getBone = [&](int id) -> fvector {
            FTransform b = Kernel->read<FTransform>(BoneDataPtr + (id * sizeof(FTransform)));

            D3DMATRIX Matrix = matrix_multiplication(b.to_matrix_with_scale(), ctw_m);

            return fvector(Matrix._41, Matrix._42, Matrix._43);
            /*FTransform b = Kernel->read<FTransform>(bone_array + (id * 0x60));
            D3DMATRIX  m = matrix_multiplication(b.to_matrix_with_scale(), ctw_m);
            return fvector(m._41, m._42, m._43);*/
        };

        fvector head3d = getBone(110);
        fvector2d head2d = project(fvector(head3d.x, head3d.y, head3d.z + 40));
        fvector bottom3d = getBone(0);
        fvector2d bottom2d = project(bottom3d);
        bool isPlayerVisible = is_visible(skeletalmesh);
        
        if (parent::Settings.visible_only && !isPlayerVisible) continue;
        fvector2d head_box = project(fvector(head3d.x, head3d.y, head3d.z + 15));
        float distance = camera_postion.location.distance(bottom3d) / 100.f;
        float box_height = fabsf(head2d.y - bottom2d.y);
        float box_width = box_height * 0.40f;
        fvector root_bone = getBone(0);
        fvector foot_bone = getBone(75);
        fvector2d root_box = project(fvector(foot_bone.x, foot_bone.y, foot_bone.z - 35));
        const float halfWidth = box_width * 0.5f;
        ImVec2 topLeft(head_box.x - halfWidth, head_box.y);
        ImVec2 bottomRight(root_box.x + halfWidth, root_box.y);
        bool withinAimDistance = (distance <= parent::Settings.aimbot_distance);
        if (parent::Settings.aimbot_enabled && isAimbotKeyHeld && withinAimDistance) {
            if (CachePointers.cameraIDK) {
                fvector head3d_aim = getBone(parent::Settings.aimbot_bone == 0 ? 110 : (parent::Settings.aimbot_bone == 1 ? 66 : 2));

                if (parent::Settings.aimbot_prediction) {
                    uintptr_t current_weapon = Kernel->read<uintptr_t>(CachePointers.AcknownledgedPawn + CURRENT_WEAPON);
                    if (current_weapon) {
                        uintptr_t weapon_data = Kernel->read<uintptr_t>(current_weapon + WEAPON_DATA);
                        if (weapon_data) {
                            float projectile_speed = Kernel->read<float>(weapon_data + PROJECTILE_SPEED);
                            float projectile_gravity = Kernel->read<float>(weapon_data + PROJECTILE_SPEED + 0x8);

                            if (projectile_speed > 500.0f) {
                                uintptr_t root_comp = Kernel->read<uintptr_t>(current_actor + ROOT_COMPONENT);
                                fvector velocity = Kernel->read<fvector>(root_comp + VELOCITY);

                                float travel_time = distance / (projectile_speed / 100.0f); 
                                head3d_aim.x += velocity.x * travel_time;
                                head3d_aim.y += velocity.y * travel_time;
                                head3d_aim.z += (velocity.z * travel_time) + (0.5f * projectile_gravity * 980.0f * travel_time * travel_time);
                            }
                        }
                    }
                }

                fvector2d head2d_aim = project(head3d_aim);
                const float screencx = static_cast<float>(globals.ScreenWidth) * 0.5f;
                const float screency = static_cast<float>(globals.ScreenHeight) * 0.5f;
                float crossDist = static_cast<float>(get_cross_distance(head2d_aim.x, head2d_aim.y, screencx, screency));
                float fovRadius = (parent::Settings.aimbot_fov_size / 100.0f) * (globals.ScreenWidth / 2.0f);
                float distanceToCenter = std::sqrt(std::pow(head2d_aim.x - (globals.ScreenWidth / 2.0f), 2) + std::pow(head2d_aim.y - (globals.ScreenHeight / 2.0f), 2));
                bool canAim = (distanceToCenter <= fovRadius);
                if (parent::Settings.aimbot_visible_check) { canAim = isPlayerVisible; }

                if (lockedTarget != NULL) {
                    if (current_actor == lockedTarget && canAim) { memory_move(head3d_aim); }
                }
                else {
                    if (head2d_aim.x > 0 && head2d_aim.y > 0 && crossDist <= fovRadius && canAim) {
                        if (crossDist < closest_distance) { 
                            closest_distance = crossDist; 
                            lockedTarget = current_actor;
                            memory_move(head3d_aim);
                        }
                    }
                }
            }
        }
        if (parent::Settings.box && distance <= parent::Settings.visuals_distance) {
            float box_thickness = parent::Settings.box_thickness;
            ImU32 box_color = isPlayerVisible ? get_color(parent::Settings.visibleColor) : get_color(parent::Settings.notVisibleColor);

            ImVec2 tl = topLeft;
            ImVec2 br = bottomRight;
            auto finite = [](const ImVec2& p) { return std::isfinite(p.x) && std::isfinite(p.y); };
            if (finite(tl) && finite(br)) {
                if (tl.x > br.x) std::swap(tl.x, br.x);
                if (tl.y > br.y) std::swap(tl.y, br.y);
                const ImVec2 screen = ImGui::GetIO().DisplaySize;
                tl.x = std::clamp(tl.x, 0.0f, screen.x - 1.0f);
                tl.y = std::clamp(tl.y, 0.0f, screen.y - 1.0f);
                br.x = std::clamp(br.x, 0.0f, screen.x - 1.0f);
                br.y = std::clamp(br.y, 0.0f, screen.y - 1.0f);

                float bw = br.x - tl.x;
                float bh = br.y - tl.y;

                if (bw > 2.0f && bh > 2.0f) {
                    tl.x = IM_FLOOR(tl.x) + 0.5f;
                    tl.y = IM_FLOOR(tl.y) + 0.5f;
                    br.x = IM_FLOOR(br.x) + 0.5f;
                    br.y = IM_FLOOR(br.y) + 0.5f;

                    // --- Box Implementation ---
                    if (parent::Settings.box_type == 0) {
                        drawing::DrawBox(draw_list, tl, br, box_color);
                    }
                    else if (parent::Settings.box_type == 1) {
                        drawing::DrawCornerBox(draw_list, tl, br, box_color, box_thickness);
                    }
                    else if (parent::Settings.box_type == 2) {
                        drawing::DrawFilledBox(draw_list, tl, br, box_color, box_thickness);
                    }
                }
            }
        }
        fvector rel_pos = bottom3d - my_location;
        parent::Radar.AddPlayer(rel_pos, isPlayerVisible, true);
        if (parent::Settings.skeleton && distance <= parent::Settings.visuals_distance) {
            float sk_thickness = parent::Settings.skeleton_thickness;
            ImU32 sk_color = isPlayerVisible ? get_color(parent::Settings.visibleColor) : get_color(parent::Settings.notVisibleColor);

            fvector head3d = getBone(110);
            fvector neck3d = getBone(67);
            fvector chest3d = getBone(66);
            fvector spine3d = getBone(3);
            fvector pelvis3d = getBone(2);
            fvector left_shoulder3d = getBone(9);
            fvector left_elbow3d = getBone(10);
            fvector left_hand3d = getBone(11);
            fvector right_shoulder3d = getBone(38);
            fvector right_elbow3d = getBone(39);
            fvector right_hand3d = getBone(40);
            fvector left_hip3d = getBone(71);
            fvector left_knee3d = getBone(72);
            fvector left_foot3d = getBone(75);
            fvector right_hip3d = getBone(78);
            fvector right_knee3d = getBone(79);
            fvector right_foot3d = getBone(82);

            fvector2d head2d = project(head3d);
            fvector2d neck2d = project(neck3d);
            fvector2d chest2d = project(chest3d);
            fvector2d spine2d = project(spine3d);
            fvector2d pelvis2d = project(pelvis3d);
            fvector2d left_shoulder2d = project(left_shoulder3d);
            fvector2d left_elbow2d = project(left_elbow3d);
            fvector2d left_hand2d = project(left_hand3d);
            fvector2d right_shoulder2d = project(right_shoulder3d);
            fvector2d right_elbow2d = project(right_elbow3d);
            fvector2d right_hand2d = project(right_hand3d);
            fvector2d left_hip2d = project(left_hip3d);
            fvector2d left_knee2d = project(left_knee3d);
            fvector2d left_foot2d = project(left_foot3d);
            fvector2d right_hip2d = project(right_hip3d);
            fvector2d right_knee2d = project(right_knee3d);
            fvector2d right_foot2d = project(right_foot3d);

            auto isValidBone = [](const fvector2d& bone) -> bool {
                return bone.x != 0 && bone.y != 0 &&
                    bone.x > -300 && bone.x < globals.ScreenWidth + 300 &&
                    bone.y > -300 && bone.y < globals.ScreenHeight + 300;
            };

            auto drawBone = [&](const fvector2d& from, const fvector2d& to) {
                if (isValidBone(from) && isValidBone(to)) {
                    draw_list->AddLine(ImVec2(from.x, from.y), ImVec2(to.x, to.y), sk_color, sk_thickness);
                }
            };

            if (parent::Settings.skeleton_type == 1) {
                auto toImVec2 = [](const fvector2d& p) { return ImVec2((float)p.x, (float)p.y); };
                drawBone(head2d, neck2d);
                drawBone(neck2d, chest2d);
                drawBone(chest2d, spine2d);
                drawBone(spine2d, pelvis2d);
                std::vector<std::vector<ImVec2>> chains;
                if (isValidBone(chest2d) && isValidBone(left_shoulder2d) && isValidBone(left_elbow2d) && isValidBone(left_hand2d))
                    chains.push_back({ toImVec2(chest2d), toImVec2(left_shoulder2d), toImVec2(left_elbow2d), toImVec2(left_hand2d) });
                if (isValidBone(chest2d) && isValidBone(right_shoulder2d) && isValidBone(right_elbow2d) && isValidBone(right_hand2d))
                    chains.push_back({ toImVec2(chest2d), toImVec2(right_shoulder2d), toImVec2(right_elbow2d), toImVec2(right_hand2d) });
                if (isValidBone(pelvis2d) && isValidBone(left_hip2d) && isValidBone(left_knee2d) && isValidBone(left_foot2d))
                    chains.push_back({ toImVec2(pelvis2d), toImVec2(left_hip2d), toImVec2(left_knee2d), toImVec2(left_foot2d) });
                if (isValidBone(pelvis2d) && isValidBone(right_hip2d) && isValidBone(right_knee2d) && isValidBone(right_foot2d))
                    chains.push_back({ toImVec2(pelvis2d), toImVec2(right_hip2d), toImVec2(right_knee2d), toImVec2(right_foot2d) });

                for (const auto& chain : chains) {
                    std::vector<ImVec2> spline = GetCatmullRomSpline(chain, 12);
                    if (spline.size() < 2) continue;
                    draw_list->AddPolyline(spline.data(), (int)spline.size(), sk_color, 0, sk_thickness);
                }
            } else {
                drawBone(head2d, neck2d);
                drawBone(neck2d, chest2d);
                drawBone(chest2d, spine2d);
                drawBone(spine2d, pelvis2d);
                drawBone(left_shoulder2d, neck2d);
                drawBone(right_shoulder2d, neck2d);
                drawBone(left_shoulder2d, left_elbow2d);
                drawBone(left_elbow2d, left_hand2d);
                drawBone(right_shoulder2d, right_elbow2d);
                drawBone(right_elbow2d, right_hand2d);
                drawBone(pelvis2d, left_hip2d);
                drawBone(pelvis2d, right_hip2d);
                drawBone(left_hip2d, left_knee2d);
                drawBone(left_knee2d, left_foot2d);
                drawBone(right_hip2d, right_knee2d);
                drawBone(right_knee2d, right_foot2d);
            }

            // Head circle removed as requested
        }
        float text_offset_y = 10.0f;
        if (parent::Settings.distance) {
            std::string distanceText = OBF("[ ") + std::to_string(static_cast<int>(distance)) + OBF("m ]");
            ImVec2 text_size = fonts[fn_font].get(14)->CalcTextSizeA(14, FLT_MAX, 0.0f, distanceText.c_str(), nullptr);
            ImVec2 pos(topLeft.x + (halfWidth * 2 - text_size.x) * 0.5f, bottomRight.y + text_offset_y);
            draw_list->AddText(fonts[fn_font].get(14), 14, pos, IM_COL32(255, 255, 255, 255), distanceText.c_str());
            text_offset_y += 15.0f;
        }
        if (parent::Settings.weapon) {
            std::string weapon_str = get_weapon_name(current_actor);
            ImVec2 text_size = fonts[fn_font].get(14)->CalcTextSizeA(14, FLT_MAX, 0.0f, weapon_str.c_str(), nullptr);
            ImVec2 pos(topLeft.x + (halfWidth * 2 - text_size.x) * 0.5f, bottomRight.y + text_offset_y);
            draw_list->AddText(fonts[fn_font].get(14), 14, pos, IM_COL32(255, 255, 255, 255), weapon_str.c_str());
            text_offset_y += 15.0f;
        }
        if (parent::Settings.rank || parent::Settings.platform) {
            static bool textures_loaded = false;
            if (!textures_loaded && d3d_device != nullptr) {
                D3DX11CreateShaderResourceViewFromMemory(d3d_device, bronze1, sizeof(bronze1), NULL, NULL, &rank_textures[0], NULL);
                D3DX11CreateShaderResourceViewFromMemory(d3d_device, bronze2, sizeof(bronze2), NULL, NULL, &rank_textures[1], NULL);
                D3DX11CreateShaderResourceViewFromMemory(d3d_device, bronze3, sizeof(bronze3), NULL, NULL, &rank_textures[2], NULL);
                D3DX11CreateShaderResourceViewFromMemory(d3d_device, silver1, sizeof(silver1), NULL, NULL, &rank_textures[3], NULL);
                D3DX11CreateShaderResourceViewFromMemory(d3d_device, silver2, sizeof(silver2), NULL, NULL, &rank_textures[4], NULL);
                D3DX11CreateShaderResourceViewFromMemory(d3d_device, silver3, sizeof(silver3), NULL, NULL, &rank_textures[5], NULL);
                D3DX11CreateShaderResourceViewFromMemory(d3d_device, gold1, sizeof(gold1), NULL, NULL, &rank_textures[6], NULL);
                D3DX11CreateShaderResourceViewFromMemory(d3d_device, gold2, sizeof(gold2), NULL, NULL, &rank_textures[7], NULL);
                D3DX11CreateShaderResourceViewFromMemory(d3d_device, gold3, sizeof(gold3), NULL, NULL, &rank_textures[8], NULL);
                D3DX11CreateShaderResourceViewFromMemory(d3d_device, platinum1, sizeof(platinum1), NULL, NULL, &rank_textures[9], NULL);
                D3DX11CreateShaderResourceViewFromMemory(d3d_device, platinum2, sizeof(platinum2), NULL, NULL, &rank_textures[10], NULL);
                D3DX11CreateShaderResourceViewFromMemory(d3d_device, platinum3, sizeof(platinum3), NULL, NULL, &rank_textures[11], NULL);
                D3DX11CreateShaderResourceViewFromMemory(d3d_device, diamond1, sizeof(diamond1), NULL, NULL, &rank_textures[12], NULL);
                D3DX11CreateShaderResourceViewFromMemory(d3d_device, diamond2, sizeof(diamond2), NULL, NULL, &rank_textures[13], NULL);
                D3DX11CreateShaderResourceViewFromMemory(d3d_device, diamond3, sizeof(diamond3), NULL, NULL, &rank_textures[14], NULL);
                D3DX11CreateShaderResourceViewFromMemory(d3d_device, elite, sizeof(elite), NULL, NULL, &rank_textures[15], NULL);
                D3DX11CreateShaderResourceViewFromMemory(d3d_device, champion, sizeof(champion), NULL, NULL, &rank_textures[16], NULL);
                D3DX11CreateShaderResourceViewFromMemory(d3d_device, unreal, sizeof(unreal), NULL, NULL, &rank_textures[17], NULL);
                D3DX11CreateShaderResourceViewFromMemory(d3d_device, unranked, sizeof(unranked), NULL, NULL, &rank_textures[18], NULL);
                
                D3DX11CreateShaderResourceViewFromMemory(d3d_device, winlogo, sizeof(winlogo), NULL, NULL, &platform_textures[0], NULL);
                D3DX11CreateShaderResourceViewFromMemory(d3d_device, pslogo, sizeof(pslogo), NULL, NULL, &platform_textures[1], NULL);
                D3DX11CreateShaderResourceViewFromMemory(d3d_device, xboxlogo, sizeof(xboxlogo), NULL, NULL, &platform_textures[2], NULL);
                D3DX11CreateShaderResourceViewFromMemory(d3d_device, nintendologo, sizeof(nintendologo), NULL, NULL, &platform_textures[3], NULL);
                D3DX11CreateShaderResourceViewFromMemory(d3d_device, androidlogo, sizeof(androidlogo), NULL, NULL, &platform_textures[4], NULL);
                D3DX11CreateShaderResourceViewFromMemory(d3d_device, ioslogo, sizeof(ioslogo), NULL, NULL, &platform_textures[5], NULL);
                
                textures_loaded = true;
            }

            int num_icons_to_draw = 0;
            ID3D11ShaderResourceView* rank_tex = nullptr;
            ID3D11ShaderResourceView* plat_tex = nullptr;

            if (parent::Settings.rank) {
                int tier = -1;
                uintptr_t habanero = Kernel->read<uintptr_t>(player_state + HABANERO_COMPONENT);
                if (habanero) {
                    tier = Kernel->read<int>(habanero + RANKED_PROGRESS + 0x18);
                }
                
                if (tier >= 1 && tier <= 18) {
                    rank_tex = rank_textures[tier - 1];
                } else {
                    rank_tex = rank_textures[18]; // Unranked
                }
                if (rank_tex) num_icons_to_draw++;
            }

            if (parent::Settings.platform) {
                std::string platform_str = "";
                uintptr_t plat_ptr = Kernel->read<uintptr_t>(player_state + PLATFORM);
                if (plat_ptr) {
                    wchar_t buf[16] = {0};
                    Kernel->read_memory(plat_ptr, buf, sizeof(buf));
                    platform_str = user_driver::narrow(buf);
                }

                if (platform_str == "WIN" || platform_str == "PC") plat_tex = platform_textures[0];
                else if (platform_str == "PSN") plat_tex = platform_textures[1];
                else if (platform_str == "XBL") plat_tex = platform_textures[2];
                else if (platform_str == "SWT") plat_tex = platform_textures[3];
                else if (platform_str == "AND") plat_tex = platform_textures[4];
                else if (platform_str == "IOS") plat_tex = platform_textures[5];
                else plat_tex = platform_textures[0]; // PC fallback

                if (plat_tex) num_icons_to_draw++;
            }

            if (num_icons_to_draw > 0) {
                float icon_size = halfWidth * 0.5f; // Scale icon relative to the player's box width (halfWidth * 2 * 0.25 = halfWidth * 0.5)
                if (icon_size > 18.0f) icon_size = 18.0f; // Max size
                if (icon_size < 6.0f) icon_size = 6.0f;   // Min size
                
                float gap = 4.0f;
                float total_width = (num_icons_to_draw * icon_size) + ((num_icons_to_draw - 1) * gap);
                float start_x = topLeft.x + (halfWidth * 2 - total_width) * 0.5f;
                float start_y = topLeft.y - icon_size - 5.0f;

                if (rank_tex) {
                    draw_list->AddImage((ImTextureID)rank_tex, ImVec2(start_x, start_y), ImVec2(start_x + icon_size, start_y + icon_size));
                    start_x += icon_size + gap;
                }
                if (plat_tex) {
                    draw_list->AddImage((ImTextureID)plat_tex, ImVec2(start_x, start_y), ImVec2(start_x + icon_size, start_y + icon_size));
                }
            }
        }

        if (is_bot) {
            float icon_size = halfWidth * 0.5f;
            if (icon_size > 18.0f) icon_size = 18.0f;
            if (icon_size < 6.0f) icon_size = 6.0f;

            float y_offset = 5.0f;
            if (parent::Settings.rank || parent::Settings.platform) {
                y_offset += icon_size + 3.0f; // padding
            }
            
            float bot_font_size = 14.0f;
            ImVec2 text_size = fonts[fn_font].get(bot_font_size)->CalcTextSizeA(bot_font_size, FLT_MAX, 0.0f, "BOT", nullptr);
            ImVec2 pos(topLeft.x + (halfWidth * 2 - text_size.x) * 0.5f, topLeft.y - bot_font_size - y_offset);
            draw_list->AddText(fonts[fn_font].get(bot_font_size), bot_font_size, pos, IM_COL32(255, 255, 255, 255), "BOT");
        }
    }
    float target_radius = (parent::Settings.aimbot_fov_size / 100.0f) * (globals.ScreenWidth / 2.0f);
    fov_anim.update(delta_time, parent::Settings.aimbot_fov_circle, target_radius);
    if (fov_anim.is_active && fov_anim.fade_alpha > 0.0f) {
        ImVec2 screen_center(globals.ScreenWidth / 2.0f, globals.ScreenHeight / 2.0f);
        ImU32 color = get_color_alpha(parent::Settings.aimbot_fov_color, fov_anim.fade_alpha);
        ImGui::GetBackgroundDrawList()->AddCircle(screen_center, fov_anim.current_radius, color, 64, 1.5f);
    }

    // Assuming you have a separate toggle for the filled version, e.g., aimbot_fov_filled
    fov_anim.update(delta_time, parent::Settings.aimbot_fov_filled, target_radius);

    if (fov_anim.is_active && fov_anim.fade_alpha > 0.0f) {
        ImVec2 screen_center(globals.ScreenWidth / 2.0f, globals.ScreenHeight / 2.0f);

        // Grabbing the color. Make sure the base color here has a low alpha in your settings, 
        // otherwise you won't be able to see through the center of the circle!
        ImU32 color = get_color_alpha(parent::Settings.aimbot_fov_filled_color, fov_anim.fade_alpha);

        // Swapped to AddCircleFilled and removed the 1.5f thickness parameter
        ImGui::GetBackgroundDrawList()->AddCircleFilled(screen_center, fov_anim.current_radius, color, 64);
    }

    // --- Radar Drawing Implementation (Diamond Style) ---
    if (parent::Settings.radar_enabled) {
        ImGui::SetNextWindowSize(ImVec2((float)parent::Settings.radar_size * 2.0f, (float)parent::Settings.radar_size * 2.0f));
        ImGuiWindowFlags radar_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground;
        if (!parent::Settings.radar_drag) radar_flags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoInputs;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        if (ImGui::Begin("##RadarWindow", nullptr, radar_flags)) {
            ImVec2 draw_pos = ImGui::GetWindowPos();
            float radar_radius = (float)parent::Settings.radar_size;
            ImVec2 center = ImVec2(draw_pos.x + radar_radius, draw_pos.y + radar_radius);
            ImDrawList* radar_draw = ImGui::GetWindowDrawList();

            // Draw radar background (Circular)
            radar_draw->AddCircleFilled(center, radar_radius, IM_COL32(0, 0, 0, 150));

            // Draw 4 rings
            for (int i = 1; i <= 4; i++) {
                float ring_radius = (radar_radius * i) / 4.0f;
                radar_draw->AddCircle(center, ring_radius, IM_COL32(100, 100, 100, 150), 64, 1.0f);
            }

            // === Draw gray FOV cone ===
            float fov_deg = camera_postion.fov;
            float half_fov = (fov_deg * 0.5f) * ((float)M_PI / 180.0f);
            float line_length = radar_radius * 0.9f;
            float forward_angle = -(float)M_PI / 2.0f; // -90 degrees = up

            ImVec2 left_edge(center.x + cosf(forward_angle - half_fov) * line_length, center.y + sinf(forward_angle - half_fov) * line_length);
            ImVec2 right_edge(center.x + cosf(forward_angle + half_fov) * line_length, center.y + sinf(forward_angle + half_fov) * line_length);

            radar_draw->AddTriangleFilled(center, left_edge, right_edge, IM_COL32(128, 128, 128, 80));
            radar_draw->AddTriangle(center, left_edge, right_edge, IM_COL32(128, 128, 128, 150), 1.0f);

            // Local player (center)
            radar_draw->AddCircleFilled(center, 3.5f, IM_COL32(255, 255, 255, 255));

            // Enemy dots
            float range = (float)parent::Settings.radar_range * 100.0f;
            float yaw_rad = (float)camera_postion.rotation.y * (float)M_PI / 180.0f;
            float cos_y = cosf(yaw_rad);
            float sin_y = sinf(yaw_rad);

            for (const auto& p : parent::Radar.players) {
                float rx = p.rel_y;
                float ry = p.rel_x;
                
                float dx = rx * cos_y - ry * sin_y;
                float dy = rx * sin_y + ry * cos_y;

                float dist = sqrtf(dx * dx + dy * dy);
                if (dist > range) {
                    dx *= range / dist;
                    dy *= range / dist;
                }

                float screen_x = center.x + (dx / range) * radar_radius;
                float screen_y = center.y - (dy / range) * radar_radius;

                radar_draw->AddCircleFilled(ImVec2(screen_x, screen_y), 3.0f, IM_COL32(0, 255, 255, 255));
            }
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }
    // --- Loot ESP Drawing ---
    if (parent::Settings.loot_esp) {
        for (const auto& loot : loot_list) {
            float dist = camera_postion.location.distance(loot.location) / 100.0f;
            if (dist > (parent::Settings.world_max_distance > 0 ? parent::Settings.world_max_distance : 250.0f)) continue;

            fvector2d screen_pos = game_helper.ProjectWorldToScreen(loot.location, camera_postion, cache::view_matrix);
            if (screen_pos.x > 0 && screen_pos.y > 0 && screen_pos.x < globals.ScreenWidth && screen_pos.y < globals.ScreenHeight) {
                ImU32 col = IM_COL32(0, 255, 255, 255); // Cyan (#00ffff)

                char buf[128];
                sprintf(buf, "%s [%.0fm]", loot.name.c_str(), dist);
                ImVec2 text_size = fonts[fn_font].get(14)->CalcTextSizeA(14, FLT_MAX, 0.0f, buf, nullptr);
                draw_list->AddText(fonts[fn_font].get(14), 14, ImVec2(screen_pos.x - text_size.x / 2, screen_pos.y), col, buf);
            }
        }
    }
    PROTECT_MUTATE_END;
}
