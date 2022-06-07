#include "smallpp.h"

#include <stdio.h>
#include <stdint.h>

#include "test_message.h"
#include "generated.h"

int main( ) {
	test_message msg;
	auto result = msg.parse_from_buffer( resources::test_message, sizeof( resources::test_message ) );

	test_message_location location;
	auto result2 = msg.get_location( location );

	return 0;
}