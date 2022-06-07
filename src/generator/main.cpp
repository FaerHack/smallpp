#include <Windows.h>
#include <stdint.h>
#include <stdio.h>

#include "generator.h"

int main( ) {

	enum class e_field_type {

	};

	struct field_info_s {
		e_field_type type;

		union {

		};
	};

	return 0;
}