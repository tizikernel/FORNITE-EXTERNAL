#define NOMINMAX
#include "../include/slate.h"
#include "../../imgui/imgui_freetype.h"
#include <thread>

using namespace ImGui;

vec2::operator ImVec2( ) const {
	return ImVec2{ x, y } ds;
}

void c_font::setup( unsigned char* data, size_t data_size, std::vector< float > sizes, const ImWchar* ranges ) {
	auto config = ImFontConfig( );
	config.FontDataOwnedByAtlas = false;

	for ( auto& sz : sizes ) {
		fonts.push_back( { sz, ImGui::GetIO( ).Fonts->AddFontFromMemoryTTF( data, data_size, sz * g_style->dpi_scale, &config, ranges ) } );
	}
}
void c_font::setup_from_file( const char* file_path, std::vector< float > sizes, const ImWchar* ranges ) {
	auto config = ImFontConfig( );
	config.FontDataOwnedByAtlas = false;

	for ( auto& sz : sizes ) {
		fonts.push_back( { sz, ImGui::GetIO( ).Fonts->AddFontFromFileTTF( file_path, sz * g_style->dpi_scale, &config, ranges ) } );
	}
}
ImFont* c_font::get( float size ) {
	auto it = std::find_if( fonts.begin( ), fonts.end( ), [&]( const font& a ) {
		return a.sz == size;
	} );

	if ( it == fonts.end( ) ) {
		return nullptr;
	}

	return it->font;
}

bool c_widgets::button( const ptext& label, ImVec2 size, c_buttonstyle style ) 
{
	struct s {
		float anim = 0;
		float hover = 0;
		float held = 0;
	}; auto& obj = anim_obj( label.str.data( ), 0, s{ } );

	auto window = GetCurrentWindow( );
	auto id = window->GetID( label.str.data( ) );
	ImRect bb{ window->DC.CursorPos, window->DC.CursorPos + CalcItemSize( size, CalcItemWidth( ), GetFrameHeight( ) ) };
	ItemSize( bb );
	ItemAdd( bb, id );

	bool hovered, held;
	bool pressed = ButtonBehavior( bb, id, &hovered, &held );

	obj.hover = anim( obj.hover, 0.f, 1.f, hovered );
	obj.held = anim( obj.held, 0.f, 1.f, held );
	obj.anim = anim( obj.anim, 0.f, 1.f, hovered || held );

	if ( style.rounding == -1 ) style.rounding = GImGui->Style.FrameRounding;
	if ( style.iconsize == 0 ) style.iconsize = 16;

	switch ( style.style ) {
	case button_filled: {
		if ( !style.color ) {
			style.color = g_style->col( pcol_scheme );
		} if ( !style.textcolor ) {
			style.textcolor = g_style->col( pcol_bg2, 2.f );
		} if ( !style.iconcolor ) {
			style.iconcolor = g_style->col( pcol_bg2, 2.f );
		} if ( !style.border ) {
			style.border = g_style->col( pcol_border );
		}

		auto col = col_anim( col_anim( style.color, style.color.lighten( style.lightenval ), obj.hover ), style.color.lighten( pow( style.lightenval, 2 ) ), obj.held );

		if ( style.background )
			window->DrawList->AddRectFilled( bb.Min, bb.Max, col, style.rounding );
	} break;
	case button_outline: {
		if ( !style.textcolor ) {
			style.textcolor = col_anim( g_style->col( pcol_text2 ), g_style->col( pcol_text3 ), obj.hover );
		} if ( !style.iconcolor ) {
			style.iconcolor = style.textcolor;
		} if ( !style.border ) {
			style.border = g_style->col( pcol_border );
		}

		auto col = col_anim( col_anim( style.color, style.color.lighten( 0.8f ), obj.hover ), style.color.lighten( 0.6f ), obj.held );
		window->DrawList->AddRect( bb.Min, bb.Max, g_style->col( pcol_border ), style.rounding );
	} break;
	}

	float spacing = style.iconsize + 8;

	if ( style.color == g_style->col( pcol_scheme ) && style.background ) {
		style.textcolor = style.iconcolor = g_style->col( pcol_textonscheme );
	}

	if ( CalcTextSize( label.str.data( ), 0, 1 ).x > 0 )
		window->DrawList->AddText( bb.GetCenter( ) - CalcTextSize( label.translate( ).data( ), 0, 1 ) / 2 + vec2{ spacing / 2, 0 } * bool( style.icon != "" ), style.textcolor, label.translate( ).data( ), FindRenderedTextEnd( label.translate( ).data( ) ) );
	
	if ( style.icon.data( ) )
		window->DrawList->AddText( fonts[icons].get( style.iconsize ), fonts[icons].get( style.iconsize )->FontSize, bb.GetCenter( ) - ImVec2{ CalcTextSize( label.translate( ).data( ), 0, 1 ).x / 2, style.iconsize / 2 } - vec2{ spacing / 2, 0 } + vec2{ 4.f * bool( CalcTextSize( label.str.data( ), 0, 1 ).x == 0 ), 0 }, style.iconcolor, style.icon.data( ) );

	return pressed;
}

bool c_widgets::iconbutton( const char* str_id, const char* icon )
{
	struct s {
		float anim = 0;
		float hover = 0;
		float held = 0;
	}; auto& obj = anim_obj( str_id, 0, s{ } );

	auto window = GetCurrentWindow( );
	auto id = window->GetID( str_id );
	ImRect bb{ window->DC.CursorPos, window->DC.CursorPos + vec2{ 16, 16 } };
	ItemSize( bb );
	ItemAdd( bb, id );

	bool hovered, held;
	bool pressed = ButtonBehavior( bb, id, &hovered, &held );

	obj.hover = anim( obj.hover, 0.f, 1.f, hovered );
	obj.held = anim( obj.held, 0.f, 1.f, held );
	obj.anim = anim( obj.anim, 0.f, 1.f, hovered || held );

	auto col = col_anim( col_anim( g_style->col( pcol_text2 ), g_style->col( pcol_text3 ), obj.hover ), g_style->col( pcol_text3, 0.6f ), obj.held );

	g_draw->text( icons, 16, bb.Min, col, icon );

	return pressed;
}

