#pragma once

#include <stdint.h>

namespace smallpp {

	// 
	// Tiny class so we can read easier
	// 

	struct bf_reader {
	private:
		const uint8_t* m_buffer;
		const uint8_t* m_end;

	public:
		bf_reader( const uint8_t* buffer, size_t buffer_size ) {
			m_buffer = buffer;
			m_end = buffer + buffer_size;
		}

		template < typename T >
		bool read( T& value ) {
			constexpr auto size = sizeof( value );
			if ( ( m_buffer + size ) > m_end ) return false;

			value = *( T* )m_buffer;

			m_buffer += size;
			return true;
		}

		bool skip( size_t size ) {
			if ( ( m_buffer + size ) > m_end )
				return false;

			m_buffer += size;
			return true;
		}

		const uint8_t* get_buffer( ) const {
			return m_buffer;
		}
	};

	// 
	// Tiny class so we can write easier
	// 

	struct bf_writer {
	private:
		uint8_t* m_buffer;
		const uint8_t* m_end;

	public:
		bf_writer( uint8_t* buffer, size_t buffer_size ) {
			m_buffer = buffer;
			m_end = buffer + buffer_size;
		}

		template < typename T >
		bool write( T& value ) {
			constexpr auto size = sizeof( value );
			if ( ( m_buffer + size ) >= m_end ) return false;

			*( T* )m_buffer = value;

			m_buffer += size;
			return true;
		}
	};

	// 
	// Base message with base types and few virtual functions
	// 

	class base_message {
	protected:

		enum class e_wire_type : uint64_t {
			varint = 0,
			fixed64 = 1,
			length_delimited = 2,
			group_start = 3,
			group_end = 4,
			fixed32 = 5,
		};

		struct field_header_s {
			e_wire_type type : 3;
			uint64_t number : 61;
		};

		bool read_field( bf_reader& bf, field_header_s& field ) {
			uint64_t value = 0;
			if ( !read_varint( bf, value ) )
				return false;
			
			// TODO: seems weird. probably wont work on some compilers in some cases?
			field = *( field_header_s* )&value;
			return true;
		}

		bool read_varint( bf_reader& bf, uint64_t& value ) {
			bool keep_going = false;
			int shift = 0;

			value = 0;
			do {
				uint8_t next_number = 0;
				if ( !bf.read( next_number ) )
					return false;

				keep_going = ( next_number >= 128 );
				value += ( uint64_t )( next_number & 0x7f ) << shift;
				shift += 7;
			} while ( keep_going );

			return true;
		}

	public:
		virtual bool parse_from_buffer( const uint8_t* buffer, size_t buffer_size ) = 0;
		virtual bool write_to_buffer( uint8_t* buffer, size_t buffer_size ) = 0;
		virtual size_t get_size( ) = 0;
	};

}

#define SMPP_TYPE_INT32 int32_t
#define SMPP_TYPE_INT64 int64_t
#define SMPP_TYPE_UINT32 uint32_t
#define SMPP_TYPE_UINT64 uint64_t
#define SMPP_TYPE_BOOL bool
#define SMPP_TYPE_FLOAT float
#define SMPP_TYPE_DOUBLE double

#define SMPP_DATA_TYPE_STRING const char*

#define SMPP_BASE_TYPE_VARINT( name ) SMPP_TYPE_##name
#define SMPP_BASE_TYPE_FIXED32( name ) SMPP_TYPE_##name
#define SMPP_BASE_TYPE_FIXED64( name ) SMPP_TYPE_##name
#define SMPP_BASE_TYPE_DATA( name ) SMPP_DATA_TYPE_##name
#define SMPP_BASE_TYPE_ENUM( name ) name

#define SMPP_DEFAULT_VALUE_VARINT( name ) 0
#define SMPP_DEFAULT_VALUE_FIXED32( ... ) 0
#define SMPP_DEFAULT_VALUE_FIXED64( ... ) 0
#define SMPP_DEFAULT_VALUE_DATA( name ) nullptr
#define SMPP_DEFAULT_VALUE_ENUM( name ) // TODO

