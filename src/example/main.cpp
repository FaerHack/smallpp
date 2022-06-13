#define SMPP_ENABLE_STRINGS

#include "smallpp.h"

#include <Windows.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "test_message.h"
#include "test.pb.h"

int main( ) {
	test_message _msg;
	auto result = _msg.parse_from_buffer( resources::test_message, sizeof( resources::test_message ) );
	const auto& _location = _msg.get_location( );

	test_message msg;
	const auto size1 = msg.bytes_size( );

	msg.set_currency( e_currency::ukrainian );
	msg.set_txn_country_code( "TEST" );
	msg.set_version( 1337 );
	const auto size2 = msg.bytes_size( );

	test_message_location location;
	location.set_country( "UA" );
	location.set_latitude( 69.420 );
	location.set_longitude( 42.228 );
	msg.set_location( location );
	const auto size3 = msg.bytes_size( );

	auto buffer = new uint8_t[ size3 ];
	auto result2 = msg.write_to_buffer( buffer, size3 );

	test_message msg2;
	auto result3 = msg2.parse_from_buffer( buffer, size3 );
	auto test = msg2.get_location( ).get_country( );

	delete[ ] buffer;
	return 0;
}