template < typename T >
bool c_widgets::slider( const ptext& label, T* v, T min, T max, const char* format, c_sliderstyle style ) 
{
	struct s {
		float anim;
		float hover;
		float held;
		float val_anim;
	}; auto& obj = anim_obj( label.str.data( ), 0, s{ } );
	
	bool result = false;

	char max_buf[32];
	ImFormatString( max_buf, sizeof( max_buf ), format, max );

	auto window = GetCurrentWindow( );
	auto id = window->GetID( label.str.data( ) );
	ImRect total_bb = ImRect{ window->DC.CursorPos, window->DC.CursorPos + ImVec2{ CalcItemWidth( ), GImGui->FontSize } };
	ImRect bb = ImRect{ { total_bb.Max.x - 100 ds, total_bb.GetCenter( ).y - 2 ds }, { total_bb.Max.x, total_bb.GetCenter( ).y + 2 ds } };
	ItemSize( total_bb );
	ItemAdd( total_bb, id );

	auto data = GImGui->LastItemData;

	bool hovered, held;
	bool pressed = ButtonBehavior( bb, id, &hovered, &held );

	obj.hover = anim( obj.hover, 0.f, 1.f, hovered );
	obj.held = anim( obj.held, 0.f, 1.f, held );
	obj.anim = anim( obj.anim, 0.f, 1.f, hovered || held );
	obj.val_anim = ImLerp( obj.val_anim, ( ImClamp( *v, min, max ) - min * 1.f ) / ( max - min ) * bb.GetWidth( ), GetIO( ).DeltaTime * 17 );

	if ( held ) {
		*v = ImClamp( T( min + ( GetIO( ).MousePos.x - bb.Min.x ) / bb.GetWidth( ) * ( max - min ) ), min, max );
		result = true;
	}

	GetWindowDrawList()->AddRectFilled(bb.Min, bb.Max, g_style->col(pcol_bg3), 99.f);
	// fill
	GetWindowDrawList()->AddRectFilled(bb.Min, bb.Min + ImVec2{ obj.val_anim, bb.GetHeight() },
		g_style->col(pcol_scheme).lighten(1.f - 0.3f * obj.held), 99.f);
	// thumb — smaller, clean dot
	window->DrawList->AddCircleFilled(
		{ bb.Min.x + obj.val_anim, bb.GetCenter().y },
		3.5f ds + (1.f ds) * obj.hover,
		g_style->col(pcol_scheme), 36
	);
	// subtle glow ring on thumb
	window->DrawList->AddCircle(
		{ bb.Min.x + obj.val_anim, bb.GetCenter().y },
		5.5f ds * obj.hover,
		g_style->col(pcol_scheme).alpha(0.25f * obj.hover), 36
	);

	window->DrawList->AddText( total_bb.Min, g_style->col( pcol_text ), label.translate( ).data( ), FindRenderedTextEnd( label.translate( ).data( ) ) );

	char buf[32];
	ImFormatString( buf, sizeof( buf ), format, *v );
	auto col = col_anim( g_style->col( pcol_text2 ), g_style->col( pcol_text ), obj.anim );
	window->DrawList->AddText( { bb.Min.x - 45 ds, total_bb.Min.y }, col, buf );

	GImGui->LastItemData = data;

	return result;
}
bool c_widgets::sliderint( const ptext& label, int* v, int min, int max, const char* format ) 
{
	g_search->add_item( { label.str, [=] { sliderint( label, v, min, max, format ); } } );

	bool result = slider( label, v, min, max, format );
	return result;
}
bool c_widgets::sliderfloat( const ptext& label, float* v, float min, float max, const char* format ) 
{
	g_search->add_item( { label.str, [=] { sliderfloat( label, v, min, max, format ); } } );

	bool result = slider( label, v, min, max, format );
	return result;
}
bool c_widgets::checkbox( const ptext& label, bool* v, c_checkboxstyle style ) 
{
	g_search->add_item( { label.str, [=] { checkbox( label, v, style ); } } );

	struct s {
		float anim;
		float hover;
		float enabled;
		float w;
	}; auto& obj = anim_obj( label.str.data( ), 0, s{ } );

	auto window = GetCurrentWindow( );
	auto id = window->GetID( label.str.data( ) );

	style.square_sz *= g_style->dpi_scale;

	float text_width = label.str.empty( ) ? 0 : CalcTextSize( label.translate( ).data( ) ).x;
	float total_width = style.square_sz + ( text_width > 0 ? GImGui->Style.ItemInnerSpacing.x + text_width : 0 );

	ImRect total_bb = ImRect{ window->DC.CursorPos, window->DC.CursorPos + ImVec2{ total_width, style.square_sz } };
	ImRect bb = ImRect{ total_bb.Min, total_bb.Min + ImVec2{ style.square_sz, style.square_sz } };

	ItemSize( total_bb );
	ItemAdd( total_bb, id );

	bool hovered, held;
	bool pressed = ButtonBehavior( total_bb, id, &hovered, &held );

	obj.hover = anim( obj.hover, 0.f, 1.f, hovered );
	obj.enabled = anim( obj.enabled, 0.f, 1.f, *v );
	obj.anim = anim( obj.anim, 0.f, 1.f, hovered || *v );

	if ( pressed ) {
		if ( ( !( style.tooltipcolor == g_style->col( pcol_scheme ).h( 0 ) ) || style.tooltipicon == 0 ) || *v ) {
			*v = !*v;
		} else {
			modal.add( [=] {
				vec2 size{ 280, 120 };
				SetCursorPos( GetWindowSize( ) / 2 - size / 2 );
				PushStyleVar( ImGuiStyleVar_WindowPadding, vec2{ 14, 14 } );
				PushStyleVar( ImGuiStyleVar_ChildBorderSize, 1 );
				BeginChild( "warning", size, ImGuiChildFlags_Border );
				{
					PushFont( fonts[icons].get( 16 ) );
					TextColored( g_style->col( pcol_scheme ).h( 0.f ), I_ALERT__TRIANGLE );
					PopFont( );
					SameLine( 0, 26 ds );
					Text( "Are you sure u want to enable this?" );
					TextDisabled( "This function is dangerous!" );

					if ( button( "YES", { 106 ds, GetWindowHeight( ) - GetCursorPosY( ) - GImGui->Style.WindowPadding.y }, { .icon = I_CHECK, .textcolor = g_style->col( pcol_checkboxdot ), .iconcolor = g_style->col( pcol_checkboxdot ) } ) ) {
						*v = !*v;
						modal.close( );
					}

					SameLine( 0, 8 ds );

					if ( button( "NO", { GetWindowWidth( ) - GetCursorPosX( ) - GImGui->Style.WindowPadding.x, GetWindowHeight( ) - GetCursorPosY( ) - GImGui->Style.WindowPadding.y }, { .icon = I_X, .color = g_style->col( pcol_bg3 ), .textcolor = g_style->col( pcol_text ), .iconcolor = g_style->col( pcol_text ), .lightenval = 1.2f } ) ) {
						modal.close( );
					}
				}
				EndChild( );
				PopStyleVar( 2 );
			} );
		}
	}

	window->DrawList->AddRect(bb.Min, bb.Max, g_style->col(pcol_border), 99.f);
	// fill when enabled
	window->DrawList->AddRectFilled(bb.Min, bb.Max, g_style->col(pcol_scheme, obj.enabled), 99.f);
	// inner shadow when on
	window->DrawList->AddRect(bb.Min, bb.Max, pcolor{ 0,0,0,0.2f * obj.enabled }, 99.f);
	// checkmark
	RenderCheckMark(window->DrawList, bb.GetCenter() - vec2{ 4,4 },
		g_style->col(pcol_bg2, obj.enabled), 8 ds);
	RenderCheckMark( window->DrawList, bb.GetCenter( ) - vec2{ 4, 4 }, g_style->col( pcol_bg2, obj.enabled ), 8 ds );

	if ( style.label ) {
		auto col = col_anim( col_anim( g_style->col( pcol_text2 ), g_style->col( pcol_text3 ), obj.hover ), g_style->col( pcol_text ), obj.enabled );
		window->DrawList->AddText( bb.Min + ImVec2{ style.square_sz + GImGui->Style.ItemInnerSpacing.x, bb.GetHeight( ) / 2 - GImGui->FontSize / 2 }, col, label.translate( ).data( ), FindRenderedTextEnd( label.translate( ).data( ) ) );
	}

	if ( style.key ) {
		auto pos = window->DC.CursorPos;
		auto posprev = window->DC.CursorPosPrevLine;

		obj.w = ImLerp( obj.w, g_draw->textsize( font, 12, keys[*style.key] ).x, GetIO( ).DeltaTime * 17 );

		window->DC.CursorPos = ImVec2{ total_bb.Max.x - 16 ds - int( obj.w ), bb.GetCenter( ).y - 12 ds };

		char temp[64];
		ImFormatString( temp, sizeof( temp ), "##%s binder", label.str.data( ) );
		BeginChild( temp, { 16 ds + int( obj.w ), 24 ds }, 0, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse );
		binder( temp, style.key );
		EndChild( );

		window->DC.CursorPos = pos;
		window->DC.CursorPosPrevLine = posprev;
	}
	
	if ( style.options ) {
		auto pos = window->DC.CursorPos;
		auto posprev = window->DC.CursorPosPrevLine;

		window->DC.CursorPos = ImVec2{ total_bb.Max.x - 16 ds, bb.GetCenter( ).y - 8 ds };

		char temp[64];
		ImFormatString( temp, sizeof( temp ), "%s options", label.str.data( ) );
		BeginChild( temp, vec2{ 14, 14 }, 0, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse );
		optionsbtn( temp, style.options );
		EndChild( );

		window->DC.CursorPos = pos;
		window->DC.CursorPosPrevLine = posprev;
	}

	if ( !style.col.empty( ) ) {
		auto pos = window->DC.CursorPos;
		auto posprev = window->DC.CursorPosPrevLine;

		float offset = style.options ? 38 ds : 26 ds;
		window->DC.CursorPos = ImVec2{ total_bb.Max.x - ( 22 ds ) * style.col.size( ) - offset + 16 ds, bb.GetCenter( ).y - 7 ds };
		
		char temp[64];
		ImFormatString( temp, sizeof( temp ), "##%s col", label.str.data( ) );
		BeginChild( temp, vec2{ 22 * style.col.size( ) - 8.f, 14 }, 0, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse );
		PushItemFlag( ImGuiItemFlags_NoNav, true );
		for ( int i = 0; i < style.col.size( ); ++i ) {
			ImFormatString( temp, sizeof( temp ), "##%s col %d", label.str.data( ), i );
			coloredit( temp, style.col[i], vec2{ 14, 14 } );
			SameLine( 0, 8 ds );
		}
		PopItemFlag( );
		EndChild( );

		window->DC.CursorPos = pos;
		window->DC.CursorPosPrevLine = posprev;
	}

	if ( style.tooltipicon != 0 ) {
		auto pos = window->DC.CursorPos;
		auto posprev = window->DC.CursorPosPrevLine;

		window->DC.CursorPos = ImVec2{ total_bb.Min.x + CalcTextSize( label.translate( ).data( ), 0, 1 ).x + 18 ds + style.square_sz, bb.GetCenter( ).y - 8 ds };
		char temp[64];
		ImFormatString( temp, sizeof( temp ), "%s tooltip", label.str.data( ) );
		BeginChild( temp, vec2{ 16, 16 }, 0, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse );

		if ( style.tooltipcolor )
			g_style->push( pcol_text2, style.tooltipcolor );
		tooltipbtn( temp, style.tooltiptext, style.tooltipicon );
		if ( style.tooltipcolor )
			g_style->pop( );

		EndChild( );

		window->DC.CursorPos = pos;
		window->DC.CursorPosPrevLine = posprev;
	}

	return pressed;
}

void c_widgets::checkpicker( const ptext& label, bool* v, float col[4], std::function< void( ) > options )
{
	g_search->add_item( { label.str, [=, this] { this->checkpicker( label, v, col, options ); } } );

	auto window = GetCurrentWindow( );
	auto start_pos = window->DC.CursorPos;
	
	checkbox( label, v );
	
	float box_size = 16.0f ds;
	float text_width = label.str.empty( ) ? 0 : CalcTextSize( label.translate( ).data( ) ).x;
	float spacing = GImGui->Style.ItemInnerSpacing.x;
	
	window->DC.CursorPos = start_pos + ImVec2{ box_size + spacing + text_width + 15.0f ds, 0 };
	std::string picker_id = "##" + std::string( label.str ) + "_picker";
	coloredit( { picker_id }, col, { 16.0f ds, 16.0f ds } );
	
	if ( options ) {
		SameLine( );
		SetCursorPosX( GetWindowWidth( ) - 24 ds );
		std::string options_id = std::string( label.str ) + "_options";
		optionsbtn( options_id, options );
	}
}

