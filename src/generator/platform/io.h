#pragma once
#include <string>
#include <optional>
#include <vector>
#include <sstream>
#include <fstream>
#include <codecvt>
#include <filesystem>

namespace io {

	class c_text_file {
	private:
		std::wstring m_path;

	public:
		c_text_file( const std::wstring& path ) {
			m_path = path;
		}

		~c_text_file( ) {

		}

		bool exists( ) {
			return std::filesystem::exists( m_path );
		}

		std::wstring read_all_text( ) {
			std::wifstream wif( m_path );
			if ( !wif.is_open( ) )
				return L"";

			wif.imbue( std::locale( std::locale::empty( ), new std::codecvt_utf8<wchar_t> ) );
			std::wstringstream wss;
			wss << wif.rdbuf( );
			return wss.str( );
		}

		bool write_all_text( const std::wstring& text ) {
			std::wofstream wof( m_path );
			if ( !wof.is_open( ) )
				return false;

			wof << text;
			wof.close( );
			return wof.good( );
		}
	};

}