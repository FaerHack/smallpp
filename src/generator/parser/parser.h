#pragma once
#include <string>
#include <vector>
#include "shared/types.h"

class c_parser {
private:
	std::wstring m_content;
	const wchar_t* m_start;
	const wchar_t* m_end;
	const wchar_t* m_curr;
	int m_line; // TODO

	bool is_end( ) const;

	bool will_end( int count ) const;

	bool do_comments( );

	bool skip_whitespace( );

	bool skip_until( wchar_t c );

	bool skip_until( const std::wstring& s );

	bool read_word( std::wstring& word );

	bool skip( int count );

	bool expect( wchar_t c );

	bool expect( const std::wstring& text );

	bool parse_enum( proto_file_s& result );
	
	bool parse_message( proto_file_s& result );

public:
	c_parser( const std::wstring& content );

	bool parse( proto_file_s& result );
};