bool c_widgets::themebutton( const char* str_id, int i, c_theme& theme, bool selected, ImColor col, bool editable )
{
	struct s {
		float anim;
		float hover;
		float selected;
		float open_anim;
		bool open = false;
	}; auto& obj = anim_obj( str_id, 0, s{ } );

	auto window = GetCurrentWindow( );
	auto id = window->GetID( str_id );
	ImRect bb{ window->DC.CursorPos, window->DC.CursorPos + vec2{ 16, 16 } };
	ItemSize( bb );
	ItemAdd( bb, id );

	bool hovered, held;
	bool pressed = ButtonBehavior( bb, id, &hovered, &held );

	obj.hover = anim( obj.hover, 0.f, 1.f, hovered );
	obj.selected = anim( obj.selected, 0.f, 1.f, selected );
	obj.anim = anim( obj.anim, 0.f, 1.f, hovered || selected );
	obj.open_anim = anim( obj.open_anim, 0.f, 1.f, obj.open );

	window->DrawList->AddCircleFilled( bb.GetCenter( ), 8 ds - ( 2 ds ) * obj.selected, col, 36 ds );
	window->DrawList->AddCircle( bb.GetCenter( ), ( 8 ds ) * pow( obj.selected, 3 ), g_style->col( pcol_scheme, obj.selected ), 36 ds );
	
	if ( IsItemClicked( 1 ) && editable ) {
		obj.open = true;
	}

	if ( popup.begin( str_id, obj.open_anim, bb.Min, vec2{ 10, 10 } ) ) {
		if ( ( !IsWindowHovered( ImGuiHoveredFlags_AnyWindow ) || ( GImGui->HoveredWindow && !strstr( GImGui->HoveredWindow->Name, "popup" ) ) || FindWindowDisplayIndex( GImGui->HoveredWindow ) < FindWindowDisplayIndex( GetCurrentWindow( ) ) ) && IsMouseClicked( 0 ) ) {
			obj.open = false;
		}

		PushItemWidth( 180 ds );
		
		coloredit( "Window bg", theme.colors[pcol_bg].f4( ) );
		coloredit( "Content bg", theme.colors[pcol_bg5].f4( ) );
		coloredit( "Child bg", theme.colors[pcol_bg2].f4( ) );
		coloredit( "Frame bg", theme.colors[pcol_bg3].f4( ) );
		coloredit( "Frame bg hover", theme.colors[pcol_bg4].f4( ) );
		coloredit( "Text", theme.colors[pcol_text].f4( ) );
		coloredit( "Text disabled", theme.colors[pcol_text2].f4( ) );
		coloredit( "Text hover", theme.colors[pcol_text3].f4( ) );
		coloredit( "Border", theme.colors[pcol_border].f4( ) );
		coloredit( "Separator", theme.colors[pcol_separator].f4( ) );
		coloredit( "Slider & Combo", theme.colors[pcol_bg6].f4( ) );
		coloredit( "Checkbox dot", theme.colors[pcol_checkboxdot].f4( ) );
		coloredit( "Text on scheme", theme.colors[pcol_textonscheme].f4( ) );
		coloredit( "Listbox", theme.colors[pcol_listbox].f4( ) );

		if ( button( "Delete", { 0, 0 }, { .icon = I_TRASH, .color = g_style->col( pcol_bg2 ), .textcolor = g_style->col( pcol_scheme ).h( 0 ), .iconcolor = g_style->col( pcol_scheme ).h( 0 ), .lightenval = 1.2f } ) ) {
			g_style->themes.erase( g_style->themes.begin( ) + i );
			g_style->theme = 0;
			obj.open = false;
		}

		PopItemWidth( );
		popup.end( );
	}

	return pressed;
}
void c_widgets::themeselector( const ptext& label )
{
	Text( label.translate( ).data( ) );
	SameLine( GetWindowWidth( ) - ( 20 ds ) * g_style->themes.size( ) + 4 ds - GImGui->Style.WindowPadding.x - 20 ds );
	BeginGroup( );
	{
		if ( button( "##add", vec2{ 16, 16 }, { .icon = I_PLUS, .iconsize = 12, .rounding = 100 ds, .color = g_style->col( pcol_bg3 ), .iconcolor = g_style->col( pcol_text ), .lightenval = 1.2f } ) ) {
			if ( g_style->themes.size( ) < 5 )
				g_style->themes.push_back( g_style->themes[g_style->theme] );
		}
		SameLine( 0, 4 ds );
		char temp[32];
		for ( int i = 0; i < g_style->themes.size( ); ++i ) {
			ImFormatString( temp, sizeof( temp ), "##%s %d", label.str.data( ), i );
			if ( themebutton( temp, i, g_style->themes[i], i == g_style->theme, g_style->themes[i].colors[pcol_bg3], i > 1 ) ) {
				g_style->theme = i;
			}
			SameLine( 0, 4 ds );
		}
	}
	EndGroup( );
}
void c_widgets::showWidgetTeams(int solo, int duos, int trios, int squads) {
	PushStyleVar(ImGuiStyleVar_WindowRounding, GImGui->Style.ChildRounding);
	PushStyleColor(ImGuiCol_WindowBg, g_style->col(pcol_bg, 0.9f).vec4());
	widgets->window.begin("Teams Info", { 0, 0 }, ImGuiWindowFlags_AlwaysAutoResize);
	{
		BeginChild("header", vec2{ 240, 32 });
		{
			g_draw->text(icons, 16, GetWindowPos() + vec2{ 12, 8 }, g_style->col(pcol_scheme), I_USERS);
			GetWindowDrawList()->AddText(GetWindowPos() + vec2{ 36, 8 }, g_style->col(pcol_text), "Teams Info");
		}
		EndChild();
		widgets->spacing(0);
		PushStyleVar(ImGuiStyleVar_WindowPadding, vec2{ 14, 14 });
		PushStyleVar(ImGuiStyleVar_ItemSpacing, vec2{ 10, 10 });
		BeginChild("list", { 0, 0 }, ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_NoBackground);
		{
			Text(("Solos: " + std::to_string(solo)).c_str());
			Text(("Duos: " + std::to_string(duos)).c_str());
			Text(("Trios: " + std::to_string(trios)).c_str());
			Text(("Squads: " + std::to_string(squads)).c_str());
		}
		EndChild();
		PopStyleVar(2);
	}
	widgets->window.end();
	PopStyleColor();
	PopStyleVar();

}
void c_widgets::showSpecators()
{
	PushStyleVar(ImGuiStyleVar_WindowRounding, GImGui->Style.ChildRounding);
	PushStyleColor(ImGuiCol_WindowBg, g_style->col(pcol_bg, 0.9f).vec4());

	widgets->window.begin("Spectators", { 0, 0 }, ImGuiWindowFlags_AlwaysAutoResize);
	{
		BeginChild("header", vec2{ 240, 32 });
		{
			g_draw->text(icons, 16, GetWindowPos() + vec2{ 12, 8 }, g_style->col(pcol_scheme), I_EYE);
			GetWindowDrawList()->AddText(GetWindowPos() + vec2{ 36, 8 }, g_style->col(pcol_text), "Spectators");
		}
		EndChild();

		widgets->spacing(0);

		PushStyleVar(ImGuiStyleVar_WindowPadding, vec2{ 14, 14 });
		PushStyleVar(ImGuiStyleVar_ItemSpacing, vec2{ 10, 10 });

		BeginChild("list", { 0, 0 }, ImGuiChildFlags_AlwaysUseWindowPadding |
			ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeY,
			ImGuiWindowFlags_NoBackground);
		{
			float paddingX = 12.0f; 
			float startX = GetWindowPos().x + paddingX;
			float offsetY = 0.0f;

			if (parent::Spectators.CurrentSpectatorNames.empty())
			{
				GetWindowDrawList()->AddText(vec2{ startX, GetWindowPos().y + offsetY },
					g_style->col(pcol_text),
					"No Spectators.");
			}
			else
			{
				for (const auto& name : parent::Spectators.CurrentSpectatorNames)
				{
					GetWindowDrawList()->AddText(vec2{ startX, GetWindowPos().y + offsetY },
						g_style->col(pcol_text),
						name.c_str());
					offsetY += 20.0f; 
				}
			}
		}
		EndChild();

		PopStyleVar(2);
	}
	widgets->window.end();

	PopStyleColor();
	PopStyleVar();
}
void c_widgets::showRadar()
{
	PushStyleVar(ImGuiStyleVar_WindowRounding, GImGui->Style.ChildRounding);
	PushStyleColor(ImGuiCol_WindowBg, g_style->col(pcol_bg, 0.9f).vec4());

	widgets->window.begin("Radar", { 0, 0 }, ImGuiWindowFlags_AlwaysAutoResize);
	{
		BeginChild("header", vec2{ 240, 32 });
		{
			g_draw->text(icons, 16, GetWindowPos() + vec2{ 12, 8 }, g_style->col(pcol_scheme), I_RADAR);
			GetWindowDrawList()->AddText(GetWindowPos() + vec2{ 36, 8 }, g_style->col(pcol_text), "Radar");
		}
		EndChild();

		widgets->spacing(0);

		PushStyleVar(ImGuiStyleVar_WindowPadding, vec2{ 14, 14 });
		PushStyleVar(ImGuiStyleVar_ItemSpacing, vec2{ 10, 10 });

		float radarRadius = static_cast<float>(parent::Settings.radar_size);

		BeginChild("RadarArea", ImVec2(radarRadius * 2.0f, radarRadius * 2.0f), true, ImGuiWindowFlags_NoScrollbar);
		{
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			ImVec2 topLeft = ImGui::GetCursorScreenPos();
			ImVec2 center = topLeft + ImVec2(radarRadius, radarRadius);

			drawList->AddCircleFilled(center, radarRadius, ImColor(0, 0, 0, 180), 64);
			for (int i = 1; i <= 3; ++i)
			{
				float ring = radarRadius * (i / 3.0f);
				drawList->AddCircle(center, ring, ImColor(255, 255, 255, 100), 64, 1.0f);
			}
			drawList->AddCircleFilled(center, 3.0f, ImColor(255, 0, 0, 255), 12);
			drawList->AddLine({ center.x - radarRadius, center.y }, { center.x + radarRadius, center.y }, ImColor(255, 255, 255, 120), 1.0f);
			drawList->AddLine({ center.x, center.y - radarRadius }, { center.x, center.y + radarRadius }, ImColor(255, 255, 255, 120), 1.0f);

			
		}
		EndChild();

		PopStyleVar(2);
	}
	widgets->window.end();
	PopStyleColor();
	PopStyleVar();
}
void c_widgets::folder( c_folder& item )
{
	BeginChild( item.name.data( ), { 180, 0 }, ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AutoResizeX, ImGuiWindowFlags_NoBackground );
	{
		if ( button( item.name, { CalcTextSize( item.name.data( ) ).x + 24 ds, GImGui->FontSize }, { .icon = I_FOLDER, .background = false, .textcolor = g_style->col( pcol_text ), .iconcolor = g_style->col( pcol_scheme ) } ) ) {
			item.open = !item.open;
		}

		SameLine( 0, 8 ds );
		PushFont( fonts[icons].get( 16 ) );
		TextDisabled( item.open ? I_CHEVRON__DOWN : I_CHEVRON__LEFT );
		PopFont( );

		if ( item.open ) {
			char temp[128];
			ImFormatString( temp, sizeof( temp ), "%s1", item.name.data( ) );
			PushStyleVar( ImGuiStyleVar_WindowPadding, vec2{ 24, 0 } );
			BeginChild( temp, { 0, 0 }, ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_AutoResizeX, ImGuiWindowFlags_NoBackground );
			{
				for ( auto& f : item.folders ) {
					folder( f );
				}

				for ( auto& f : item.files ) {
					if ( file( f ) ) {
						f.script.name = f.name;
						editor.script = &f.script;
					}
				}
			}
			EndChild( );
			PopStyleVar( );
		}
	}
	EndChild( );
}
bool c_widgets::file( c_file& item )
{
	return button( item.name, { CalcTextSize( item.name.data( ) ).x + 24 ds, GImGui->FontSize }, { .icon = I_FILE, .background = false, .textcolor = g_style->col( pcol_text ), .iconcolor = g_style->col( pcol_scheme ) } );
}
bool c_widgets::coloredit( const ptext& label, float* col, ImVec2 size )
{
	g_search->add_item( { label.str, [=, this] { this->coloredit( label, col, size ); } } );

	struct s {
		float anim;
		float hover;
	}; auto& obj = anim_obj( label.str.data( ), 0, s{ } );

	if ( size == ImVec2{ 0, 0 } ) size = vec2{ 45, 12 };

	auto window = GetCurrentWindow( );
	auto id = window->GetID( label.str.data( ) );
	ImRect total_bb{ window->DC.CursorPos, window->DC.CursorPos + ImVec2{ CalcTextSize( label.str.data( ), 0, 1 ).x > 0 ? CalcItemWidth( ) : size.x, size.y } };
	float right_edge = window->WorkRect.Max.x;
	ImRect bb{ { right_edge - size.x - 26, total_bb.GetCenter( ).y - size.y / 2 }, { right_edge - 26, total_bb.GetCenter( ).y + size.y / 2 } };
	ItemSize( total_bb );
	ItemAdd( total_bb, id );

	ImGuiContext& g = *GImGui;
	bool hovered = IsItemHovered( );
	bool pressed = false;

	obj.hover = anim( obj.hover, 0.f, 1.f, hovered );

	if ( CalcTextSize( label.str.data( ), 0, 1 ).x > 0 )
		window->DrawList->AddText( { total_bb.Min.x, total_bb.GetCenter( ).y - GImGui->FontSize / 2 }, GetColorU32( ImGuiCol_Text ), label.translate( ).data( ), FindRenderedTextEnd( label.translate( ).data( ) ) );

	char temp[64];
	ImFormatString( temp, sizeof( temp ), "##%s picker", label.str.data( ) );
	
	auto pos = window->DC.CursorPos;
	window->DC.CursorPos = bb.Min;
	SetNextItemWidth( size.x );
	ColorEdit4( temp, col, ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoTooltip );
	window->DC.CursorPos = pos;

	return pressed;
}

