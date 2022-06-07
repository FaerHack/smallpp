#pragma once
#include <smallpp.h>

enum e_currency {
	unknown0,
	unknown1,
	unknown2,
	ukrainian
};

#define SMPP_FIELDS_test_message_location( X, a ) \
X( a, FIXED32, FLOAT, latitude, 1 ) \
X( a, FIXED32, FLOAT, longitude, 2 ) \
X( a, DATA, STRING, country, 3 )

SMPP_BIND( test_message_location );


#define SMPP_FIELDS_test_message( X, a ) \
X( a, VARINT, UINT32, version, 1 ) \
X( a, MESSAGE, test_message_location, location, 5 ) \
X( a, ENUM, e_currency, currency, 8 ) \
X( a, DATA, STRING, txn_country_code, 11 )

SMPP_BIND( test_message );