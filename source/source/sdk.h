#pragma once
#include <d3d9.h>
#include <vector>
#include <numbers>
#include "driver.h"
#include "offsets.h"
#include "settings.h"
#include <iostream>

#define M_PI 3.14159265358979323846264338327950288419716939937510

class fvector2d
{
public:
	fvector2d() : x(0.f), y(0.f) {}
	fvector2d(double _x, double _y) : x(_x), y(_y) {}
	~fvector2d() {}
	double x, y;
};

class fvector
{
public:
	fvector() : x(0.f), y(0.f), z(0.f) {}
	fvector(double _x, double _y, double _z) : x(_x), y(_y), z(_z) {}
	~fvector() {}
	double x, y, z;
	inline float dot(fvector v) { return x * v.x + y * v.y + z * v.z; }
	inline float distance(fvector v) { return double(sqrtf(powf(v.x - x, 2.0) + powf(v.y - y, 2.0) + powf(v.z - z, 2.0))); }
	fvector operator-(fvector v) { return fvector(x - v.x, y - v.y, z - v.z); }
};

struct FQuat
{
	double x;
	double y;
	double z;
	double w;
};
struct FTransform
{
	FQuat rot;
	fvector translation;
	uint8_t pad[8];
	fvector scale;
	uint8_t pad2[8];

	D3DMATRIX to_matrix_with_scale() const
	{
		fvector scale_3d(
			(scale.x == 0.0) ? 1.0 : scale.x,
			(scale.y == 0.0) ? 1.0 : scale.y,
			(scale.z == 0.0) ? 1.0 : scale.z
		);

		double x2 = rot.x + rot.x;
		double y2 = rot.y + rot.y;
		double z2 = rot.z + rot.z;
		double xx2 = rot.x * x2;
		double yy2 = rot.y * y2;
		double zz2 = rot.z * z2;
		double yz2 = rot.y * z2;
		double wx2 = rot.w * x2;
		double xy2 = rot.x * y2;
		double wz2 = rot.w * z2;
		double xz2 = rot.x * z2;
		double wy2 = rot.w * y2;

		D3DMATRIX m;

		m._41 = translation.x;
		m._42 = translation.y;
		m._43 = translation.z;

		m._11 = (1.0 - (yy2 + zz2)) * scale_3d.x;
		m._22 = (1.0 - (xx2 + zz2)) * scale_3d.y;
		m._33 = (1.0 - (xx2 + yy2)) * scale_3d.z;

		m._32 = (yz2 - wx2) * scale_3d.z;
		m._23 = (yz2 + wx2) * scale_3d.y;

		m._21 = (xy2 - wz2) * scale_3d.y;
		m._12 = (xy2 + wz2) * scale_3d.x;

		m._31 = (xz2 + wy2) * scale_3d.z;
		m._13 = (xz2 - wy2) * scale_3d.x;

		m._14 = 0.0f;
		m._24 = 0.0f;
		m._34 = 0.0f;
		m._44 = 1.0f;

		return m;
	}
};

inline D3DMATRIX matrix_multiplication(D3DMATRIX pM1, D3DMATRIX pM2)
{
	D3DMATRIX pOut;
	pOut._11 = pM1._11 * pM2._11 + pM1._12 * pM2._21 + pM1._13 * pM2._31 + pM1._14 * pM2._41;
	pOut._12 = pM1._11 * pM2._12 + pM1._12 * pM2._22 + pM1._13 * pM2._32 + pM1._14 * pM2._42;
	pOut._13 = pM1._11 * pM2._13 + pM1._12 * pM2._23 + pM1._13 * pM2._33 + pM1._14 * pM2._43;
	pOut._14 = pM1._11 * pM2._14 + pM1._12 * pM2._24 + pM1._13 * pM2._34 + pM1._14 * pM2._44;
	pOut._21 = pM1._21 * pM2._11 + pM1._22 * pM2._21 + pM1._23 * pM2._31 + pM1._24 * pM2._41;
	pOut._22 = pM1._21 * pM2._12 + pM1._22 * pM2._22 + pM1._23 * pM2._32 + pM1._24 * pM2._42;
	pOut._23 = pM1._21 * pM2._13 + pM1._22 * pM2._23 + pM1._23 * pM2._33 + pM1._24 * pM2._43;
	pOut._24 = pM1._21 * pM2._14 + pM1._22 * pM2._24 + pM1._23 * pM2._34 + pM1._24 * pM2._44;
	pOut._31 = pM1._31 * pM2._11 + pM1._32 * pM2._21 + pM1._33 * pM2._31 + pM1._34 * pM2._41;
	pOut._32 = pM1._31 * pM2._12 + pM1._32 * pM2._22 + pM1._33 * pM2._32 + pM1._34 * pM2._42;
	pOut._33 = pM1._31 * pM2._13 + pM1._32 * pM2._23 + pM1._33 * pM2._33 + pM1._34 * pM2._43;
	pOut._34 = pM1._31 * pM2._14 + pM1._32 * pM2._24 + pM1._33 * pM2._34 + pM1._34 * pM2._44;
	pOut._41 = pM1._41 * pM2._11 + pM1._42 * pM2._21 + pM1._43 * pM2._31 + pM1._44 * pM2._41;
	pOut._42 = pM1._41 * pM2._12 + pM1._42 * pM2._22 + pM1._43 * pM2._32 + pM1._44 * pM2._42;
	pOut._43 = pM1._41 * pM2._13 + pM1._42 * pM2._23 + pM1._43 * pM2._33 + pM1._44 * pM2._43;
	pOut._44 = pM1._41 * pM2._14 + pM1._42 * pM2._24 + pM1._43 * pM2._34 + pM1._44 * pM2._44;

	return pOut;
}