bool c_widgets::optionsbtn( const std::string_view& str_id, std::function< void( ) > options, ImVec2 windowpos, vec2 padding )
{
	struct s {
		float anim;
		float hover;
		float open_anim;
		bool open = false;
		bool closed = false;
	}; auto& obj = anim_obj( str_id.data( ), 0, s{ } );

	auto window = GetCurrentWindow( );
	auto id = window->GetID( str_id.data( ) );
	ImRect bb = ImRect{ window->DC.CursorPos, window->DC.CursorPos + vec2{ 14, 14 } };

	ItemSize( bb );
	ItemAdd( bb, id );

	bool hovered, held;
	bool pressed = ButtonBehavior( bb, id, &hovered, &held );

	obj.hover = anim( obj.hover, 0.f, 1.f, hovered );
	obj.open_anim = anim( obj.open_anim, 0.f, 1.f, obj.open );
	obj.anim = anim( obj.anim, 0.f, 1.f, hovered || obj.open );

	if ( pressed && !obj.closed ) {
		obj.open = true;
	}

	if ( !IsMouseDown( 0 ) ) {
		obj.closed = false;
	}

	auto col = col_anim( col_anim( g_style->col( pcol_text2 ), g_style->col( pcol_text3 ), obj.hover ), g_style->col( pcol_text ), obj.open_anim );
	g_draw->text( icons, 14, bb.Min, pcolor{ col }, I_SETTINGS );

	if ( windowpos == ImVec2{ 0, 0 } ) {
		windowpos = bb.Min;
	}

	if ( popup.begin( str_id, obj.open_anim, windowpos, padding ) ) {
		if ( ( !IsWindowHovered( ImGuiHoveredFlags_AnyWindow ) || ( GImGui->HoveredWindow && !strstr( GImGui->HoveredWindow->Name, "popup" ) ) ) && IsMouseClicked( 0 ) ) {
			obj.open = false;
			obj.closed = true;
		}

		PushItemWidth( 260 ds );
		options( );
		PopItemWidth( );
		popup.end( );
	}

	return pressed;
}
bool c_widgets::tooltipbtn( const std::string_view& str_id, const ptext& tooltip, const char* icon )
{
	struct s {
		float anim;
		float hover;
	}; auto& obj = anim_obj( str_id.data( ), 0, s{ } );

	auto window = GetCurrentWindow( );
	auto id = window->GetID( str_id.data( ) );
	ImRect bb = ImRect{ window->DC.CursorPos, window->DC.CursorPos + vec2{ 16, 16 } };

	ItemSize( bb );
	ItemAdd( bb, id );

	bool hovered, held;
	bool pressed = ButtonBehavior( bb, id, &hovered, &held );

	obj.hover = anim( obj.hover, 0.f, 1.f, hovered );

	auto col = col_anim( g_style->col( pcol_text2 ), g_style->col( pcol_text3 ), obj.hover );
	g_draw->text( icons, 16, bb.Min, pcolor{ col }, icon );

	if ( obj.hover > 0.05f ) {
		GetForegroundDrawList( )->AddRectFilled( { bb.Min.x, bb.Max.y + 4 ds }, ImVec2{ bb.Min.x, bb.Max.y } + vec2{ 16, 20 } + CalcTextSize( tooltip.translate( ).data( ) ), g_style->col( pcol_bg3, obj.hover ), 3 ds );
		GetForegroundDrawList( )->AddRect( { bb.Min.x, bb.Max.y + 4 ds }, ImVec2{ bb.Min.x, bb.Max.y } + vec2{ 16, 20 } + CalcTextSize( tooltip.translate( ).data( ) ), g_style->col( pcol_border, obj.hover ), 3 ds );
		GetForegroundDrawList( )->AddText( ImVec2{ bb.Min.x, bb.Max.y } + vec2{ 8, 12 }, g_style->col( pcol_text, obj.hover ), tooltip.translate( ).data( ) );
	}

	return pressed;
}
bool c_widgets::comboex( const ptext& label, const std::string_view& preview, bool should_close, c_combostyle style ) 
{
	struct s {
		float anim;
		float hover;
		float held;
		float open_anim;
		bool open;
		float w;
	}; auto& obj = anim_obj( label.str.data( ), 0, s{ } );
	
	bool result = false;

	obj.w = ImLerp( obj.w, g_draw->textsize( font, 12, preview.data( ) ).x, GetIO( ).DeltaTime * 17 );

	style.height *= g_style->dpi_scale;
	style.rounding *= g_style->dpi_scale;
	style.xpad *= g_style->dpi_scale;

	auto window = GetCurrentWindow( );
	auto id = window->GetID( label.str.data( ) );
	ImRect total_bb = ImRect{ window->DC.CursorPos, window->DC.CursorPos + ImVec2{ CalcItemWidth( ), style.height } };
	ImRect bb = ImRect{ { total_bb.Max.x - 36 ds - int( obj.w ), total_bb.Min.y }, total_bb.Max };

	ItemSize( bb );
	ItemAdd( bb, id );

	bool hovered, held;
	bool pressed = ButtonBehavior( bb, id, &hovered, &held );

	if ( pressed ) {
		obj.open = !obj.open;
	}

	obj.hover = anim( obj.hover, 0.f, 1.f, hovered );
	obj.held = anim( obj.held, 0.f, 1.f, held );
	obj.anim = anim( obj.anim, 0.f, 1.f, obj.open || hovered );
	obj.open_anim = anim( obj.open_anim, 0.f, 1.f, obj.open );

	auto col = col_anim( g_style->col( pcol_bg3 ), g_style->col( pcol_bg4 ), obj.anim );
	auto arrowcol = col_anim( col_anim( g_style->col( pcol_text2 ), g_style->col( pcol_text3 ), obj.hover ), g_style->col( pcol_scheme ), obj.open ); 

	window->DrawList->AddRectFilled( bb.Min, bb.Max, col, style.rounding );
	g_draw->text( font, 12, bb.Min + vec2{ 8, 6 }, g_style->col( pcol_text ), preview.data( ) );
	window->DrawList->AddText( { total_bb.Min.x, total_bb.GetCenter( ).y - GImGui->FontSize / 2 }, g_style->col( pcol_text ), label.translate( ).data( ), FindRenderedTextEnd( label.translate( ).data( ) ) );

	if ( style.rotate ) g_draw->rotatestart( );
	GetWindowDrawList( )->AddText( fonts[icons].get( 12 ), fonts[icons].get( 12 )->FontSize, { bb.Max.x - 20 ds, bb.GetCenter( ).y - fonts[icons].get( 12 )->FontSize / 2 }, arrowcol, style.icon );
	if ( style.rotate ) g_draw->rotateend( IM_PI / 2 - IM_PI * obj.open_anim, g_draw->rotationcenter( ) );

	if ( obj.open_anim > 0.05f ) {
		SetNextWindowPos( { bb.Min.x, bb.Max.y + 4 ds } );
		char temp[64];
		ImFormatString( temp, sizeof( temp ), "%s popup", label.str.data( ) );
		PushStyleVar( ImGuiStyleVar_WindowPadding, vec2{ 0, 4 } );
		PushStyleVar( ImGuiStyleVar_WindowRounding, style.rounding );
		PushStyleVar( ImGuiStyleVar_WindowBorderSize, 1 );
		PushStyleVar( ImGuiStyleVar_ItemSpacing, vec2{ 0, 0 } );
		PushStyleVar( ImGuiStyleVar_Alpha, GImGui->Style.Alpha * obj.open_anim );
		PushStyleColor( ImGuiCol_WindowBg, g_style->col( pcol_bg3 ).vec4( ) );
		Begin( temp, 0, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize );
		SetWindowSize( { ImMax( GetCurrentWindow( )->ContentSize.x, bb.GetWidth( ) ), ImMin( GetCurrentWindow( )->ContentSize.y + GImGui->Style.WindowPadding.y * 2 + 2 ds, 200.f ds ) * obj.open_anim } );

		if ( GImGui->HoveredWindow && !strstr( GImGui->HoveredWindow->Name, "popup" ) && IsMouseClicked( 0 ) && !hovered || should_close ) {
			obj.open = false;
		}

		return true;
	}

	return false;
}
bool c_widgets::combo( const ptext& label, int* v, std::vector< std::string_view > items ) 
{
	g_search->add_item( { label.str, [=] { combo( label, v, items ); } } );

	struct s {
		bool should_close;
	}; auto& obj = anim_obj( label.str.data( ), 0, s{ } );
	float maxw = 0;
	for ( int i = 0; i < items.size( ); ++i ) {
		maxw = ImMax( CalcTextSize( items[i].data( ), 0, 1 ).x + 48 ds, maxw );
	}

	if ( comboex( label, items[*v], obj.should_close ) ) {
		if ( obj.should_close ) obj.should_close = false;

		for ( int i = 0; i < items.size( ); ++i ) {
			if ( selectable( items[i], *v == i, { ImMax( maxw, GetWindowWidth( ) - GImGui->Style.WindowPadding.x * 2 ), 28 ds } ) ) {
				*v = i;
				obj.should_close = true;
			}
		}

		End( );
		PopStyleColor( );
		PopStyleVar( 5 );
	}

	return obj.should_close;
}
bool c_widgets::multicombo( const ptext& label, bool* v, std::vector< std::string_view > items ) 
{
	g_search->add_item( { label.str, [=] { multicombo( label, v, items ); } } );

	auto& style = GetStyle( );

    std::string buf;

    buf.clear( );
    for ( size_t i = 0; i < items.size( ); ++i ) {
        if ( v[i] ) {
			buf += ptext{ items[i] }.translate( );
            buf += ", ";
        }
    }

    if ( !buf.empty( ) ) {
        buf.resize( buf.size( ) - 2 );
    }

	float maxw = 0;
	for ( int i = 0; i < items.size( ); ++i ) {
		maxw = ImMax( CalcTextSize( items[i].data( ), 0, 1 ).x + 48 ds, maxw );
	}

    if ( CalcTextSize( buf.c_str( ) ).x > 100 ds ) {
        for ( int i = 0; i < buf.size( ) - 1; ++i ) {
            if ( CalcTextSize( buf.substr( 0, i + 1 ).c_str( ) ).x > 100 ds ) {
                buf.resize( i );
                if ( buf[buf.size( ) - 1] == ',' ) {
                    buf.resize( buf.size( ) - 1 );
                }
                buf.append( ".." );
            }
        }
    }

	if ( buf == "" ) { 
		buf = "None";
	}

	bool result = false;
	if ( comboex( label, buf.c_str( ) ) ) {
		for ( int i = 0; i < items.size( ); ++i ) {
			if ( selectable( items[i], v[i], { ImMax( maxw, GetWindowWidth( ) ), 28 ds } ) ) {
				v[i] = !v[i];
				result = true;
			}
		}

		End( );
		PopStyleColor( );
		PopStyleVar( 5 );
	}

	return result;
}
bool c_widgets::binder( const ptext& label, int* key )
{
	g_search->add_item( { label.str, [=] { binder( label, key ); } } );

	struct s {
		float anim;
		float hover;
		float active_anim;
		float w;
		bool active = false;
	}; auto& obj = anim_obj( label.str.data( ), 0, s{ } );
	std::string_view buf = obj.active ? "..." : keys[*key];
	obj.w = ImLerp( obj.w, g_draw->textsize( font, 12, buf.data( ) ).x, GetIO( ).DeltaTime * 17 );

	auto window = GetCurrentWindow( );
	auto id = window->GetID( label.str.data( ) );
	ImRect total_bb{ window->DC.CursorPos, window->DC.CursorPos + ImVec2{ CalcTextSize( label.str.data( ), 0, 1 ).x > 0 ? CalcItemWidth( ) : ( 16.f ds + ( int )obj.w ), 24 ds } };
	ImRect bb{ total_bb.Max - ImVec2{ 16.f ds + ( int )obj.w, total_bb.GetHeight( ) }, total_bb.Max };
	ItemSize( total_bb );
	ItemAdd( total_bb, id );

	bool hovered, held;
	bool pressed = ButtonBehavior( bb, id, &hovered, &held );
	bool value_changed = false;

	if ( pressed ) {
		obj.active = true;
		*key = 7;
	}

	if ( obj.active ) {
		for ( auto i = 0; i < 5; i++ ) {
			if ( GetIO( ).MouseDown[i] ) {
				switch ( i ) {
				case 0:
					*key = VK_LBUTTON;
					break;
				case 1:
					*key = VK_RBUTTON;
					break;
				case 2:
					*key = VK_MBUTTON;
					break;
				case 3:
					*key = VK_XBUTTON1;
					break;
				case 4:
					*key = VK_XBUTTON2;
				}
				value_changed = true;
				obj.active = false;
			}
		}

		if ( !value_changed ) {
			for ( auto i = VK_BACK; i <= VK_RMENU; i++ ) {
				if ( GetAsyncKeyState( i ) & 0x8000 ) {
					*key = i == VK_ESCAPE ? 0 : i;
					value_changed = true;
					obj.active = false;
				}
			}
		}
	}

	obj.hover = anim( obj.hover, 0.f, 1.f, hovered );
	obj.active_anim = anim( obj.active_anim, 0.f, 1.f, obj.active );
	obj.anim = anim( obj.anim, 0.f, 1.f, hovered || obj.active );

	window->DrawList->AddRectFilled( bb.Min, bb.Max, g_style->col( pcol_bg3 ), 3 ds );
	g_draw->text( font, 12, { bb.Min.x + 8 ds, bb.GetCenter( ).y - 6 ds }, g_style->col( pcol_text ), buf.data( ) );

	window->DrawList->AddText( { total_bb.Min.x, total_bb.GetCenter( ).y - GImGui->FontSize / 2 }, g_style->col( pcol_text ), label.translate( ).data( ), FindRenderedTextEnd( label.translate( ).data( ) ) );

	return value_changed;
}
bool c_widgets::selector( const ptext& label, int* v, const std::vector< std::string_view >& items )
{
	struct s {
		ImVec2 size;
	}; auto& obj = anim_obj( label.str.data( ), 0, s{ } );

	bool result = false;

	GetWindowDrawList( )->AddText( GetCurrentWindow( )->DC.CursorPos + ImVec2{ 0, obj.size.y / 2 - GImGui->FontSize / 2 }, g_style->col( pcol_text ), label.translate( ).data( ) );
	Dummy( CalcTextSize( label.translate( ).data( ) ) );
	SameLine( CalcItemWidth( ) - obj.size.x + GImGui->Style.WindowPadding.x );
	PushStyleColor( ImGuiCol_ChildBg, g_style->col( pcol_bg3 ).vec4( ) );
	PushStyleVar( ImGuiStyleVar_WindowPadding, vec2{ 2, 2 } );
	PushStyleVar( ImGuiStyleVar_ChildRounding, 3 ds );
	BeginChild( label.str.data( ), { 0, 0 }, ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Border );
	{
		obj.size = GetWindowSize( );

		for ( int i = 0; i < items.size( ); ++i ) {
			if ( selectable( items[i], *v == i, { 0, CalcTextSize( ptext{ items[i] }.translate( ).data( ) ).y + 4 ds }, { .xpad = 4, .rounding = 2 } ) ) {
				*v = i;
				result = true;
			}

			if ( i < items.size( ) - 1 )
				SameLine( 0, 2 ds );
		}
	}
	EndChild( );
	PopStyleVar( 2 );
	PopStyleColor( );

	return result;
}
bool c_widgets::selectable( const ptext& label, bool selected, ImVec2 size, c_selectablestyle style )
{
	struct s {
		float anim;
		float hover;
		float selected;
	}; auto& obj = anim_obj( label.str.data( ), 0, s{ } );

	style.rounding *= g_style->dpi_scale;
	style.xpad *= g_style->dpi_scale;

	auto window = GetCurrentWindow( );
	auto id = window->GetID( label.str.data( ) );
	ImRect bb{ window->DC.CursorPos, window->DC.CursorPos + CalcItemSize( size, style.xpad * 2 + int( ( 22 ds ) * obj.selected ) + 4 ds + g_draw->textsize( font, 14, label.translate( ).data( ) ).x, GetFrameHeight( ) ) };
	ItemSize( bb );
	ItemAdd( bb, id );

	bool hovered, held;
	bool pressed = ButtonBehavior( bb, id, &hovered, &held );

	obj.hover = anim( obj.hover, 0.f, 1.f, hovered );
	obj.selected = anim( obj.selected, 0.f, 1.f, selected );
	obj.anim = anim( obj.anim, 0.f, 1.f, hovered || selected );

	auto col = col_anim( col_anim( g_style->col( pcol_text2 ), g_style->col( pcol_text3 ), obj.hover ), g_style->col( pcol_text ), obj.selected );

	GetWindowDrawList( )->AddRectFilled( bb.Min, bb.Max, g_style->col( pcol_bg6 ).alpha( obj.selected ), style.rounding );
	g_draw->text( icons, 14, { bb.Min.x + style.xpad, bb.GetCenter( ).y - g_draw->textsize( icons, 14, I_CHECK ).y / 2 }, g_style->col( pcol_scheme, obj.selected ), I_CHECK );
	GetWindowDrawList( )->AddText( { bb.Min.x + style.xpad + int( ( 22 ds ) * obj.selected ), bb.GetCenter( ).y - GImGui->FontSize / 2 }, col, label.translate( ).data( ), FindRenderedTextEnd( label.translate( ).data( ) ) );

	return pressed;
}

