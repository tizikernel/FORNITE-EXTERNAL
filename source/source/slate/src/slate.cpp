#include "../../../protection/security_macros.h"
#include "../include/slate.h"
#include "../resources/fonts.h"
#include "../resources/images.h"
#include "../resources/burbank_font.h"
#include "../../sdk.h"

using namespace ImGui;

struct {
	struct {
		bool enabled = true;
		int type = 1;
		float col[4] { 1.f, 1.f, 1.f, 1.f };
	} box;
	struct {
		bool enabled = true;
		float col[4] { 1.f, 1.f, 1.f, 1.f };
	} name;
	struct {
		bool enabled;
		float col[4] { 1.f, 1.f, 1.f, 1.f };
	} fraction;
	struct {
		bool enabled;
		float col[4] { 1.f, 1.f, 1.f, 1.f };
	} level;
	struct {
		bool enabled;
		float col[4] { 1.f, 1.f, 1.f, 1.f };
	} staticid;
	struct {
		bool enabled = true;
		float col[4] { 1.f, 1.f, 1.f, 1.f };
	} distance;
	struct {
		bool enabled = true;
		float col[4] { 1.f, 1.f, 1.f, 1.f };
	} weapon;
	struct {
		bool enabled;
		float thickness = 1;
		float col[4] { 1.f, 1.f, 1.f, 1.f };
	} skeleton;
	struct {
		bool enabled = false;
		bool rounding = true;
		int type = 0;
	} healthbar;
	struct {
		bool enabled = true;
		bool rounding = true;
		int type = 0;
	} armorbar;
} esp;

void c_slate::initialize_fonts( ) {
	fonts.resize( fonts_size );
	fonts.at( font ).setup( fontbinary, sizeof( fontbinary ),
		{ 16, 14, 12 },
		GetIO( ).Fonts->GetGlyphRangesCyrillic( ) );

	fonts.at( cascadia ).setup( cascadiamonobinary, sizeof( cascadiamonobinary ),
		{ 13 },
		GetIO( ).Fonts->GetGlyphRangesCyrillic( ) );

	const static ImWchar icons_ranges[] = { 0x1 + 1000, 0x1 + 5881, 0 };
	fonts.at( icons ).setup( iconsbinary, sizeof( iconsbinary ),
		{ 18, 16, 14, 12 },
		icons_ranges );
	
	fonts.at( fn_font ).setup( (unsigned char*)burbank_font, burbank_font_size, { 14, 16, 18 }, nullptr );
}

