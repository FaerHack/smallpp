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

//enum class e_proto_message_type {
//	_double,
//	_float,
//	int32,
//	_int64,
//
//};

struct proto_message_entry_s {
	e_proto_message_rule rule;
	std::wstring type;
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