bool c_widgets::tab( const ptext& label, bool selected )
{
	struct s {
		float anim;
		float hover;
		float selected;
	}; auto& obj = anim_obj( label.str.data( ), 0, s{ } );

	auto window = GetCurrentWindow( );
	auto id = window->GetID( label.str.data( ) );
	ImRect bb{ window->DC.CursorPos, window->DC.CursorPos + CalcTextSize( label.translate( ).data( ), 0, 1 ) + vec2{ 24, 20 } };
	ItemSize( bb );
	ItemAdd( bb, id );

	bool hovered, held;
	bool pressed = ButtonBehavior( bb, id, &hovered, &held );

	obj.hover = anim( obj.hover, 0.f, 1.f, hovered );
	obj.selected = anim( obj.selected, 0.f, 1.f, selected );
	obj.anim = anim( obj.anim, 0.f, 1.f, hovered || selected );

	auto col = col_anim( col_anim( g_style->col( pcol_text2 ), g_style->col( pcol_text3 ), obj.hover ), g_style->col( pcol_text ), obj.selected );

	GetWindowDrawList( )->AddRectFilled( bb.Min, bb.Max, g_style->col( pcol_bg3 ).alpha( obj.selected ), 3 ds );
	GetWindowDrawList( )->AddText( bb.Min + vec2{ 12, 10 }, col, label.translate( ).data( ), FindRenderedTextEnd( label.translate( ).data( ) ) );

	return pressed;
}

void c_widgets::spacing( float px )
{
	SetCursorPosY( GetCursorPosY( ) - GImGui->Style.ItemSpacing.y + px );
}

void c_widgets::separator( bool vertical, float size )
{
	if ( vertical ) {
		if ( size == 0 ) size = GetWindowHeight( );

		GetWindowDrawList( )->AddRectFilled( { GetCurrentWindow( )->DC.CursorPos.x, GetCurrentWindow( )->Rect( ).GetCenter( ).y - size / 2 }, { GetCurrentWindow( )->DC.CursorPos.x + 1, GetCurrentWindow( )->Rect( ).GetCenter( ).y + size / 2 }, g_style->col( pcol_separator ) );
		Dummy( { 1, size } );
	} else {
		if ( size == 0 ) size = CalcItemWidth( );

		GetWindowDrawList( )->AddRectFilled( GetCurrentWindow( )->DC.CursorPos, GetCurrentWindow( )->DC.CursorPos + ImVec2{ size, 1 }, g_style->col( pcol_separator ) );
		Dummy( { size, 1 } );
	}	
}

bool c_widgets::textinput( const ptext& label, char* buf, size_t buf_size, c_textinputstyle style )
{
	bool result = false;

	if ( !style.bgcol ) {
		style.bgcol = g_style->col( pcol_bg3 );
	}

	BeginGroup( );
	if ( CalcTextSize( label.str.data( ), 0, 1 ).x > 0 ) {
		TextDisabled( label.translate( ).data( ) );
		spacing( 10 ds );
	}

	PushStyleVar( ImGuiStyleVar_FramePadding, style.padding );
	ImVec2 size{ CalcItemSize( style.size, CalcItemWidth( ), GetFrameHeight( ) ) };
	ImRect bb{ GetCurrentWindow( )->DC.CursorPos, GetCurrentWindow( )->DC.CursorPos + size };
	GetWindowDrawList( )->AddRectFilled( bb.Min, bb.Max, style.bgcol, style.rounding );

	if ( GImGui->Style.FrameBorderSize > 0 ) {
		GetWindowDrawList( )->AddRect( bb.Min, bb.Max, g_style->col( pcol_border ), style.rounding );
	}

	PushStyleColor( ImGuiCol_FrameBg, GetColorU32( ImGuiCol_FrameBg, 0 ) );
	char temp[64];
	ImFormatString( temp, sizeof( temp ), "##%s", label.str.data( ) );

	if ( style.icon ) {
		g_draw->text( icons, 16, bb.Min + style.padding, g_style->col( pcol_text2 ), style.icon );
		SetCursorPosX( GetCursorPosX( ) + style.padding.x + 16 ds );
	}

	PushStyleVar( ImGuiStyleVar_FrameBorderSize, 0 );
	result = InputTextEx( temp, style.hint.data( ), buf, buf_size, size - ImVec2{ ( style.padding.x + 14.f ) * bool( style.icon ), 0 }, style.flags );
	PopStyleColor( );
	PopStyleVar( 2 );
	EndGroup( );

	return result;
}

bool c_widgets::keybind( const char* label, int* key, bool show_label )
{
	struct s {
		float anim;
		float hover;
		float active_anim;
		float w;
		bool active = false;
	}; auto& obj = anim_obj( label, 0, s{ } );

	auto window = GetCurrentWindow( );
	auto id = window->GetID( label );
	const float width = CalcItemWidth( );
	const float height = 28.f ds;
	const ImRect rect( window->DC.CursorPos, window->DC.CursorPos + ImVec2( width, height ) );

	ItemSize( rect );
	if ( !ItemAdd( rect, id ) )
		return false;

	bool hovered, held;
	bool pressed = ButtonBehavior( rect, id, &hovered, &held );
	bool value_changed = false;

	if ( pressed ) {
		obj.active = true;
	}

	char buf_display[64] = "NONE";
	if ( obj.active )
		strcpy( buf_display, "..." );
	else if ( *key > 0 && *key < keys.size( ) )
		strcpy( buf_display, keys[*key] );

	obj.hover = anim( obj.hover, 0.f, 1.f, hovered );
	obj.active_anim = anim( obj.active_anim, 0.f, 1.f, obj.active );

	// Calculate text width for the background box
	float text_width = g_draw->textsize( font, 12, buf_display ).x;
	obj.w = ImLerp( obj.w, text_width + 26 ds, GetIO( ).DeltaTime * 17 );

	const float icon_space = 28.0f ds;
	ImVec2 rect_max = rect.Max;
	ImVec2 rect_min = ImVec2( rect_max.x - obj.w, rect.Min.y );
	ImVec2 full_rect_min = rect_min - ImVec2( icon_space, 0 );

	// Draw Background
	window->DrawList->AddRectFilled( full_rect_min, rect_max, g_style->col( pcol_bg3 ), 3 ds );
	// Draw Outline
	window->DrawList->AddRect( full_rect_min, rect_max, g_style->col( pcol_border ), 3 ds );
	// Draw Separator
	window->DrawList->AddLine( ImVec2( rect_min.x, rect_min.y + 3 ds ), ImVec2( rect_min.x, rect_max.y - 3 ds ), g_style->col( pcol_border ) );

	// Label
	if ( show_label ) {
		auto col = col_anim( g_style->col( pcol_text2 ), g_style->col( pcol_text ), obj.hover || obj.active );
		window->DrawList->AddText( rect.Min + ImVec2{ 0, height / 2 - GImGui->FontSize / 2 }, col, label, FindRenderedTextEnd( label ) );
	}

	// Value text
	window->DrawList->AddText( rect_min + ImVec2{ ( obj.w - text_width ) * 0.5f, height / 2 - GImGui->FontSize / 2 }, g_style->col( pcol_text ), buf_display );

	// Icon
	bool is_mouse_key = ( *key == VK_LBUTTON || *key == VK_RBUTTON || *key == VK_MBUTTON || *key == VK_XBUTTON1 || *key == VK_XBUTTON2 );
	ImVec2 icon_center = full_rect_min + ImVec2( icon_space * 0.5f - 4 ds, height * 0.5f );
	
	if ( is_mouse_key ) {
		// Draw mouse icon primitives (refined)
		float mw = 3.5f ds, mh = 5.5f ds;
		ImVec2 m_min = icon_center - ImVec2( mw, mh ) + ImVec2( 0, 1 ds );
		ImVec2 m_max = icon_center + ImVec2( mw, mh ) + ImVec2( 0, 1 ds );
		window->DrawList->AddRect( m_min, m_max, g_style->col( pcol_scheme ), 2.0f ds, 0, 1.2f ds );
		window->DrawList->AddLine( icon_center - ImVec2( 0, mh - 3 ds ) + ImVec2( 0, 1 ds ), icon_center + ImVec2( 0, 1 ds ), g_style->col( pcol_scheme ), 1.0f ds );
		window->DrawList->AddCircleFilled( icon_center - ImVec2( 0, mh - 2 ds ) + ImVec2( 0, 1 ds ), 1.0f ds, g_style->col( pcol_scheme ) );
	} else {
		// Draw keyboard icon
		ImGui::PushFont( fonts[icons].get( 16 ds ) );
		ImVec2 icon_size = CalcTextSize( I_KEYBOARD );
		window->DrawList->AddText( icon_center - icon_size / 2 + ImVec2( 0, 1 ds ), g_style->col( pcol_scheme ), I_KEYBOARD );
		ImGui::PopFont( );
	}

	if ( obj.active ) {
		for ( auto i = 0; i < 5; i++ ) {
			if ( GetIO( ).MouseDown[i] ) {
				switch ( i ) {
				case 0: *key = VK_LBUTTON; break;
				case 1: *key = VK_RBUTTON; break;
				case 2: *key = VK_MBUTTON; break;
				case 3: *key = VK_XBUTTON1; break;
				case 4: *key = VK_XBUTTON2; break;
				}
				value_changed = true;
				obj.active = false;
			}
		}

		if ( !value_changed ) {
			for ( auto i = VK_BACK; i <= VK_RMENU; i++ ) {
				if ( GetAsyncKeyState( i ) & 0x8000 ) {
					if ( i == VK_ESCAPE ) {
						*key = 0;
						obj.active = false;
						value_changed = true;
						break;
					}
					*key = i;
					value_changed = true;
					obj.active = false;
				}
			}
		}
	}

	return value_changed;
}

