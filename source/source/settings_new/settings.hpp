#pragma once

#include <Windows.h>
#include "../../imgui/imgui.h"
namespace parent {
	class settings_c {
	public:
		//box
		bool box = 1;
		bool box_outline = 1;
		int box_type = 0;
		bool custom_box_color = false;
		float box_thickness = 1.5f;

		//skeleton
		bool skeleton = 1;
		bool skeleton_outline = 1;
		float skeleton_thickness = 1.5f;
		bool custom_skeleton_color = false;
		int skeleton_type;
		bool curved_skeleton = false;
		bool GlowBones = false;
		float GlowBonesStrength = 1.0f;
		bool head_circle = false;
		float head_circle_size = 1.0f;

		bool default_outline_colors = false;
		bool custom_outline = false;

		//visuals
		bool rank = true;
		bool level = false;
		bool kills = false;
		bool weapon = false;
		bool ammo = false;
		bool rarity = false;
		bool distance = false;
		bool username = false;
		bool teamid = false;
		bool teamsize = false;
		bool platform = true;
		float visuals_distance = 500.0f;
		bool filled_fov;
		bool snapline;
		bool custom_snapline_color;
		bool snapline_outline;
		float snapline_thickness = 1.0f;
		int snapline_pos = 1;

		bool customcolor;
		bool viewline = 1;
		

		//colors
		float visibleColor[4] = { 0.0f, 1.0f, 1.0f, 1.0f }; 
		float skeletonColor[4] = { 0.0f, 1.0f, 1.0f, 1.0f };
		float boxColor[4] = { 0.0f, 1.0f, 1.0f, 1.0f };
		float snaplineColor[4] = { 0.0f, 1.0f, 1.0f, 1.0f };


		float outline_color[4] = { 0.0f, 1.0f, 1.0f, 1.0f };

		float notVisibleColor[4] = { 1.0f, 0.0f, 0.0f, 1.0f }; 
		float knockedColor[4] = { 1.0f, 0.65f, 0.0f, 1.0f }; 
		
		//text
		float background_color[4] = { 1.0f, 0.65f, 0.0f, 1.0f };
		bool text_outline = true;
		bool text_background = false;
		float font_size = 16.0f;

		//filters
		bool visible_only = false;
		bool enemies_only = true;
		bool standing_only = false;
		bool ignore_npc = false;
		bool ignore_bots = false;

		//radar
		float radar_opacity = 50.0f;
		int radar_size = 100;
		int radar_range = 100;
		bool radar_drag = 0;
		bool radar_enabled = false;
		ImVec2 radar_pos = ImVec2(200, 200);

		// aimbot
		bool aimbot_enabled = true;
		int aimbot_type = 0; // 0 = Memory, 1 = Aim Assist
		int aimbot_distance_control = 0; // 0 = Sight, 1 = Near
		int aimbot_angle_type = 0; // 0 = Static, 1 = Curved, 2 = Angular
		int aimbot_keybind_1 = 0x02;
		int aimbot_keybind_2 = 4;
		int aimbot_bone_type = 0; // 0 = One, 1 = Multi, 2 = Closest
		int aimbot_bone = 0;
		bool aimbot_multi_bones[15] = { true, false, false, false, false, false, false, false, false, false, false, false, false, false, false };
		bool aimbot_fov_circle = true;
		bool aimbot_fov_filled = false; // Good practice to initialize this to false
		float aimbot_fov_size = 10.0f; // % of min screen side
		float aimbot_fov_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f }; // Solid white outline
		float aimbot_fov_filled_color[4] = { 1.0f, 1.0f, 1.0f, 0.2f }; // White fill with 20% opacity
		float aimbot_smoothness = 35.0f; // %
		bool aimbot_visible_check = true;
		bool aimbot_lock_enemy = false;
		bool aimbot_ignore_knocked = true;
		float aimbot_distance = 200.0f; // meters
		bool aimbot_config_on_spectate = false;
		bool aimbot_prediction = false;
		bool stream_proof = false;

		//world
		bool loot_esp = false;
		bool chest_esp = false;
		float world_max_distance = 100.0f;
		bool rarity_common = true;
		bool rarity_uncommon = true;
		bool rarity_rare = true;
		bool rarity_epic = true;
		bool rarity_legendary = true;
		bool rarity_mythic = true;

		//options
		int menu_key = VK_INSERT;
		bool vsync = false;
		bool unload = false;
		bool debug_esp = true;
	};
	inline settings_c Settings;
}
