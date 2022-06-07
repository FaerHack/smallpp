#pragma once
#include <smallpp.h>

// TODO: Test enum

#define SMPP_FIELDS_test_message_location( X, a ) \
X( a, FIXED32, FLOAT, latitude, 1 ) \
X( a, FIXED32, FLOAT, longitude, 2 ) \
X( a, DATA, STRING, country, 3 )

SMPP_BIND( test_message_location );


#define SMPP_FIELDS_test_message( X, a ) \
X( a, VARINT, UINT32, version, 1 ) \
X( a, VARINT, UINT32, currency, 8 ) \
X( a, DATA, STRING, txn_country_code, 11 )

SMPP_BIND( test_message );