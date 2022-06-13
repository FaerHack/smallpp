#pragma once
#include <smallpp.h>

enum class e_currency {
	unknown0 = 0,
	unknown1 = 1,
	unknown2 = 2,
	ukrainian = 3,
};

#define SMPP_FIELDS_CMsgSOIDOwner( X, a ) \
	X( a, OPTIONAL, VARINT, UINT32, type, 1 ) \
	X( a, OPTIONAL, VARINT, UINT64, id, 2 )
SMPP_BIND( CMsgSOIDOwner, 2 );

#define SMPP_FIELDS_CMsgSOCacheSubscriptionCheck( X, a ) \
	X( a, OPTIONAL, VARINT, UINT64, version, 2 ) \
	X( a, OPTIONAL, MESSAGE, CMsgSOIDOwner, owner_soid, 3 )
SMPP_BIND( CMsgSOCacheSubscriptionCheck, 3 );

#define SMPP_FIELDS_test_message_location( X, a ) \
	X( a, OPTIONAL, FIXED32, FLOAT, latitude, 1 ) \
	X( a, OPTIONAL, FIXED32, FLOAT, longitude, 2 ) \
	X( a, OPTIONAL, DATA, STRING, country, 3 )
SMPP_BIND( test_message_location, 3 );

#define SMPP_FIELDS_test_message( X, a ) \
	X( a, REQUIRED, VARINT, UINT32, version, 1 ) \
	X( a, OPTIONAL, DATA, BYTES, game_data, 2 ) \
	X( a, OPTIONAL, MESSAGE, test_message_location, location, 5 ) \
	X( a, OPTIONAL, ENUM, e_currency, currency, 8 ) \
	X( a, OPTIONAL, DATA, STRING, txn_country_code, 11 )
SMPP_BIND( test_message, 11 );

