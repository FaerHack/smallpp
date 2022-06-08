#include "smallpp.h"

#include <Windows.h>
#include <stdio.h>
#include <stdint.h>

#include "test_message.h"
#include "generated.h"

int main( ) {
	//test_message msg;
	//auto result = msg.parse_from_buffer( resources::test_message, sizeof( resources::test_message ) );
	//const auto& location = msg.get_location( );

	test_message msg;
	auto size1 = msg.get_size( );

	return 0;
}