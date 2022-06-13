#include <stdint.h>
#include <iostream>
#include <codecvt>
#include <filesystem>

#include "platform/io.h"
#include "parser/parser.h"
#include "generator/generator.h"

int wmain( int argc, wchar_t* argv[ ] ) {

	if ( argc <= 2 ) {
		std::wcout << L"Usage: " << argv[ 0 ] << " <output dir> files..." << std::endl;
		return -1;
	}

	const auto output_dir = std::filesystem::path( argv[ 1 ] );
	if ( !std::filesystem::is_directory( output_dir ) ) {
		std::wcout << L"Output directory is invalid!" << std::endl;
		return -1;
	}

	for ( auto i = 2; i < argc; ++i ) {
		const auto path = std::filesystem::path( argv[ i ] );
		auto file = io::c_text_file( path );

		if ( !file.exists( ) ) {
			std::wcout << L"Failed to open " << path << "!" << std::endl;
			continue;
		}

		auto content = file.read_all_text( );
		if ( content.empty( ) ) {
			std::wcout << L"Failed to read " << path << "!" << std::endl;
			continue;
		}

		proto_file_s proto_file = { };

		auto parser = c_parser( content );
		if ( !parser.parse( proto_file ) ) {
			//wprintf( L"Error! %s at line %d\n", parser_result.error );
			return -1;
		}

		const auto output_path = ( output_dir / path.filename( ) ).replace_extension( L".pb.h" );

		auto generator = c_generator( output_path );
		if ( !generator.generate( proto_file ) ) {
			return -1;
		}

		std::wcout << L"Generated " << output_path << std::endl;
	}

	return 0;
}