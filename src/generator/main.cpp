#include <Windows.h>
#include <stdint.h>
#include <stdio.h>
#include <codecvt>

#include "io.h"
#include "parser.h"

int wmain( int argc, wchar_t* argv[ ] ) {

	if ( argc <= 1 ) {
		wprintf( L"Usage: %s files...\n", argv[ 0 ] );
		return -1;
	}

	for ( auto i = 1; i < argc; ++i ) {
		const std::wstring path = argv[ i ];
		auto file = io::c_text_file( path );

		if ( !file.exists( ) ) {
			wprintf( L"Failed to open %s!\n", path.data( ) );
			continue;
		}

		auto content = file.read_all_text( );
		if ( content.empty( ) ) {
			wprintf( L"Failed to read %s!\n", path.data( ) );
			continue;
		}

		auto parser = c_parser( content );

		parser_result_s parser_result;
		if ( !parser.parse( parser_result ) ) {
			//wprintf( L"Error! %s at line %d\n", parser_result.error );
			return -1;
		}

		for ( const auto& _enum : parser_result.enums ) {
			wprintf( L"%s:\n", _enum.name.data( ) );
			for ( const auto& entry : _enum.entries ) {
				wprintf( L"%s: %d\n", entry.name.data( ), entry.number );
			}
			wprintf( L"\n" );
		}
		
		for ( const auto& message : parser_result.messages ) {
			wprintf( L"%s:\n", message.name.data( ) );
			for ( const auto& entry : message.entries ) {
				std::wstring rule;
				switch ( entry.rule ) {
					case e_proto_message_rule::required:
						rule = L"required";
						break;
					case e_proto_message_rule::optional:
						rule = L"optional";
						break;
					default:
						rule = L"        ";
						break;
				}
				wprintf( L"%s %s: %s ( %d )\n", rule.data( ), entry.name.data( ), entry.type.data( ), entry.number );
			}
			wprintf( L"\n" );
		}

		// TODO: process in generator
	}

	return 0;
}