///////////////////////////////////////////////////////////////////////////////


// WINDOW
void c_widgets::widget_window::begin( const std::string_view& name, ImVec2 size, ImGuiWindowFlags flags ) {
	SetNextWindowSize( size );
	Begin( name.data( ), 0, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus | flags );
}
void c_widgets::widget_window::end( ) {
	End( );
}

// CHILD
void c_widgets::widget_child::begin( const std::string_view& icon, const ptext& name, ImVec2 size, c_childstyle style ) {
	ImGuiChildFlags flags;
	if ( size.y == 0 ) flags = ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeY;
	float padding = GImGui->Style.WindowPadding.x;
	PushStyleVar( ImGuiStyleVar_WindowPadding, { 0, 0 } );
	PushStyleColor( ImGuiCol_ChildBg, g_style->col( pcol_bg6 ).vec4( ) );
	BeginChild( name.str.data( ), CalcItemSize( size, GetWindowWidth( ) / 2 - padding - GImGui->Style.ItemSpacing.x / 2, 0 ), ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Border, style.flags );
	PopStyleColor( );
	PopStyleVar( );

	char temp[64];
	ImFormatString( temp, sizeof( temp ), "%s header", name.str.data( ) );
	BeginChild( temp, { 0, 36 ds }, 0, ImGuiWindowFlags_NoBackground );
	{
		g_draw->text( icons, 16, GetWindowPos( ) + ImVec2{ style.padding.x, GetWindowHeight( ) / 2 - GImGui->FontSize / 2 }, g_style->col( pcol_scheme ), icon.data( ) );
		GetWindowDrawList( )->AddText( GetWindowPos( ) + ImVec2{ style.padding.x + 24 ds, GetWindowHeight( ) / 2 - GImGui->FontSize / 2 }, g_style->col( pcol_text ), name.translate( ).data( ), FindRenderedTextEnd( name.translate( ).data( ) ) );
	}
	EndChild( );
	widgets->spacing( 0 );

	ImFormatString( temp, sizeof( temp ), "%s content", name.str.data( ) );
	PushStyleVar( ImGuiStyleVar_WindowPadding, style.padding );
	PushStyleVar( ImGuiStyleVar_ItemSpacing, style.itemspacing );
	BeginChild( temp, { 0, size.y == 0 ? 0 : size.y - GetCursorPosY( ) }, ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar );
	PushItemWidth( GetWindowWidth( ) - GImGui->Style.WindowPadding.x * 2 );
	smoothscroll( true );
}
void c_widgets::widget_child::end( ) {
	PopItemWidth( );
	EndChild( );
	PopStyleVar( 2 );
	EndChild( );
}
float* c_widgets::widget_child::smoothscroll( bool scrollbar, ImVec2 padding )
{
	struct s {
		ImVec2 scroll;
		ImVec2 scroll_anim;
		bool scroll_activey = false;
		bool scroll_activex = false;
		float oldscroll;
		float clickpos;

		float xanim;
		float yanim;
	}; auto& obj = anim_obj( GetCurrentWindow( )->Name, 23123, s{ } );

	obj.scroll.y = ImClamp( obj.scroll.y, 0.f, GetCurrentWindow( )->ScrollMax.y );
	obj.scroll.x = ImClamp( obj.scroll.x, 0.f, GetCurrentWindow( )->ScrollMax.x );
	obj.scroll_anim.y = ImClamp( obj.scroll_anim.y, 0.f, GetCurrentWindow( )->ScrollMax.y );
	obj.scroll_anim.x = ImClamp( obj.scroll_anim.x, 0.f, GetCurrentWindow( )->ScrollMax.x );

	if ( scrollbar ) {
		// vertical
		{
			ImRect scrollbarbb{ GetWindowPos( ) + ImVec2{ GetWindowWidth( ) - 5 ds, 3 ds + padding.y }, GetWindowPos( ) + GetWindowSize( ) - vec2{ 2, 3 + padding.y } };
			float visiblepart = GetWindowHeight( ) / ( GetCurrentWindow( )->ContentSize.y + GImGui->Style.WindowPadding.y * 2 );

			if ( visiblepart < 1.f ) {
				float scrollh = scrollbarbb.GetHeight( ) * visiblepart;
				float invisiblepart = 1.f - visiblepart;
				float scrolloffset = ( obj.scroll.y / GetCurrentWindow( )->ScrollMax.y ) * ( scrollbarbb.GetHeight( ) - scrollh );

				obj.yanim = anim( obj.yanim, 0.f, 1.f, IsMouseHoveringRect( scrollbarbb.Min, scrollbarbb.Max ) || obj.scroll_activey );
				auto col = col_anim( g_style->col( pcol_bg3 ), g_style->col( pcol_text2 ), obj.yanim );

				GetWindowDrawList( )->PushClipRect( scrollbarbb.Min, scrollbarbb.Max );
				GetWindowDrawList( )->AddRectFilled( scrollbarbb.Min + ImVec2{ 0, scrolloffset }, { scrollbarbb.Max.x, scrollbarbb.Min.y + scrollh + scrolloffset }, col, 5 ds );
				GetWindowDrawList( )->PopClipRect( );

				if ( IsMouseClicked( 0 ) && IsMouseHoveringRect( scrollbarbb.Min, scrollbarbb.Max ) && !obj.scroll_activey ) {
					obj.scroll_activey = true;
					obj.oldscroll = obj.scroll.y;
					obj.clickpos = ( GetIO( ).MousePos.y - scrollbarbb.Min.y ) / scrollbarbb.GetHeight( );
				} if ( !IsMouseDown( 0 ) ) obj.scroll_activey = false;

				if ( obj.scroll_activey ) {
					GetCurrentWindow( )->Flags |= ImGuiWindowFlags_NoMove;

					float newscroll = ( GetIO( ).MousePos.y - scrollbarbb.Min.y ) / scrollbarbb.GetHeight( );
					float diff = ( newscroll - obj.clickpos ) / invisiblepart;

					obj.scroll.y = obj.oldscroll + diff * GetCurrentWindow( )->ScrollMax.y;
					obj.scroll.y = ImClamp( obj.scroll.y, 0.f, GetCurrentWindow( )->ScrollMax.y );
				}
			}
		}
		// horizontal
		{
			ImRect scrollbarbb{ GetWindowPos( ) + ImVec2{ 3 ds + padding.x ds, GetWindowHeight( ) - 5 ds }, GetWindowPos( ) + GetWindowSize( ) - vec2{ 3 + padding.x, 2 } };
			float visiblepart = GetWindowWidth( ) / ( GetCurrentWindow( )->ContentSize.x + GImGui->Style.WindowPadding.x * 2 );

			if ( visiblepart < 1.f ) {
				float scrollh = scrollbarbb.GetWidth( ) * visiblepart;
				float invisiblepart = 1.f - visiblepart;
				float scrolloffset = ( obj.scroll.x / GetCurrentWindow( )->ScrollMax.x ) * ( scrollbarbb.GetWidth( ) - scrollh );

				obj.xanim = anim( obj.xanim, 0.f, 1.f, IsMouseHoveringRect( scrollbarbb.Min, scrollbarbb.Max ) || obj.scroll_activex );
				auto col = col_anim( g_style->col( pcol_bg3 ), g_style->col( pcol_text2 ), obj.xanim );

				GetWindowDrawList( )->PushClipRect( scrollbarbb.Min, scrollbarbb.Max );
				GetWindowDrawList( )->AddRectFilled( scrollbarbb.Min + ImVec2{ scrolloffset, 0 }, { scrollbarbb.Min.x + scrollh + scrolloffset, scrollbarbb.Max.y }, col, 5 ds );
				GetWindowDrawList( )->PopClipRect( );

				if ( IsMouseClicked( 0 ) && IsMouseHoveringRect( scrollbarbb.Min, scrollbarbb.Max ) && !obj.scroll_activex ) {
					obj.scroll_activex = true;
					obj.oldscroll = obj.scroll.x;
					obj.clickpos = ( GetIO( ).MousePos.x - scrollbarbb.Min.x ) / scrollbarbb.GetWidth( );
				} if ( !IsMouseDown( 0 ) ) obj.scroll_activex = false;

				if ( obj.scroll_activex ) {
					GetCurrentWindow( )->Flags |= ImGuiWindowFlags_NoMove;

					float newscroll = ( GetIO( ).MousePos.x - scrollbarbb.Min.x ) / scrollbarbb.GetWidth( );
					float diff = ( newscroll - obj.clickpos ) / invisiblepart;

					obj.scroll.x = obj.oldscroll + diff * GetCurrentWindow( )->ScrollMax.x;
					obj.scroll.x = ImClamp( obj.scroll.x, 0.f, GetCurrentWindow( )->ScrollMax.x );
				}
			}
		}
	}

	GetCurrentWindow( )->Scroll.y = obj.scroll_anim.y < 0.5f ? 0.f : obj.scroll_anim.y > ( GetCurrentWindow( )->ScrollMax.y - 0.5f ) ? GetCurrentWindow( )->ScrollMax.y : ImLerp( GetCurrentWindow( )->Scroll.y, obj.scroll.y, GetIO( ).DeltaTime * 40 );
	GetCurrentWindow( )->Scroll.x = ImLerp( GetCurrentWindow( )->Scroll.x, obj.scroll.x, GetIO( ).DeltaTime * 40 );

	ImGuiWindow* wheeling_window = nullptr;
	if ( GImGui->HoveredWindow ) {
		if ( GImGui->HoveredWindow->Flags & ImGuiWindowFlags_ChildWindow ) {
			for ( ImGuiWindow* window = GImGui->HoveredWindow; window->Flags & ImGuiWindowFlags_ChildWindow; window = window->ParentWindow ) {
				if ( window->ScrollMax[ImGuiAxis_Y] == 0 )
					continue;

				wheeling_window = window;
			}
		} else {
			wheeling_window = GImGui->HoveredWindow;
		}
	}

	if ( wheeling_window == GetCurrentWindow( ) ) {
		if ( !IsKeyDown( ImGuiKey_LeftCtrl ) )
			obj.scroll.y = ImClamp( obj.scroll.y - GetIO( ).MouseWheel * 80, 0.f, GetCurrentWindow( )->ScrollMax.y );
		else
			obj.scroll.x = obj.scroll.x - GetIO( ).MouseWheel * 80;
	}

	obj.scroll_anim.y = ImLerp( obj.scroll_anim.y, obj.scroll.y, GetIO( ).DeltaTime * 40 );
	obj.scroll_anim.x = ImLerp( obj.scroll_anim.x, obj.scroll.x, GetIO( ).DeltaTime * 40 );

	return &obj.scroll.y;
}


