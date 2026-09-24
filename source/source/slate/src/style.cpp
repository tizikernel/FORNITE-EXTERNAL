#include "../include/slate.h"

void c_style::setup( ) {
	static bool init = false;
	if ( !init )
	{
		themes.resize( 2 );

		for ( auto& t : themes ) {
			t.colors.resize( pcol_count );
		}

		colors.resize( pcol_count );

		
		themes[0].colors[pcol_scheme] = pcolor{ 139, 92, 246 };   // purple
		themes[0].colors[pcol_checkboxdot] = pcolor{ 139, 92, 246 };

		themes[0].colors[pcol_text] = pcolor{ 220, 220, 225 };   // softer white
		themes[0].colors[pcol_text2] = pcolor{ 140, 138, 150 };
		themes[0].colors[pcol_text3] = pcolor{ 90, 88, 100, 0.6f };

		themes[0].colors[pcol_bg] = pcolor{ 10, 10, 13 };
		themes[0].colors[pcol_bg2] = pcolor{ 14, 14, 18 };
		themes[0].colors[pcol_bg3] = pcolor{ 20, 20, 26 };
		themes[0].colors[pcol_bg4] = pcolor{ 26, 26, 34 };
		themes[0].colors[pcol_bg5] = pcolor{ 11, 11, 14 };
		themes[0].colors[pcol_bg6] = pcolor{ 16, 16, 20 };

		themes[0].colors[pcol_border] = pcolor{ 255, 255, 255, 0.05f };
		themes[0].colors[pcol_separator] = pcolor{ 255, 255, 255, 0.04f };
		themes[0].colors[pcol_textonscheme] = pcolor{ 237, 233, 254 };   // light lavender

		themes [ 0 ].colors [ pcol_listbox ] = pcolor { 15, 15, 15, 0.6f };
		themes [ 1 ].colors [ pcol_scheme ] = pcolor { 0, 0, 0 };

		themes [ 1 ].colors [ pcol_text ] = pcolor { 20, 20, 20 };
		themes [ 1 ].colors [ pcol_text2 ] = pcolor { 90, 90, 90 };
		themes [ 1 ].colors [ pcol_text3 ] = pcolor { 140, 140, 140 };

		themes [ 1 ].colors [ pcol_bg ] = pcolor { 245, 245, 245 };
		themes [ 1 ].colors [ pcol_bg2 ] = pcolor { 235, 235, 235 };
		themes [ 1 ].colors [ pcol_bg3 ] = pcolor { 225, 225, 225 };
		themes [ 1 ].colors [ pcol_bg4 ] = pcolor { 255, 255, 255 };
		themes [ 1 ].colors [ pcol_bg5 ] = pcolor { 240, 240, 240 };
		themes [ 1 ].colors [ pcol_bg6 ] = themes [ 1 ].colors [ pcol_bg3 ];

		themes [ 1 ].colors [ pcol_border ] = pcolor { 0, 0, 0, 0.08f };
		themes [ 1 ].colors [ pcol_separator ] = pcolor { 0, 0, 0, 0.12f };

		themes [ 1 ].colors [ pcol_checkboxdot ] = pcolor { 20, 20, 20 };
		themes [ 1 ].colors [ pcol_textonscheme ] = pcolor { 255, 255, 255 };

		themes [ 1 ].colors [ pcol_listbox ] = pcolor { 230, 230, 230 };

		init = true;
	}

	for ( int i = 0; i < colors.size( ); ++i ) {
		colors[i].r = ImLerp( colors[i].r, themes[theme].colors[i].r, ImGui::GetIO( ).DeltaTime * 17 );
		colors[i].g = ImLerp( colors[i].g, themes[theme].colors[i].g, ImGui::GetIO( ).DeltaTime * 17 );
		colors[i].b = ImLerp( colors[i].b, themes[theme].colors[i].b, ImGui::GetIO( ).DeltaTime * 17 );
		colors[i].a = ImLerp( colors[i].a, themes[theme].colors[i].a, ImGui::GetIO( ).DeltaTime * 17 );
	}

	GImGui->Style.Colors[ImGuiCol_Text] = colors[pcol_text];
	GImGui->Style.Colors[ImGuiCol_TextDisabled] = colors[pcol_text2];
	GImGui->Style.Colors[ImGuiCol_WindowBg] = colors[pcol_bg];
	GImGui->Style.Colors[ImGuiCol_ChildBg] = colors[pcol_bg2];
	GImGui->Style.Colors[ImGuiCol_Border] = colors[pcol_border];
	GImGui->Style.Colors[ImGuiCol_TextSelectedBg] = colors[pcol_scheme].alpha( 0.3f );
	GImGui->Style.Colors[ImGuiCol_WindowShadow] = pcolor{ 0.f, 0.f, 0.f, 0.0f };

	auto& style = GImGui->Style;

	style.WindowRounding = 8 ds;
    style.WindowPadding = ImVec2{ 0, 0 };
    style.WindowBorderSize = 0;

	style.FrameRounding = 4 ds;
    style.FramePadding = vec2{ 9, 8 };
    style.FrameBorderSize = 0;

    style.PopupRounding = 3 ds;
    style.PopupBorderSize = 0;

	style.ChildRounding = 5 ds;
    style.ChildBorderSize = 0;

	style.ItemSpacing = vec2{ 12, 11 };
	style.ItemInnerSpacing = vec2{ 8, 8 };

    style.ScrollbarRounding = 4;
    style.ScrollbarSize = 4;
    style.WindowMinSize = ImVec2{ 1, 1 };
}

void c_style::push( pcol_ col, pcolor newcol )
{
	pushcache.push_back( { col, g_style->col( col ) } );
	colors[col] = newcol;
}
void c_style::pop( int k )
{
	for ( int i = 0; i < k; ++i ) {
		auto& col = pushcache[pushcache.size( ) - 1];
		colors[col.col] = col.oldcol;
		pushcache.pop_back( );
	}
}