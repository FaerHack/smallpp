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
	const auto size1 = msg.bytes_size( );

	msg.set_currency( e_currency::ukrainian );
	msg.set_txn_country_code( { "TEST", 4 } );
	msg.set_version( 1337 );
	const auto size2 = msg.bytes_size( );

	test_message_location location;
	location.set_country( { "UA", 2 } );
	location.set_latitude( 69.420 );
	location.set_longitude( 42.228 );
	msg.set_location( location );
	const auto size3 = msg.bytes_size( );

	return 0;
}