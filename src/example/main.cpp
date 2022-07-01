#define SMPP_ENABLE_STRINGS
#define SMPP_ENABLE_REPEATED

#include "smallpp.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "test_message.h"
#include "test.pb.h"

#define SMPP_FIELDS_test_repeated_message( X, a ) \
	X( a, REPEATED, VARINT, UINT64, ids, 1 ) \
	X( a, OPTIONAL, VARINT, UINT64, test, 2 )
SMPP_BIND( test_repeated_message, 5 );

// test:
// - repeated string
// - repeated message

int main( ) {
	test_repeated_message msg;
	msg.set_test( 694201337 );
	msg.add_ids( 42 );
	msg.add_ids( 47 );
	msg.add_ids( 69 );
	msg.add_ids( 228 );
	msg.add_ids( 1337 );
	msg.add_ids( 1488 );
	msg.add_ids( 123 );

	const auto size = msg.bytes_size( );
	auto buffer = new uint8_t[ size ];
	auto result1 = msg.write_to_buffer( buffer, size );

	test_repeated_message msg2;
	auto result2 = msg2.parse_from_buffer( buffer, size );

	delete[ ] buffer;
	return 0;

	/*test_message _msg;
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
	return 0;*/
}