#include "parser.h"

#include <functional>
#include <unordered_map>
#include <unordered_set>

using namespace std::placeholders;

// THIS IS QUICK AND DIRTY PARSER!!!
// dont be surprised if it breaks easily for you and either open issue or send PR.

#define error( text, ... ) wprintf( L"Error! " text " at line %d\n", __VA_ARGS__ , m_line );
//#define expect( condition )

bool c_parser::is_end( ) const {
	return m_curr >= m_end;
};

bool c_parser::will_end( int count ) const {
	return m_curr + count >= m_end;
};

bool c_parser::process_comments( ) {
	// is it a comment?
	if ( *m_curr != '/' )
		return true;

	// unexpected end
	if ( will_end( 1 ) )
		return false;

	switch ( *( m_curr + 1 ) ) {
		case '*':
			break;
		case '/':
			skip_until( '\n' );
			break;
		default:
			// its okay
			return true;
	}

	return true;
};

bool c_parser::skip_whitespace( ) {
	while ( !is_end( ) && iswspace( *m_curr ) ) ++m_curr;
	return !is_end( );
};

bool c_parser::read_word( std::wstring& word ) {
	const auto start = m_curr;
	while ( !is_end( ) && ( iswalpha( *m_curr ) || iswdigit( *m_curr ) || *m_curr == '_' ) ) ++m_curr;
	if ( is_end( ) ) return false;

	word = std::wstring( start, m_curr - start );
	return word.length( ) > 0 && skip_whitespace( );
};

bool c_parser::skip_until( wchar_t c ) {
	while ( !is_end( ) && *m_curr != c ) ++m_curr;
	++m_curr;
	if ( is_end( ) ) return false;
	return skip_whitespace( );
};

bool c_parser::skip ( int count ) {
	if ( m_curr + count >= m_end )
		return false;

	m_curr += count;
	return skip_whitespace( );
};

bool c_parser::expect( wchar_t c ) {
	return !is_end( ) && *m_curr == c && skip( 1 );
}

bool c_parser::expect( const std::wstring& text ) {
	const auto len = text.length( );
	return !will_end( len ) && wcsncmp( m_curr, text.data( ), len ) == 0 && skip( len );
}

bool c_parser::parse_enum( proto_file_s& result ) {
	proto_enum_s proto_enum = { };

	if ( !read_word( proto_enum.name ) ) {
		error( "Unexpected end" );
		return false;
	}

	if ( !expect( '{' ) ) {
		error( L"Expected '{'" );
		return false;
	}

	while ( *m_curr != '}' ) {
		proto_enum_entry_s entry = { };

		if ( !read_word( entry.name ) ) {
			error( "Unexpected end" );
			return false;
		}

		if ( !expect( '=' ) ) {
			error( L"Expected '='" );
			return false;
		}

		std::wstring num;
		if ( !read_word( num ) ) {
			error( "Unexpected end" );
			return false;
		}

		if ( !expect( ';' ) ) {
			error( L"Expected ';'" );
			return false;
		}

		entry.number = _wtoi( num.data( ) );

		proto_enum.entries.push_back( entry );
	}
	skip( 1 );

	result.enums.push_back( proto_enum );
	return true;
}

