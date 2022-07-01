#include "generator.h"

#include <sstream>
#include <iostream>
#include <unordered_map>
#include <algorithm>

#include "platform/io.h"

c_generator::c_generator( const std::wstring& path ) {
	m_path = path;
}

bool c_generator::generate( proto_file_s& proto_file ) {
	std::wstringstream ss;

	ss << L"#pragma once" << std::endl;
	ss << L"#include <smallpp.h>" << std::endl;
	ss << std::endl;

	for ( const auto& _enum : proto_file.enums ) {
		ss << L"enum class " << _enum.name << L" {" << std::endl;
		for ( const auto& entry : _enum.entries ) {
			ss << L"	" << entry.name << L" = " << entry.number << "," << std::endl;
		}
		ss << L"};" << std::endl;
		ss << std::endl;
	}

	for ( const auto& msg : proto_file.messages ) {
		ss << L"#define SMPP_FIELDS_" << msg.name << L"( X, a )" << ( !msg.entries.empty( ) ? L" \\" : L"" ) << std::endl;
		for ( const auto& entry : msg.entries ) {
			const auto is_last = &entry == &msg.entries.back( );

			auto rule = L"OPTIONAL";
			switch ( entry.rule ) {
				case e_proto_message_rule::required:
					rule = L"REQUIRED";
					break;
				case e_proto_message_rule::repeated:
					rule = L"REPEATED";
					break;
				default:
					break;
			}

			struct type_definition_s {
				std::wstring base_type;
				std::wstring type_name;
			};
			static std::unordered_map< e_proto_message_type, type_definition_s > s_types_table = {
				{ e_proto_message_type::type_int32, { L"VARINT", L"INT32" } },
				{ e_proto_message_type::type_int64, { L"VARINT", L"INT64" } },
				{ e_proto_message_type::type_uint32, { L"VARINT", L"UINT32" } },
				{ e_proto_message_type::type_uint64, { L"VARINT", L"UINT64" } },
				{ e_proto_message_type::type_sint32, { L"VARINT", L"INT32" } },
				{ e_proto_message_type::type_sint64, { L"VARINT", L"INT64" } },
				{ e_proto_message_type::type_fixed32, { L"FIXED32", L"UINT32" } },
				{ e_proto_message_type::type_sfixed32, { L"FIXED32", L"INT32" } },
				{ e_proto_message_type::type_float, { L"FIXED32", L"FLOAT" } },
				{ e_proto_message_type::type_fixed64, { L"FIXED64", L"UINT64" } },
				{ e_proto_message_type::type_sfixed64, { L"FIXED64", L"INT64" } },
				{ e_proto_message_type::type_double, { L"FIXED64", L"DOUBLE" } },
				{ e_proto_message_type::type_bool, { L"VARINT", L"BOOL" } },
				{ e_proto_message_type::type_string, { L"DATA", L"STRING" } },
				{ e_proto_message_type::type_bytes, { L"DATA", L"BYTES" } },
				{ e_proto_message_type::type_message, { L"MESSAGE", L"" } },
				{ e_proto_message_type::type_enum, { L"ENUM", L"" } },
			};

			const auto type_def = s_types_table.find( entry.type );
			if ( type_def == s_types_table.end( ) ) {
				std::wcout << L"Error! Invalid field type for message " << msg.name << L"!" << std::endl;
				return false;
			}
			
			auto base_type = type_def->second.base_type;
			auto type_name = type_def->second.type_name;
			
			if ( type_name.empty( ) ) {
				type_name = entry.type_name;
			}
			
			ss << L"	X( a, " << rule << L", " << base_type << L", " << type_name << L", " << entry.name << L", " << entry.number << L" )" << ( !is_last ? L" \\" : L"" ) << std::endl;
		}
		const auto max = std::max_element( msg.entries.begin( ), msg.entries.end( ), [ ] ( const proto_message_entry_s& a, const proto_message_entry_s& b ) -> bool { return a.number < b.number; } );
		ss << L"SMPP_BIND( " << msg.name << L", " << ( max != msg.entries.end( ) ? max->number : 0 ) << L" );" << std::endl;
		ss << std::endl;
	}

	auto file = io::c_text_file( m_path );
	if ( !file.write_all_text( ss.str( ) ) ) {
		std::wcout << L"Failed writing to output file!" << std::endl;
		return false;
	}

	return true;
}