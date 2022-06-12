#pragma once
#include <string>
#include <vector>
#include "types.h"

struct parser_result_s {
	std::wstring error;
	std::vector< proto_enum_s > enums;
	std::vector< proto_message_s > messages;
};

class c_parser {
private:
	std::wstring m_content;
	const wchar_t* m_start;
	const wchar_t* m_end;
	const wchar_t* m_curr;
	int m_line; // TODO

	bool is_end( ) const;

	bool will_end( int count ) const;

	bool process_comments( );

	bool skip_whitespace( );

	bool read_word( std::wstring& word );

	bool skip_until( wchar_t c );

	bool skip( int count );

	bool expect( wchar_t c );

	bool expect( const std::wstring& text );

	bool parse_enum( parser_result_s& result );
	
	bool parse_message( parser_result_s& result );

public:
	c_parser( const std::wstring& content );

	bool parse( parser_result_s& result );
};