c_image bg;
void c_slate::initialize( ID3D11Device* device, ID3D11DeviceContext* ctx, IDXGISwapChain* swapchain ) {
	GImGui->IO.IniFilename = "";
	g_style->dpi_scale = 1;
	g_style->setup( );
	g_lang->initialize( );

	initialize_fonts( );

	widgets->nav.tabs = {
		{ I_VIEWFINDER, OBF("Aimbot"), { { I_SETTINGS, OBF("General") } } },
		{ I_EYE, OBF("Visuals"), { { I_USERS, OBF("Players") } } },
		{ I_WORLD, OBF("World"), { { I_GLOBE, OBF("World ESP") } } },
		{ I_SETTINGS, OBF("Settings") },
	};

	static bool bools[100];
	static int ints[100];
	static float floats[100];
	static bool multicombov[6];
	static bool multicombov2[6];
	static float col[100][4];

	widgets->nav.addpage( 0, [&] {
		BeginGroup( );
		{
			widgets->child.begin( I_SETTINGS, OBF("Main options") );
			{
				widgets->checkbox( OBF("Enable Aimbot"), &parent::Settings.aimbot_enabled);
				widgets->checkbox( OBF("Enable Prediction"), &parent::Settings.aimbot_prediction );
				widgets->combo( OBF("Hitbox Type"), &parent::Settings.aimbot_bone, { OBF("Head"), OBF("Chest"), OBF("Legs") } );
				widgets->keybind( OBF("Keybind"), &parent::Settings.aimbot_keybind_1 );
				widgets->sliderfloat( OBF("Aimbot Smoothness") , &parent::Settings.aimbot_smoothness , 1.0f , 25.0f , OBF("%.0f%%") );
			}
			widgets->child.end( );
		}
		EndGroup( );

		SameLine( );

		BeginGroup( );
		{
			widgets->child.begin( I_DOTS, OBF("Miscellaneous") );
			{
				widgets->checkbox( OBF("Visible Check"), &parent::Settings.aimbot_visible_check );
				widgets->checkbox( OBF("Ignore Knocked"), &parent::Settings.aimbot_ignore_knocked );
			}
			widgets->child.end( );

			widgets->child.begin( I_ADJUSTMENTS, OBF("Adjustments") );
			{
				widgets->checkpicker( OBF("FOV Circle"), &parent::Settings.aimbot_fov_circle, parent::Settings.aimbot_fov_color );
				widgets->checkpicker(OBF("FOV Filled"), &parent::Settings.aimbot_fov_filled, parent::Settings.aimbot_fov_filled_color);
				widgets->sliderfloat( OBF("FOV Circle Size"), &parent::Settings.aimbot_fov_size, 1.0f, 100.0f, OBF("%.0f%%") );
				widgets->sliderfloat( OBF("Aim Distance"), &parent::Settings.aimbot_distance, 1.0f, 1000.0f, OBF("%.0fm") );
			}
			widgets->child.end( );
		}
		EndGroup( );
	} );
	widgets->nav.addpage(1, [&] {
		BeginGroup();
		{
			widgets->child.begin( I_SETTINGS, OBF("Main options") );
			{
				widgets->checkpicker(OBF("Show Box"), &parent::Settings.box, parent::Settings.boxColor, [&]() {
					widgets->checkbox(OBF("Box Outline"), &parent::Settings.box_outline);
					widgets->sliderfloat(OBF("Thickness"), &parent::Settings.box_thickness, 0, 4, OBF("%.2fs"));
				});
				widgets->checkpicker(OBF("Show Skeletons"), &parent::Settings.skeleton, parent::Settings.skeletonColor, [&]() {
					widgets->checkbox(OBF("Skeleton Outline"), &parent::Settings.skeleton_outline);
					widgets->sliderfloat(OBF("Thickness"), &parent::Settings.skeleton_thickness, 0, 4, OBF("%.2fs"));
				});
				widgets->combo(OBF("Box Type"), &parent::Settings.box_type, { OBF("Normal"), OBF("Cornered Box"), OBF("Filled Box") });
				widgets->combo(OBF("Skeleton Type"), &parent::Settings.skeleton_type, { OBF("Normal"), OBF("Curved") });
				widgets->checkbox(OBF("Show Rank"), &parent::Settings.rank);
				widgets->checkbox(OBF("Show distance"), &parent::Settings.distance);
				widgets->checkbox(OBF("Show Weapon"), &parent::Settings.weapon);
				widgets->checkbox(OBF("Show Platform"), &parent::Settings.platform);
				widgets->sliderfloat(OBF("ESP Distance"), &parent::Settings.visuals_distance, 0, 500, OBF("%.0fm"));
			}
			widgets->child.end();

		}
		EndGroup();

		SameLine();

		BeginGroup();
		{
			widgets->child.begin(I_FENCE, OBF("Filters"));
			{
				widgets->checkbox(OBF("Ignore Team"), &parent::Settings.enemies_only);
				widgets->checkbox(OBF("Ignore Bots"), &parent::Settings.ignore_bots);
				widgets->checkbox( OBF("Streamproof") , &parent::Settings.stream_proof );
			
			}
			widgets->child.end();
			widgets->child.begin(I_RADAR__2, OBF("Radar"));
			{
				widgets->checkbox(OBF("Enable Radar"), &parent::Settings.radar_enabled);
				widgets->sliderint(OBF("Radar Size"), &parent::Settings.radar_size, 0, 100);
				widgets->sliderint(OBF("Radar Range"), &parent::Settings.radar_range, 10, 300, OBF("%d m"));
				widgets->checkbox(OBF("Draggable Radar"), &parent::Settings.radar_drag);
			}
			widgets->child.end();
		}
		EndGroup();
		});

	widgets->nav.addpage(2, [&] {
		BeginGroup();
		{
			widgets->child.begin(I_SETTINGS, OBF("Settings"));
			{
				widgets->checkbox(OBF("Enable Loot ESP"), &parent::Settings.loot_esp);
				widgets->sliderfloat(OBF("Max Distance"), &parent::Settings.world_max_distance, 0, 200, OBF("%.0fm"));
			}
			widgets->child.end();
		}
		EndGroup();

		SameLine();

		BeginGroup();
		{
			widgets->child.begin(I_STARS, OBF("Rarity"));
			{
				widgets->checkbox(OBF("Common"), &parent::Settings.rarity_common);
				widgets->checkbox(OBF("Uncommon"), &parent::Settings.rarity_uncommon);
				widgets->checkbox(OBF("Rare"), &parent::Settings.rarity_rare);
				widgets->checkbox(OBF("Epic"), &parent::Settings.rarity_epic);
				widgets->checkbox(OBF("Legendary"), &parent::Settings.rarity_legendary);
				widgets->checkbox(OBF("Mythic"), &parent::Settings.rarity_mythic);
			}
			widgets->child.end();
		}
		EndGroup();
		});

	widgets->nav.addpage( 3, [&] {
		BeginGroup();
		{
			widgets->child.begin(I_SETTINGS, OBF("Options"));
			{
				widgets->keybind(OBF("Menu Key"), &parent::Settings.menu_key);
				widgets->checkbox(OBF("Enable V-Sync"), &parent::Settings.vsync);
				if (widgets->button(OBF("Unload Cheat"), { 150, 30 })) {
					parent::Settings.unload = true;
				}
			}
			widgets->child.end();
		}
		EndGroup();
	});
	avatar.load( device, avatarb, sizeof( avatarb ) );
	logo.load( device, logo_ghost, sizeof( logo_ghost ) );
	bg.load( device, bgbinary, sizeof( bgbinary ) );
}

