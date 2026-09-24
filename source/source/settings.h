#pragma once
#include <Windows.h>
#include <cstdint>
#include "settings_new/settings.hpp"

// ==========================================
// Compatibility layer: maps old settings 
// namespaces to parent::Settings
// ==========================================

namespace settings {
	inline float screen_center_x = 0;
	inline float screen_center_y = 0;
	inline int crosshair_type = 0;
}

struct globals_t {
	int ScreenWidth = 0;
	int ScreenHeight = 0;
	bool show_menu = true;
	bool radar_drag = false;
	bool radar_enabled = false;
	float radar_opacity = 0.5f;
	bool vsync = false;
	bool rank = true;
	bool platform = true;
	HWND window_handle = nullptr;
};
inline globals_t globals;

// Aimbot compatibility struct that maps to parent::Settings
struct aimbot_t {
	// These now read/write from parent::Settings directly
	float& get_smooth() { return parent::Settings.aimbot_smoothness; }
	float& get_fov_size() { return parent::Settings.aimbot_fov_size; }

	int Mode = 0; // 0 = Memory
	bool crosshair = false;

	// Properties that map to parent::Settings
	__declspec(property(get = get_smooth, put = set_smooth)) float Smooth;
	void set_smooth(float v) { parent::Settings.aimbot_smoothness = v; }

	__declspec(property(get = get_fov_size, put = set_fov_size)) float FovSize;
	void set_fov_size(float v) { parent::Settings.aimbot_fov_size = v; }
};
inline aimbot_t aimbot;

// inline uintptr_t base_address = 0; // Already defined in driver.h

struct entity {
	uintptr_t actor;
	uintptr_t mesh;
	uintptr_t player_state;
	uintptr_t root_component;
	int team_id;
};

struct CachePointers_t {
	uintptr_t UWorld;
	uintptr_t GameInstance;
	uintptr_t LocalPlayer;
	uintptr_t PlayerController;
	uintptr_t AcknownledgedPawn;
	uintptr_t Mesh;
	uintptr_t RootComponent;
	uintptr_t GameState;
	uintptr_t PlayerArray;
	int PlayerArraySize;
	uintptr_t cameraIDK;
	int TeamIndex;
};
inline CachePointers_t CachePointers;

// Forward declarations for D3D globals used by overlay
inline ID3D11Device* g_pd3dDevice = nullptr;
inline ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
inline IDXGISwapChain* g_pSwapChain = nullptr;
inline ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