// CONTENT
void c_widgets::widget_content::begin( const std::string_view& str_id, ImVec2 size ) {

}
void c_widgets::widget_content::end( ) {

}

// MENU
void c_widgets::widget_menu::begin( const std::string_view& str_id ) {

}
void c_widgets::widget_menu::end( ) {

}
bool c_widgets::widget_menu::button( const ptext& label, ImVec2 size, c_buttonstyle style ) {

}

// NOTIFY
void c_widgets::widget_notify::add( const std::string_view& title, const std::string_view& text, notify_ status )
{
	notifications.emplace_back( c_notify{ title, text, status, 3.f } );
}
void c_widgets::widget_notify::handle( )
{
	auto draw_list = GetBackgroundDrawList( );

	float offset = 0.f;
	for ( int i = 0; i < notifications.size( ); ++i ) {
		auto& n = notifications[i];
		float alpha = n.time <= n.fade_time ? n.time / n.fade_time : n.time >= n.duration - n.fade_time ? ( n.duration - n.time ) / n.fade_time : 1.f;

		ImVec2 size{ ImMax( CalcTextSize( n.message.data( ) ).x, CalcTextSize( n.title.data( ) ).x + 20 ds ) + 24 ds, 28 ds + GImGui->FontSize * 2 };

		if ( n.pos.x == 0 ) n.pos = GetIO( ).DisplaySize - ImVec2{ 0, 20 + offset + size.y };

		n.pos.x = ImLerp( n.pos.x, GetIO( ).DisplaySize.x - size.x - 20, GetIO( ).DeltaTime * 14 );
		n.pos.y = ImLerp( n.pos.y, GetIO( ).DisplaySize.y - 20 - offset - size.y, GetIO( ).DeltaTime * 14 );

		draw_list->AddRectFilled( n.pos, n.pos + size, g_style->col( pcol_bg4, alpha ), 3 ds, ImDrawFlags_RoundCornersTop );

		const char* n_icons[] = {
			I_CIRCLE__CHECK,
			I_ALERT__CIRCLE,
			I_CIRCLE__X,
			I_INFO__CIRCLE,
		};

		pcolor colors[] = {
			g_style->col( pcol_scheme ),
			g_style->col( pcol_scheme ).h( 0.14f ),
			g_style->col( pcol_scheme ).h( 0.f ),
			g_style->col( pcol_scheme ).h( 0.59f ),
		};

		g_draw->text( icons, 16, n.pos + ImVec2{ 12, 12 }, colors[n.status].alpha( alpha ), n_icons[n.status], false, draw_list );
		draw_list->AddText( n.pos + vec2{ 32, 12 }, colors[n.status].alpha( alpha ), n.title.data( ) );
		draw_list->AddText( n.pos + vec2{ 12, 16 } + ImVec2{ 0, GImGui->FontSize }, g_style->col( pcol_text, alpha ), n.message.data( ) );

		n.time += 1.f / GetIO( ).Framerate;
		offset += size.y + 12;
	}

	notifications.erase(
		std::remove_if( notifications.begin( ), notifications.end( ), []( const c_notify& n ) {
			return n.time >= n.duration;
		} ), 
		notifications.end( )
	);
}

// POPUP
bool c_widgets::widget_popup::begin( const std::string_view& str_id, float alpha, ImVec2 pos, ImVec2 padding )
{
	char temp[64];
	ImFormatString( temp, sizeof( temp ), "%s popup", str_id.data( ) );

	if ( alpha > 0.05f ) {
		if ( pos != ImVec2{ 0, 0 } )
			SetNextWindowPos( pos );
		PushStyleVar( ImGuiStyleVar_Alpha, GImGui->Style.Alpha * alpha );
		PushStyleVar( ImGuiStyleVar_WindowPadding, padding );
		PushStyleVar( ImGuiStyleVar_WindowBorderSize, 1 );
		PushStyleVar( ImGuiStyleVar_WindowRounding, 4 ds );
		Begin( temp, 0, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize );

		if ( alpha > 0.95f ) {
			GetCurrentWindow( )->Flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
		} else {
			GetCurrentWindow( )->Flags &= ~ImGuiWindowFlags_NoBringToFrontOnFocus;
		}

		return true;
	}

	return false;
}
void c_widgets::widget_popup::end( )
{
	End( );
	PopStyleVar( 4 );
}

bool c_widgets::widget_nav::tab( c_tab tab, bool selected )
{
	struct s {
		float anim;
		float hover;
		float selected;
	}; auto& obj = anim_obj( tab.label.str.data( ), 0, s{ } );

	auto window = GetCurrentWindow( );
	auto id = window->GetID( tab.label.str.data( ) );
	ImRect bb{ window->DC.CursorPos, window->DC.CursorPos + CalcTextSize( tab.label.translate( ).data( ), 0, 1 ) + vec2{ 48, 20 } };
	ItemSize( bb );
	ItemAdd( bb, id );

	bool hovered, held;
	bool pressed = ButtonBehavior( bb, id, &hovered, &held );

	obj.hover = anim( obj.hover, 0.f, 1.f, hovered );
	obj.selected = anim( obj.selected, 0.f, 1.f, selected );
	obj.anim = anim( obj.anim, 0.f, 1.f, hovered || selected );

	auto col = col_anim( col_anim( g_style->col( pcol_text2 ), g_style->col( pcol_text3 ), obj.hover ), g_style->col( pcol_text ), obj.selected );
	auto iconcol = col_anim( col_anim( g_style->col( pcol_text2 ), g_style->col( pcol_text3 ), obj.hover ), g_style->col( pcol_scheme ), obj.selected );

	GetWindowDrawList( )->AddRectFilled( bb.Min, bb.Max, g_style->col( pcol_bg6 ).alpha( obj.selected ), 4 ds );

	g_draw->text( icons, 16, bb.Min + vec2{ 12, 10 }, iconcol, tab.icon );
	g_draw->text( font, 16, bb.Min + vec2{ 36, 10 }, col, tab.label.translate( ).data( ) );

	return pressed;
}
bool c_widgets::widget_nav::subtab( const char* icon, const ptext& label, bool selected )
{
	struct s {
		float anim;
		float hover;
		float selected;
	}; auto& obj = anim_obj( label.str.data( ), 0, s{ } );

	auto window = GetCurrentWindow( );
	auto id = window->GetID( label.str.data( ) );
	ImRect bb{ window->DC.CursorPos, window->DC.CursorPos + CalcTextSize( label.translate( ).data( ), 0, 1 ) + vec2{ 48, 20 } };
	ItemSize( bb );
	ItemAdd( bb, id );

	bool hovered, held;
	bool pressed = ButtonBehavior( bb, id, &hovered, &held );

	obj.hover = anim( obj.hover, 0.f, 1.f, hovered );
	obj.selected = anim( obj.selected, 0.f, 1.f, selected );
	obj.anim = anim( obj.anim, 0.f, 1.f, hovered || selected );

	auto col = col_anim( col_anim( g_style->col( pcol_text2 ), g_style->col( pcol_text3 ), obj.hover ), g_style->col( pcol_text ), obj.selected );
	auto iconcol = col_anim( col_anim( g_style->col( pcol_text2 ), g_style->col( pcol_text3 ), obj.hover ), g_style->col( pcol_scheme ), obj.selected );

	// window->DrawList->AddRectFilled( bb.Min, bb.Max, g_style->col( pcol_bg2, obj.selected ), 4 ds );

	g_draw->text( icons, 16, bb.Min + vec2{ 12, 10 }, iconcol, icon );
	window->DrawList->AddText( bb.Min + vec2{ 36, 10 }, col, label.translate( ).data( ), FindRenderedTextEnd( label.translate( ).data( ) ) );

	return pressed;
}
void c_widgets::widget_nav::drawtabs( )
{
	SetCursorPos( vec2{ 14, 14 } );
	BeginGroup( );
	{
		PushStyleVar( ImGuiStyleVar_ItemSpacing, vec2{ 8, 0 } );

		for ( int i = 0; i < tabs.size( ); ++i ) {
			if ( tab( tabs[i], i == next ) && next != i ) {
				next = i;
				tab_animdest = 0;
			}

			SameLine( );
		}

		PopStyleVar( );
	}
	EndGroup( );

	tab_anim = ImLerp( tab_anim, tab_animdest, GetIO( ).DeltaTime * 17 );
	subtab_anim = ImLerp( subtab_anim, subtab_animdest, GetIO( ).DeltaTime * 17 );

	if ( tab_anim < 0.05f ) {
		tab_animdest = 1.f;
		current = next;
	}

	if ( subtab_anim < 0.05f ) {
		subtab_animdest = 1.f;
		tabs[current].current = tabs[current].next;
	}
}
void c_widgets::widget_nav::drawsubtabs( )
{
	if ( tabs[current].subtabs.empty( ) ) {
		GetWindowDrawList( )->AddRectFilled( GetWindowPos( ), GetWindowPos( ) + ImVec2{ GetWindowWidth( ), 1 }, g_style->col( pcol_border ) );
		return;
	}

	PushStyleVar( ImGuiStyleVar_Alpha, GImGui->Style.Alpha * tab_anim );
	BeginChild( "subtabs", vec2{ 0, 40 }, 0, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse );
	{
		// GetWindowDrawList( )->AddRectFilled( GetWindowPos( ), GetWindowPos( ) + GetWindowSize( ), g_style->col( pcol_bg6 ), GImGui->Style.ChildRounding );
		// GetWindowDrawList( )->AddRect( GetWindowPos( ) - ImVec2{ 1, 0 }, GetWindowPos( ) + GetWindowSize( ) + ImVec2{ 1, 1 }, g_style->col( pcol_border ), GImGui->Style.ChildRounding );


		SetCursorPos( vec2{ 12, 12 } );

		for ( int i = 0; i < tabs[current].subtabs.size( ); ++i ) {
			if ( subtab( tabs[current].subtabs[i].icon, tabs[current].subtabs[i].label, i == tabs[current].next ) && i != tabs[current].next ) {
				tabs[current].next = i;
				subtab_animdest = 0;
			}

			SameLine( 0, 8 ds );
		}
	}
	EndChild( );
	PopStyleVar( );
	widgets->spacing( 0 );
}
void c_widgets::widget_nav::drawpage( )
{
	if ( tabs[current].pages.size( ) <= tabs[current].current )
		return;

	tabs[current].pages[tabs[current].current]( );
}
void c_widgets::widget_nav::addpage( int tab, std::function< void( ) > code )
{
	tabs[tab].pages.push_back( code );
}

// COLORPICKER

