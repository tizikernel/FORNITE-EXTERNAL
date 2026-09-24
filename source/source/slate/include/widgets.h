#pragma once
#include <string>
#include <vector>
#include <functional>
#include <memory>

// Dummy TextEditor for compilation
class TextEditor {
public:
    struct LanguageDefinition {
        static LanguageDefinition Lua() { return {}; }
    };
    void SetImGuiChildIgnored(bool) {}
    void SetLanguageDefinition(const LanguageDefinition&) {}
    void SetReadOnly(bool) {}
    void SetText(const std::string&) {}
    void SetPalette(const void*) {}
    void* GetDarkPalette() { return nullptr; }
    void Render(const char*, const ImVec2&) {}
    bool IsTextChanged() { return false; }
};

class c_widgets {
public:
	struct widget_window {
		void begin( const std::string_view& name, ImVec2 size, ImGuiWindowFlags flags = 0 );
		void end( );
	}; widget_window window;
	struct widget_child {
		void begin( const std::string_view& icon, const ptext& name, ImVec2 size = ImVec2{ 0, 0 }, c_childstyle style = { } );
		void end( );
		float* smoothscroll( bool scrollbar = false, ImVec2 padding = ImVec2{ 0, 0 } );
	} child;
	struct widget_content {
		void begin( const std::string_view& str_id, ImVec2 size );
		void end( );
	} content;
	struct widget_menu {
		void begin( const std::string_view& str_id );
		void end( );
		bool button( const ptext& label, ImVec2 size, c_buttonstyle style = { } );
	} menu;
	struct widget_popup {
		bool begin( const std::string_view& str_id, float alpha, ImVec2 pos, ImVec2 padding = ImVec2{ 0, 0 } );
		void end( );
	} popup;
	struct widget_notify {
		std::vector< c_notify > notifications;

		void add( const std::string_view& title, const std::string_view& text, notify_ status );
		void handle( );
	} notify;
	struct widget_modal {
	private:
		struct c_modal {
			std::function< void( ) > code;
			float anim = 0.f;
			float anim_dest = 1.f;
		};

		std::vector< c_modal > modals;
	public:
		void add( std::function< void( ) > code );
		void close( );
		void handle( );
	} modal;
	struct widget_nav {
		std::vector< c_tab > tabs;
		std::vector< c_tabcategory > categories;
		int current = 0;
		int next = 0;
		float tab_anim = 1;
		float tab_animdest = 1;
		float subtab_anim = 1;
		float subtab_animdest = 1;

		void drawtabs( );
		void drawsubtabs( );
		void drawpage( );

		bool tab( c_tab tab, bool selected );
		bool subtab( const char* icon, const ptext& label, bool selected );

		void addpage( int tab, std::function< void( ) > code );
	} nav;
	struct widget_colorpicker {
		bool draw( const std::string_view& str_id, float* col );
	} colorpicker;
	struct widget_editor {
		c_script* script;
		bool draw( char* code, TextEditor& editor );
	} editor;
	struct widget_table {
		void begin( const char* str_id, const std::vector< ptext >& columns );
		void end( );

		void beginrow( );
		void endrow( );
		void nextrow( );

		void begincolumn( float w = 0 );
		void endcolumn( );
		void nextcolumn( );

		void setcolumnsizes( const std::vector< float >& sizes );

	private:
		const char* id;
		std::vector< float > colsizes;
		std::vector< ptext > columns;

		int currow = 0;
		int curcol = 0;

		float colpos = 0;

		float gap = 8;
	};

	bool button( const ptext& label, ImVec2 size = ImVec2{ 0, 0 }, c_buttonstyle style = { } );
	bool iconbutton( const char* str_id, const char* icon );
	
	template < typename T >
	bool slider( const ptext& label, T* v, T min, T max, const char* format, c_sliderstyle style = { } );
	bool sliderint( const ptext& label, int* v, int min, int max, const char* format = "%d" );
	bool sliderfloat( const ptext& label, float* v, float min, float max, const char* format = "%.1f" );
	bool checkbox( const ptext& label, bool* v, c_checkboxstyle style = { } );
	void checkpicker( const ptext& label, bool* v, float col[4], std::function< void( ) > options = nullptr );
	bool optionsbtn( const std::string_view& str_id, std::function< void( ) > options, ImVec2 windowpos = ImVec2{ 0, 0 }, vec2 padding = vec2{ 14, 14 } );
	bool tooltipbtn( const std::string_view& str_id, const ptext& tooltip, const char* icon = I_INFO__CIRCLE );
	bool comboex( const ptext& label, const std::string_view& preview, bool should_close = false, c_combostyle style = { } );
	bool combo( const ptext& label, int* v, std::vector< std::string_view > items );
	bool multicombo( const ptext& label, bool* v, std::vector< std::string_view > items );
	bool coloredit( const ptext& label, float* col, ImVec2 size = ImVec2{ 0, 0 } );
	bool binder( const ptext& label, int* key );
	bool selector( const ptext& label, int* v, const std::vector< std::string_view >& items );
	bool selectable( const ptext& label, bool selected, ImVec2 size = ImVec2{ 0, 0 }, c_selectablestyle style = { } );
	void spacing( float px );
	void separator( bool vertical = false, float size = 0 );
	bool textinput( const ptext& label, char* buf, size_t buf_size, c_textinputstyle style = { } );
	bool keybind( const char* label, int* key, bool show_label = true );
	int config( int i, std::vector< c_config >& configs, bool loaded );
	int script( int i, std::vector< c_script >& scripts );
	void tag( const std::string_view& label );
	bool tab( const ptext& label, bool selected );
	bool themebutton( const char* str_id, int i, c_theme& theme, bool selected, ImColor col, bool editable );
	void themeselector( const ptext& label );
	void showWidgetTeams(int solo, int duos, int trios, int squads);
	void showSpecators();
	void showRadar();
	void folder( c_folder& item );
	bool file( c_file& item );
};

inline auto widgets = std::make_unique< c_widgets >( );