#define SMPP_GET_TYPE( base_type, type ) SMPP_BASE_TYPE_##base_type( type )

#define SMPP_DEFINE_MEMBER_ENTRY( a, base_type, type, name, tag ) SMPP_GET_TYPE( base_type, type ) name;
#define SMPP_DEFINE_CONSTRUCTOR_ENTRY( a, base_type, type, name, tag ) name = SMPP_DEFAULT_VALUE_##base_type( type ); // TODO: Add support for enums

#define SMPP_DEFINE_READ_BASE( a, name, function ) \
uint64_t value = 0; \
if ( !( function( bf, value ) ) ) \
	return false; \
\
name = value;

#define SMPP_DEFINE_READ_TYPE( a, name ) \
if ( !( bf.read( name ) ) ) \
	return false;
 
#define SMPP_DEFINE_READ_TYPE_VARINT( a, base_type, type, name, tag ) SMPP_DEFINE_READ_BASE( a, name, read_varint )
#define SMPP_DEFINE_READ_TYPE_FIXED32( a, base_type, type, name, tag ) SMPP_DEFINE_READ_TYPE( a, name )
#define SMPP_DEFINE_READ_TYPE_FIXED64( a, base_type, type, name, tag ) SMPP_DEFINE_READ_TYPE( a, name )
//#define SMPP_DEFINE_READ_TYPE_ENUM( a, base_type, type, name, tag ) SMPP_DEFINE_READ_BASE( a, name, read_varint )

#define SMPP_DEFINE_READ_TYPE_DATA( a, base_type, type, name, tag ) \
uint64_t size = 0; \
if ( !read_varint( bf, size ) ) \
	return false; \
\
name = ( SMPP_GET_TYPE( base_type, type ) )bf.get_buffer( ); \
if ( !bf.skip( size ) ) \
	return false;

#define SMPP_DEFINE_READ_ENTRY( a, base_type, type, name, tag ) \
case tag: \
{ \
SMPP_DEFINE_READ_TYPE_##base_type( a, base_type, type, name, tag ) \
break; \
}

// TODO
#define SMPP_DEFINE_WRITE_ENTRY( )
#define SMPP_DEFINE_GET_SIZE_ENTRY( )

// TODO: Add support for is_*_set
// TODO: do some mask shit to ensure all required fields are set and to check if optional fields are indeed set
#define SMPP_BIND( name ) \
struct name : public smallpp::base_message { \
	SMPP_FIELDS_##name( SMPP_DEFINE_MEMBER_ENTRY, 0 ) \
\
	name( ) { \
		SMPP_FIELDS_##name( SMPP_DEFINE_CONSTRUCTOR_ENTRY, 0 ) \
	}\
\
	bool parse_from_buffer( const uint8_t* buffer, size_t buffer_size ) override { \
		auto bf = smallpp::bf_reader( buffer, buffer_size ); \
		field_header_s field = { }; \
		while ( read_field( bf, field ) ) { \
			switch ( field.number ) { \
				SMPP_FIELDS_##name( SMPP_DEFINE_READ_ENTRY, 0 ) \
				default: \
				{\
					switch ( field.type ) { \
						case e_wire_type::varint: \
						{ \
							uint64_t dummy = 0; \
							if ( !read_varint( bf, dummy ) ) \
								return false; \
							break; \
						} \
						case e_wire_type::length_delimited: \
						{ \
							uint64_t length = 0; \
							if ( !read_varint( bf, length ) ) \
								return false; \
							bf.skip( length ); \
							break; \
						} \
						default: \
							return false; \
					} \
				}\
			} \
		} \
		return true; \
	} \
\
	bool write_to_buffer( uint8_t* buffer, size_t buffer_size ) override { \
		auto bf = smallpp::bf_writer( buffer, buffer_size ); \
		SMPP_FIELDS_##name( SMPP_DEFINE_WRITE_ENTRY, 0 ) \
		return true; \
	} \
\
	size_t get_size( ) override { \
		return 0; \
	} \
};