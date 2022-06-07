#include "smallpp.h"

#include <Windows.h>
#include <stdio.h>
#include <stdint.h>

#include "test_message.h"
#include "generated.h"

int main( ) {
	test_message msg;
	auto result = msg.parse_from_buffer( resources::test_message, sizeof( resources::test_message ) );
	auto location = msg.get_location( );
	
	return 0;
}