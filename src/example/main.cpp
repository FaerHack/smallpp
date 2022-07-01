#define SMPP_ENABLE_STRINGS
#define SMPP_ENABLE_REPEATED

#include "smallpp.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "test_message.h"
#include "test.pb.h"

#define SMPP_FIELDS_test_message( X, a ) \
	X( a, OPTIONAL, DATA, STRING, name, 1 ) \
	X( a, OPTIONAL, VARINT, UINT64, magic, 2 )
SMPP_BIND( test_message, 3 );

#define SMPP_FIELDS_test_repeated_message( X, a ) \
	X( a, REPEATED, MESSAGE, test_message, messages, 1 ) \
	X( a, OPTIONAL, VARINT, UINT64, test, 2 )
SMPP_BIND( test_repeated_message, 5 );

// test:
// - repeated message

int main( ) {
	test_repeated_message msg;
	msg.set_test( 694201337 );

	static const std::string table[ ] = {
		"test",
		"lol",
		"kek",
		"cheburek",
		"",
		"1337",
		"69420"
	};
	
	int magic = 0;
	for ( const auto& name : table ) {
		test_message inline_msg;
		inline_msg.set_name( name );
		inline_msg.set_magic( magic ); magic += 69;
		msg.add_messages( inline_msg );
	}

	const auto size = msg.bytes_size( );
	auto buffer = new uint8_t[ size ];
	auto result1 = msg.write_to_buffer( buffer, size );

	test_repeated_message msg2;
	auto result2 = msg2.parse_from_buffer( buffer, size );

	delete[ ] buffer;
	return 0;

	//test_repeated_message msg;
	//msg.set_test( 694201337 );
	//msg.add_names( "test" );
	//msg.add_names( "lol" );
	//msg.add_names( "kek" );
	//msg.add_names( "cheburek" );
	//msg.add_names( "" );
	//msg.add_names( "1337" );
	//msg.add_names( "69420" );
	//
	//const auto size = msg.bytes_size( );
	//auto buffer = new uint8_t[ size ];
	//auto result1 = msg.write_to_buffer( buffer, size );

	//test_repeated_message msg2;
	//auto result2 = msg2.parse_from_buffer( buffer, size );

	//delete[ ] buffer;
	//return 0;

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