bool c_widgets::widget_colorpicker::draw( const std::string_view& str_id, float* col )
{
	return ColorPicker4( str_id.data( ), col, ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_NoLabel );
}

// MODAL
void c_widgets::widget_modal::add( std::function< void( ) > code )
{
	modals.push_back( { code } );
}
void c_widgets::widget_modal::close( )
{
	modals[modals.size( ) - 1].anim_dest = 0;
}
void c_widgets::widget_modal::handle( )
{
	int i = 0;
	for ( auto& modal : modals ) {
		modal.anim = ImLerp( modal.anim, modal.anim_dest, GetIO( ).DeltaTime * 17 );

		if ( modal.anim < 0.05f && modal.anim_dest == 0 ) {
			modals.erase( modals.begin( ) + ( modals.size( ) - 1 ) );
		}

		SetCursorPos( { 0, 0 } );
		char temp[64];
		ImFormatString( temp, sizeof( temp ), "modal %d", i );
		PushStyleVar( ImGuiStyleVar_Alpha, GImGui->Style.Alpha * modal.anim );
		PushStyleColor( ImGuiCol_ChildBg, GetColorU32( ImGuiCol_WindowBg, 0.98f ) );
		BeginChild( temp, { 0, 0 } );
		{
			PopStyleColor( );

			if ( modal.code )
				modal.code( );
		}
		EndChild( );
		PopStyleVar( );

		i++;
	}
}

// TABLE

void c_widgets::widget_table::begin( const char* str_id, const std::vector< ptext >& columns )
{
	this->id = str_id;
	this->columns = columns;
	this->colsizes.resize( columns.size( ) );
	
	float wp = GImGui->Style.WindowPadding.x;

	PushStyleColor( ImGuiCol_ChildBg, g_style->col( pcol_bg3 ).vec4( ) );
	PushStyleVar( ImGuiStyleVar_WindowPadding, { 0, 0 } );
	BeginChild( str_id, { GetWindowWidth( ) - wp * 2, 0 }, ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Border );
	PopStyleVar( );
	PopStyleColor( );

	char temp[64];
	ImFormatString( temp, sizeof( temp ), "%s header", str_id );
	PushStyleVar( ImGuiStyleVar_WindowPadding, { GImGui->Style.WindowPadding.x, 11 ds } );
	BeginChild( temp, vec2{ 0, 36 }, ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoBackground );
	{
		for ( int i = 0; i < columns.size( ); ++i ) {
			colsizes[i] = ImMax( CalcTextSize( columns[i].translate( ).data( ) ).x, colsizes[i] );
			Text( columns[i].translate( ).data( ) );

			if ( i == columns.size( ) - 2 ) {
				SameLine( GetWindowWidth( ) - GImGui->Style.WindowPadding.x - CalcTextSize( columns[i].translate( ).data( ) ).x );
			} else {
				SameLine( 0, colsizes[i] - CalcTextSize( columns[i].translate( ).data( ) ).x + gap ds );
			}
		}
	}
	EndChild( );
	PopStyleVar( );
	widgets->spacing( 0 );

	ImFormatString( temp, sizeof( temp ), "%s content", str_id );
	BeginChild( temp, { 0, 0 }, ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Border );
}
void c_widgets::widget_table::end( )
{
	EndChild( );
	EndChild( );
	currow = 0;
}

void c_widgets::widget_table::beginrow( )
{
	char temp[128];
	ImFormatString( temp, sizeof( temp ), "%s%d", id, currow );
	BeginChild( temp, { 0, 0 }, ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse );
}
void c_widgets::widget_table::endrow( )
{
	EndChild( );
	currow++;
	curcol = 0;
	colpos = 0;
}
void c_widgets::widget_table::nextrow( )
{

}

void c_widgets::widget_table::begincolumn( float w )
{
	char temp[128];
	ImFormatString( temp, sizeof( temp ), "%s%d%d", id, currow, curcol );
	BeginChild( temp, { 0, 0 }, ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse );
	if ( curcol == columns.size( ) - 1 && w < colsizes[curcol] ) {
		SetCursorPosX( colsizes[curcol] - w );
	}
}
void c_widgets::widget_table::endcolumn( )
{
	colsizes[curcol] = ImMax( colsizes[curcol], GetCurrentWindow( )->ContentSize.x );
	float cellw = GetCurrentWindow( )->ContentSize.x;
	EndChild( );
	if ( curcol == columns.size( ) - 2 ) {
		SameLine( GetWindowWidth( ) - colsizes[curcol + 1] );
	} else {
		SameLine( 0, colsizes[curcol] - cellw + gap ds );
	}
	curcol++;
}
void c_widgets::widget_table::nextcolumn( )
{

}

void c_widgets::widget_table::setcolumnsizes( const std::vector< float >& sizes )
{
	for ( int i = 0; i < sizes.size( ); ++i ) {
		colsizes[i] = sizes[i];
	}
}

// EDITOR

bool c_widgets::widget_editor::draw( char* code, TextEditor& editor )
{
	/*static bool load = false;
	editor.SetImGuiChildIgnored(true);
	if (!load)
	{
		auto lang = TextEditor::LanguageDefinition::Lua();
		editor.SetLanguageDefinition(lang);
		editor.SetReadOnly(false);

		std::string str( code );
		editor.SetText(str);

		load = true;
	}

	editor.SetPalette( editor.GetDarkPalette( ) );

	editor.Render("editor", GetWindowSize());

	if ( editor.IsTextChanged( ) ) {
		auto text = editor.GetText( );
		size_t len = std::min( text.size( ), size_t( 40959 ) );
		std::memcpy( code, text.data( ), len );

		if ( text.size( ) > strlen( code ) )
			editor.SetText( code );
	}*/

	return editor.IsTextChanged();
}

int c_widgets::script( int i, std::vector< c_script >& scripts )
{
	return -1;
}
void c_widgets::tag( const std::string_view& label )
{
	auto window = GetCurrentWindow( );

	ImRect bb{ window->DC.CursorPos, window->DC.CursorPos + vec2{ 12, 10 } + g_draw->textsize( font, 12, label.data( ) ) };

	ItemSize( bb );
	ItemAdd( bb, 0 );

	window->DrawList->AddRectFilled( bb.Min, bb.Max, g_style->col( pcol_scheme, 0.2f ), bb.GetHeight( ) / 2 );
	g_draw->text( font, 12, bb.Min + vec2{ 6, 5 }, g_style->col( pcol_scheme ), label.data( ) );
}

int c_widgets::config( int i, std::vector< c_config >& configs, bool loaded )
{
	int result = -1;
	char temp[64];

	auto& item = configs[i];
	auto& id = item.id;
	auto& editing = item.editing;
	auto& name = item.name;
	auto& creator = item.author;
	auto& modified = item.modified;

	struct s {
		char savedname[32];
	}; auto& obj = anim_obj( id.data( ), 0, s{ } );

	BeginChild( id.data( ), { GetWindowWidth( ) - GImGui->Style.WindowPadding.x * 2, 32 ds + GImGui->FontSize * 2 }, ImGuiChildFlags_Border );
	{
		SetCursorPos( vec2{ 14, 14 } );
		BeginGroup( );
		{
			bool inputhovered = false;
			if ( !editing ) {
				Text( name );
			} else {
				ImFormatString( temp, sizeof( temp ), "##%s name", id.data( ) );
				PushStyleVar( ImGuiStyleVar_FramePadding, { 0, 0 } );
				PushStyleColor( ImGuiCol_FrameBg, GetColorU32( ImGuiCol_FrameBg, 0 ) );
				InputTextEx( temp, 0, name, 32, strlen( name ) == 0 ? ImVec2{ 1, 0 } : CalcTextSize( name ), ImGuiInputTextFlags_NoHorizontalScroll );
				PopStyleColor( );
				PopStyleVar( );

				inputhovered = IsItemHovered( );
			}
			
			SameLine( 0, 8 ds );
			ImFormatString( temp, sizeof( temp ), "##%s edit", id.data( ) );
			if ( button( temp, vec2{ 16, 16 }, { .icon = editing ? I_CHECK : I_EDIT, .background = false, .iconcolor = g_style->col( pcol_text2 ) } ) ) {
				if ( editing ) {
					editing = !editing;
					if ( strlen( name ) == 0 ) {
						strcpy( name, "New config" );
					}
				} else {
					for ( int a = 0; a < configs.size( ); ++a ) {
						configs[a].editing = a == i;
					}
					strcpy( obj.savedname, name );
				}
			}

			if ( !IsItemHovered( ) && !inputhovered && IsMouseClicked( 0 ) && editing ) {
				strcpy( name, obj.savedname );
				editing = false;
			}

			spacing( 4 ds );
			TextDisabled( ptext{ "Created by: " }.translate( ).data( ) );
			SameLine( 0, 0 );
			TextColored( g_style->col( pcol_scheme ), creator.data( ) );
			SameLine( 0, 12 ds );
			TextDisabled( ptext{ "Modified: " }.translate( ).data( ) );
			SameLine( 0, 0 );
			TextColored( g_style->col( pcol_scheme ), modified.data( ) );
		}
		EndGroup( );

		SetCursorPos( ImVec2{ GetWindowWidth( ), GetWindowHeight( ) / 2 } - vec2{ 122, 18 } );
		BeginGroup( );
		{
			ImFormatString( temp, sizeof( temp ), "##%s export", id.data( ) );
			if ( button( temp, vec2{ 36, 36 }, { .icon = I_FILE__EXPORT, .background = false, .iconcolor = g_style->col( pcol_text2 ) } ) ) {
				widgets->modal.add( [=] {
					ImVec2 size{ 300 ds, GImGui->FontSize + 46 ds + GImGui->Style.ItemSpacing.y + GetFrameHeight( ) * 2 };
					SetCursorPos( GetWindowSize( ) / 2 - size / 2 );
					PushStyleVar( ImGuiStyleVar_WindowPadding, vec2{ 14, 14 } );
					PushStyleVar( ImGuiStyleVar_ChildBorderSize, 1 );
					BeginChild( "export", size, ImGuiChildFlags_Border );
					{
						PushItemWidth( GetWindowWidth( ) - GImGui->Style.WindowPadding.x * 2 );

						char code[32];
						ImFormatString( code, sizeof( code ), "%s", id.data( ) );
						widgets->textinput( "Copy this code", code, sizeof( code ), { .flags = ImGuiInputTextFlags_ReadOnly } );

						if ( widgets->button( "Copy", { 0, 0 }, { .icon = I_COPY } ) ) {
							widgets->notify.add( "Information", "The code has been copied", notify_info );
							widgets->modal.close( );
						}

						PopItemWidth( );
					}
					EndChild( );
					PopStyleVar( 2 );
				} );
			}

			SameLine( 0, 0 );

			ImFormatString( temp, sizeof( temp ), "##%s delete", id.data( ) );
			if ( button( temp, vec2{ 36, 36 }, { .icon = I_TRASH, .background = false, .iconcolor = g_style->col( pcol_scheme ).h( 0 ) } ) ) {
				result = 2;
			}

			SameLine( 0, 0 );

			ImFormatString( temp, sizeof( temp ), "##%s", id.data( ) );
			auto btncol = loaded ? g_style->col( pcol_scheme, 0.2f ) : g_style->col( pcol_bg3 );
			auto iconcol = loaded ? g_style->col( pcol_scheme ) : g_style->col( pcol_text2 );
			if ( button( temp, vec2{ 36, 36 }, { .icon = loaded ? I_DEVICE__FLOPPY : I_PLAYER__PLAY, .color = btncol, .iconcolor = iconcol, .lightenval = loaded ? 0.8f : 1.2f } ) ) {
				result = 1;
			}
		}
		EndGroup( );
	}
	EndChild( );

	return result;
}
