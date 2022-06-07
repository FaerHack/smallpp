#pragma once
#include <stdint.h>

class i_base_field_type {
public:
	virtual size_t get_size( ) = 0;
};