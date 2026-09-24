#pragma once

class c_language {
	struct c_lang {
        std::string name;
        std::unordered_map< std::string, std::string > dict{ };
    };
public:
	int selected;
	std::vector< c_lang > list;

	void add_language( std::string name, std::unordered_map< std::string, std::string > dict ) {
        list.push_back( c_lang{ name, dict } );
    }

    void initialize( ) {
        add_language( "English", {} );
        add_language( "Russian", {
            { "Enable", reinterpret_cast< const char* >( u8"¬ключить" ) },    
        } );
    }

    std::string_view translate( std::string_view str ) {
        if ( !str.data( ) ) return "";
        if ( selected == 0 )
            return str;

        auto it = list[selected].dict.find( str.data( ) );
        if ( it == list[selected].dict.end( ) ) return str;

        return it->second.c_str( );
    }

    std::vector< std::string_view > vec( ) {
        std::vector< std::string_view > result;

        for ( const auto& lang : list ) {
            result.push_back( lang.name.c_str( ) );
        }

        return result;
    }
};

inline auto g_lang = std::make_unique< c_language >( );