inline D3DMATRIX to_matrix(fvector rot, fvector origin = fvector(0, 0, 0)) {
	float radPitch = (rot.x * float(M_PI) / 180.f);
	float radYaw = (rot.y * float(M_PI) / 180.f);
	float radRoll = (rot.z * float(M_PI) / 180.f);

	float SP = sinf(radPitch);
	float CP = cosf(radPitch);
	float SY = sinf(radYaw);
	float CY = cosf(radYaw);
	float SR = sinf(radRoll);
	float CR = cosf(radRoll);

	D3DMATRIX matrix;
	matrix.m[0][0] = CP * CY;
	matrix.m[0][1] = CP * SY;
	matrix.m[0][2] = SP;
	matrix.m[0][3] = 0.f;

	matrix.m[1][0] = SR * SP * CY - CR * SY;
	matrix.m[1][1] = SR * SP * SY + CR * CY;
	matrix.m[1][2] = -SR * CP;
	matrix.m[1][3] = 0.f;

	matrix.m[2][0] = -(CR * SP * CY + SR * SY);
	matrix.m[2][1] = CY * SR - CR * SP * SY;
	matrix.m[2][2] = CR * CP;
	matrix.m[2][3] = 0.f;

	matrix.m[3][0] = origin.x;
	matrix.m[3][1] = origin.y;
	matrix.m[3][2] = origin.z;
	matrix.m[3][3] = 1.f;

	return matrix;
}

struct Camera
{
	fvector location;
	fvector rotation;
	float fov;
};

struct FNRot
{
	double a;
	char pad_0008[24];
	double b;
	char pad_0028[424];
	double c;
};

namespace cache
{
	inline uintptr_t uworld;
	inline uintptr_t game_instance;
	inline uintptr_t local_players;
	inline uintptr_t player_controller;
	inline uintptr_t local_pawn;
	inline uintptr_t root_component;
	inline uintptr_t player_state;
	inline fvector relative_location;
	inline int my_team_id;
	inline uintptr_t game_state;
	inline uintptr_t player_array;
	inline int player_count;
	inline float closest_distance;
	inline uintptr_t closest_mesh;
	inline Camera local_camera;
	inline D3DMATRIX view_matrix;
}

inline Camera get_view_point()
{
	Camera view_point{};
	uintptr_t location_pointer = Kernel->read<uintptr_t>(cache::uworld + CameraLocation);
	uintptr_t rotation_pointer = Kernel->read<uintptr_t>(cache::uworld + CameraRotation);
	FNRot fnrot{};
	fnrot.a = Kernel->read<double>(rotation_pointer);
	fnrot.b = Kernel->read<double>(rotation_pointer + ROTATION_B_OFFSET);
	fnrot.c = Kernel->read<double>(rotation_pointer + ROTATION_C_OFFSET);
	view_point.location = Kernel->read<fvector>(location_pointer);
	view_point.rotation.x = asin(fnrot.c) * (180.0 / M_PI);
	view_point.rotation.y = ((atan2(fnrot.a * -1, fnrot.b) * (180.0 / M_PI)) * -1) * -1;
	
	view_point.fov = Kernel->read<float>((uintptr_t)cache::player_controller + DEFAULTFOV) * 90.f;
	
	//idk what kind of witchcraft this is
	//uintptr_t camera_manager = Kernel->read<uintptr_t>(cache::player_controller + PLAYERCAMERAMANAGER);
	//float base_fov = Kernel->read<float>(camera_manager + DEFAULTFOV); 
	//float fov_multiplier = Kernel->read<float>(cache::player_controller + 0x3b4);
	//
	//if (fov_multiplier > 0.1f && fov_multiplier < 2.0f) {
	//	view_point.fov = base_fov * fov_multiplier;
	//} else {
	//	view_point.fov = base_fov; 
	//}

	//if (view_point.fov <= 1.0f || view_point.fov >= 170.0f) {
	//	view_point.fov = 90.0f;
	//}

	return view_point;
}