bool c_parser::parse_message( proto_file_s& result ) {
	proto_message_s proto_message = { };

	if ( !read_word( proto_message.name ) ) {
		error( "Unexpected end" );
		return false;
	}

	if ( !expect( '{' ) ) {
		error( L"Expected a '{'" );
		return false;
	}

	while ( *m_curr != '}' ) {
		proto_message_entry_s entry = { };

		std::wstring temp;
		if ( !read_word( temp ) ) {
			error( "Unexpected end" );
			return false;
		}

		auto word_to_type = [ & ] ( const std::wstring& word, e_proto_message_type& type_result ) -> bool {
			static std::unordered_map< std::wstring, e_proto_message_type > s_table = {
				{ L"int32", e_proto_message_type::type_int32 },
				{ L"int64", e_proto_message_type::type_int64 },
				{ L"uint32", e_proto_message_type::type_uint32 },
				{ L"uint64", e_proto_message_type::type_uint64 },
				{ L"sint32", e_proto_message_type::type_sint32 },
				{ L"sint64", e_proto_message_type::type_sint64 },
				{ L"fixed32", e_proto_message_type::type_fixed32 },
				{ L"sfixed32", e_proto_message_type::type_sfixed32 },
				{ L"float", e_proto_message_type::type_float },
				{ L"fixed64", e_proto_message_type::type_fixed64 },
				{ L"sfixed64", e_proto_message_type::type_sfixed64 },
				{ L"double", e_proto_message_type::type_double },
				{ L"bool", e_proto_message_type::type_bool },
				{ L"string", e_proto_message_type::type_string },
				{ L"bytes", e_proto_message_type::type_bytes },
			};

			const auto& it = s_table.find( word );
			if ( it == s_table.end( ) ) {
				// looks like its not a basic type

				// maybe its a message?
				for ( const auto& msg : result.messages ) {
					if ( word == msg.name ) {
						type_result = e_proto_message_type::type_message;
						entry.type_name = msg.name;
						return true;
					}
				}

				// maybe its an enum?
				for ( const auto& _enum : result.enums ) {
					if ( word == _enum.name ) {
						type_result = e_proto_message_type::type_enum;
						entry.type_name = _enum.name;
						return true;
					}
				}
			}

			type_result = it->second;
			return true;
		};

		if ( temp == L"required" ) {
			entry.rule = e_proto_message_rule::required;
		} else if ( temp == L"optional" ) {
			entry.rule = e_proto_message_rule::optional;
		} else if ( temp == L"repeated" ) {
			entry.rule = e_proto_message_rule::repeated;
		} else {
			entry.rule = e_proto_message_rule::none;
			if ( !word_to_type( temp, entry.type ) ) {
				error( "Invalid type %s in %s", temp.data( ), proto_message.name.data( ) );
				return false;
			}
		}

		if ( entry.rule != e_proto_message_rule::none ) {
			// rule was specified, not type
			// so read type again

			if ( !read_word( temp ) ) {
				error( "Unexpected end" );
				return false;
			}

			if ( !word_to_type( temp, entry.type ) ) {
				error( "Invalid type %s in %s", temp.data( ), proto_message.name.data( ) );
				return false;
			}
		}

		if ( !read_word( entry.name ) ) {
			error( "Unexpected end" );
			return false;
		}

		if ( !expect( '=' ) ) {
			error( L"Expected a '='" );
			return false;
		}

		std::wstring tag;
		if ( !read_word( tag ) ) {
			error( "Unexpected end" );
			return false;
		}

		if ( !expect( ';' ) ) {
			error( L"Expected a ';'" );
			return false;
		}

		entry.number = _wtoi( tag.data( ) ); // TODO: check for failed parsing

		proto_message.entries.push_back( entry );
	}
	skip( 1 );
	
	result.messages.push_back( proto_message );
	return true;
}

c_parser::c_parser( const std::wstring& content ) {
	m_content = m_content;
	m_start = content.data( );
	m_end = content.data( ) + content.size( );
	m_curr = content.data( );
	m_line = 0;
}

bool c_parser::parse( proto_file_s& result ) {

	// tokens we dont support
	static std::unordered_set< std::wstring > s_ignore_tokens = {
		L"syntax", L"option", L"import"
	};

	std::unordered_map< std::wstring, std::function< bool( proto_file_s& ) > > g_handlers = {
		{ L"enum", std::bind( &c_parser::parse_enum, this, _1 ) },
		{ L"message", std::bind( &c_parser::parse_message, this, _1 ) },
	};

	// make sure there is something to read
	if ( !skip_whitespace( ) )
		return true;

	while ( !is_end( ) ) {
		std::wstring token;
		if ( !read_word( token ) )
			break;

		// ignore some token
		if ( s_ignore_tokens.contains( token ) ) {
			if ( !skip_until( ';' ) )
				break;
			continue;
		}

		// parse token
		const auto& handler = g_handlers.find( token );
		if ( handler == g_handlers.end( ) ) {
			error( L"Unexpected token %s", token.data( ) );
			return false;
		}

		if ( !handler->second( result ) )
			return false;
	}

	return true;
}