void c_slate::draw( ID3D11Device* device, ID3D11DeviceContext* ctx, IDXGISwapChain* swapchain ) {
	m_device = device;
	m_ctx = ctx;
	m_swapchain = swapchain;

	g_style->setup( );
	g_search->initialize( );

	// Ported Watermark
	{
		int fps = (int)ImGui::GetIO().Framerate;
		ImDrawList* draw = ImGui::GetForegroundDrawList();

		std::string brand = ("Axiom ");
		std::string name = ("Private");
		std::string sep = ("  |  ");
		std::string fpstr = std::to_string(fps) + (" FPS");

		float pad_x = 12.f, pad_y = 7.f;
		float text_h = ImGui::GetFontSize();
		float total_w = ImGui::CalcTextSize((brand + name + sep + fpstr).c_str()).x;

		ImVec2 pos = ImVec2(15, 15);
		ImVec2 bgMin = ImVec2(pos.x - pad_x, pos.y - pad_y);
		ImVec2 bgMax = ImVec2(bgMin.x + total_w + pad_x * 2, bgMin.y + text_h + pad_y * 2);

		// background
		draw->AddRectFilled(bgMin, bgMax, IM_COL32(10, 10, 13, 240), 6.f);
		draw->AddRect(bgMin, bgMax, IM_COL32(255, 255, 255, 12), 6.f);

		// left purple accent bar
		draw->AddRectFilled(
			ImVec2(bgMin.x, bgMin.y + 5),
			ImVec2(bgMin.x + 2, bgMax.y - 5),
			IM_COL32(139, 92, 246, 255), 99.f
		);

		float x = bgMin.x + pad_x + 6;
		float y = bgMin.y + pad_y;

		draw->AddText(ImVec2(x, y), IM_COL32(196, 181, 253, 255), brand.c_str());
		x += ImGui::CalcTextSize(brand.c_str()).x;
		draw->AddText(ImVec2(x, y), IM_COL32(200, 198, 210, 255), name.c_str());
		x += ImGui::CalcTextSize(name.c_str()).x;
		draw->AddText(ImVec2(x, y), IM_COL32(80, 78, 95, 255), sep.c_str());
		x += ImGui::CalcTextSize(sep.c_str()).x;
		draw->AddText(ImVec2(x, y), IM_COL32(150, 148, 165, 255), fpstr.c_str());
	}




	

	if (globals.show_menu) {
		PROTECT_MUTATE_START;
		widgets->window.begin(OBF("GUI"), vec2{ 650, 600 });
		{
			BeginChild(OBF("header"), vec2{ 0, 64 }, 0, ImGuiWindowFlags_NoBackground);
			{
				BeginChild(OBF("tabs"), vec2{ 0, 0 }, 0, ImGuiWindowFlags_NoBackground);
				{
					GetWindowDrawList()->AddRectFilled(GetWindowPos(), GetWindowPos() + GetWindowSize(), g_style->col(pcol_bg5), GImGui->Style.WindowRounding, ImDrawFlags_RoundCornersTop);
					widgets->nav.drawtabs();
				}
				EndChild();
			}
			EndChild();

			widgets->spacing(0);

			BeginChild(OBF("main"), { 0, 0 }, 0, ImGuiWindowFlags_NoBackground);
			{
				widgets->nav.drawsubtabs();

				PushStyleVar(ImGuiStyleVar_WindowPadding, vec2{ 16, 16 });
				PushStyleVar(ImGuiStyleVar_Alpha, GImGui->Style.Alpha * widgets->nav.tab_anim * widgets->nav.subtab_anim);
				BeginChild(OBF("content"), vec2{ 0, -36 }, ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoBackground);
				{
					widgets->nav.drawpage();
				}
				EndChild();
				PopStyleVar(2);

				widgets->spacing(0);

				BeginChild(OBF("footer"), { 0, 0 }, 0, ImGuiWindowFlags_NoBackground);
				{
					GetWindowDrawList()->AddCircleFilled(
						GetWindowPos() + ImVec2{ 12 ds, GetWindowHeight() / 2 },
						2.5f, g_style->col(pcol_scheme), 12
					);
					GetWindowDrawList()->AddText(
						GetWindowPos() + ImVec2{ 20 ds, GetWindowHeight() / 2 - GImGui->FontSize / 2 },
						g_style->col(pcol_scheme),
						OBF("Axiom Private")
					);

					PushStyleVar(ImGuiStyleVar_Alpha, GImGui->Style.Alpha * widgets->nav.tab_anim);
					if (widgets->nav.current == 4) {
						SetCursorPos({ GetWindowWidth() - 80- GetWindowHeight() / 2 + 12 ds, GetWindowHeight() / 2 - 12});
						if (widgets->button(OBF("Import"), vec2{ 80, 24 }, { .icon = I_FILE__ARROW__LEFT })) {

						}
					}
					PopStyleVar();
				}
				EndChild();
			}
			EndChild();

			widgets->modal.handle();
			widgets->notify.handle();
		}
		widgets->window.end();
		PROTECT_MUTATE_END;
	}
}