inline fvector2d project_world_to_screen(fvector world_location, Camera vcamera, D3DMATRIX temp_matrix)
{
	fvector vaxisx = fvector(temp_matrix.m[0][0], temp_matrix.m[0][1], temp_matrix.m[0][2]);
	fvector vaxisy = fvector(temp_matrix.m[1][0], temp_matrix.m[1][1], temp_matrix.m[1][2]);
	fvector vaxisz = fvector(temp_matrix.m[2][0], temp_matrix.m[2][1], temp_matrix.m[2][2]);

	fvector vdelta = world_location - vcamera.location;
	fvector vtransformed = fvector(vdelta.dot(vaxisy), vdelta.dot(vaxisz), vdelta.dot(vaxisx));

	if (vtransformed.z < 1.f) vtransformed.z = 1.f;

	return fvector2d(settings::screen_center_x + vtransformed.x * ((settings::screen_center_x / tanf(vcamera.fov * (float)M_PI / 360.f))) / vtransformed.z, settings::screen_center_y - vtransformed.y * ((settings::screen_center_x / tanf(vcamera.fov * (float)M_PI / 360.f))) / vtransformed.z);
}

inline fvector get_entity_bone(uintptr_t mesh, int bone_id)
{
	uintptr_t bone_array = Kernel->read<uintptr_t>(mesh + BONE_ARRAY);
	if (bone_array == 0) bone_array = Kernel->read<uintptr_t>(mesh + BONE_ARRAY + BONE_ARRAY_PADDING);
	FTransform bone = Kernel->read<FTransform>(bone_array + (bone_id * 0x60));
	FTransform component_to_world = Kernel->read<FTransform>(mesh + COMPONENT_TO_WORLD);
	D3DMATRIX matrix = matrix_multiplication(bone.to_matrix_with_scale(), component_to_world.to_matrix_with_scale());
	return fvector(matrix._41, matrix._42, matrix._43);
}

inline fvector get_entity_bone_cached(uintptr_t bone_array, FTransform component_to_world, int bone_id)
{
	FTransform bone = Kernel->read<FTransform>(bone_array + (bone_id * 0x60));
	D3DMATRIX matrix = matrix_multiplication(bone.to_matrix_with_scale(), component_to_world.to_matrix_with_scale());
	return fvector(matrix._41, matrix._42, matrix._43);
}

inline bool is_visible(uintptr_t mesh)
{
	auto proxy = Kernel->read<std::uintptr_t>(std::uintptr_t(mesh) + 0xB0);
	if (!proxy)
		return false;
	auto last_render = Kernel->read<float>(proxy + 0x7DC);
	auto tolerance = Kernel->read<float>(std::uintptr_t(mesh) + 0x330);
	auto last_submit = Kernel->read<double>(proxy + 0x7B8);
	return (std::fmax(0.f, last_render + 0.0001f) >= last_submit - static_cast<double>(tolerance));
}

inline double get_cross_distance(double x1, double y1, double x2, double y2) {
	return sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
}

class game_helper_t {
public:
	fvector getsocketlocation(uintptr_t mesh, int bone_id) {
		return get_entity_bone(mesh, bone_id);
	}
	fvector getsocketlocation_cached(uintptr_t bone_array, FTransform component_to_world, int bone_id) {
		return get_entity_bone_cached(bone_array, component_to_world, bone_id);
	}
	fvector2d ProjectWorldToScreen(fvector world_location, Camera vcamera, D3DMATRIX temp_matrix) {
		return project_world_to_screen(world_location, vcamera, temp_matrix);
	}
	Camera get_camera() {
		return get_view_point();
	}
};
inline game_helper_t game_helper;
inline Camera camera_postion;

class decryption_t {
public:
	std::string GetPlayerName(uintptr_t player_state) {
		if (!player_state) return "";
		return Kernel->read_w_string(player_state + PLAYERNAME);
	}
	void AddPlayerToFovCircle(fvector position, float distance, bool isVisible, ImColor color, Camera vcamera, D3DMATRIX temp_matrix) {
		fvector2d screen_pos = project_world_to_screen(position, vcamera, temp_matrix);
		if (screen_pos.x == 0.0 && screen_pos.y == 0.0) return;
		float cx = globals.ScreenWidth * 0.5f;
		float cy = globals.ScreenHeight * 0.5f;
		float radius = aimbot.FovSize * 1.5f;
		float dx = static_cast<float>(screen_pos.x) - cx;
		float dy = static_cast<float>(screen_pos.y) - cy;
		float len = sqrtf(dx * dx + dy * dy);
		if (len < 0.001f) return;
		float nx = dx / len;
		float ny = dy / len;
		ImVec2 arrow_pos = ImVec2(cx + nx * radius, cy + ny * radius);
		ImU32 arrow_color = isVisible ? IM_COL32(0, 255, 0, 200) : IM_COL32(255, 0, 0, 200);
		ImGui::GetBackgroundDrawList()->AddCircleFilled(arrow_pos, 4.0f, arrow_color);
	}
};
inline decryption_t decryption;
