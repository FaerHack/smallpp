#pragma once
#include <string>
#include <vector>

struct proto_enum_entry_s {
	std::wstring name;
	int number;
};

enum class e_proto_message_rule {
	none,
	required,
	optional,
	repeated
};

enum class e_proto_message_type {
	type_double,
	type_float,
	type_int32,
	type_int64,
	type_uint32,
	type_uint64,
	type_sint32,
	type_sint64,
	type_fixed32,
	type_fixed64,
	type_sfixed32,
	type_sfixed64,
	type_bool,
	type_string,
	type_bytes,
	type_message,
	type_enum,
};

struct proto_message_entry_s {
	e_proto_message_rule rule;
	e_proto_message_type type;
	std::wstring type_name;
	std::wstring name;
	int number;
};

template< typename T >
struct proto_generic_s {
	std::wstring name;
	std::vector< T > entries;
};

using proto_enum_s = proto_generic_s< proto_enum_entry_s >;
using proto_message_s = proto_generic_s< proto_message_entry_s >;

struct proto_file_s {
	std::wstring error;
	std::vector< proto_enum_s > enums;
	std::vector< proto_message_s > messages;
};