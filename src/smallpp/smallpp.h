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
	// Tiny class to simplify working with bits
	// 

	constexpr auto BITS_PER_INT = 32;
	constexpr auto BITSET_INT( int bitNum ) { return ( ( bitNum ) >> 5/*log2(32)*/ ); }

	template<size_t num_bits, size_t num_data = ( num_bits + ( BITS_PER_INT - 1 ) ) / BITS_PER_INT>
	struct bit_set {
	public:
		uint32_t m_data[ num_data ];

		bit_set( ) {
			clear( );
		}

		inline bool is_set( int n ) const {
			return ( ( m_data[ BITSET_INT( n ) ] & ( 1U << ( n % BITS_PER_INT ) ) ) != 0 );
		}

		inline void set( int n, bool v ) {
			if ( v )
				m_data[ BITSET_INT( n ) ] |= ( 1U << ( n % BITS_PER_INT ) );
			else
				m_data[ BITSET_INT( n ) ] &= ~( 1U << ( n % BITS_PER_INT ) );
		}

		inline void clear( ) {
			for ( auto i = 0; i < num_data; i++ )
				m_data[ i ] = 0;
		}
	};

	// 
	// Small structs to hold certain data
	// 

	struct string_s {
		const char* buffer;
		size_t length;
	};

	struct data_s {
		const uint8_t* buffer;
		size_t size;
	};

	// 
	// 
	// 

	enum class e_wire_type : uint64_t {
		varint = 0,
		fixed64 = 1,
		length_delimited = 2,
		group_start = 3,
		group_end = 4,
		fixed32 = 5,
	};

	// 
	// Base message with base types and few virtual functions
	// 

	class base_message {
	protected:

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
		virtual void clear( ) = 0;
	};

}

#define SMPP_TYPE_INT32 int32_t
#define SMPP_TYPE_INT64 int64_t
#define SMPP_TYPE_UINT32 uint32_t
#define SMPP_TYPE_UINT64 uint64_t
#define SMPP_TYPE_BOOL bool
#define SMPP_TYPE_FLOAT float
#define SMPP_TYPE_DOUBLE double

#define SMPP_DATA_TYPE_STRING smallpp::string_s
#define SMPP_DATA_TYPE_BYTES smallpp::data_s

#define SMPP_WIRE_TYPE_VARINT smallpp::e_wire_type::varint
#define SMPP_WIRE_TYPE_FIXED32 smallpp::e_wire_type::fixed32
#define SMPP_WIRE_TYPE_FIXED64 smallpp::e_wire_type::fixed64
#define SMPP_WIRE_TYPE_DATA smallpp::e_wire_type::length_delimited
#define SMPP_WIRE_TYPE_MESSAGE smallpp::e_wire_type::length_delimited
#define SMPP_WIRE_TYPE_ENUM smallpp::e_wire_type::varint

#define SMPP_BASE_TYPE_VARINT( name ) SMPP_TYPE_##name
#define SMPP_BASE_TYPE_FIXED32( name ) SMPP_TYPE_##name
#define SMPP_BASE_TYPE_FIXED64( name ) SMPP_TYPE_##name
#define SMPP_BASE_TYPE_DATA( name ) SMPP_DATA_TYPE_##name
#define SMPP_BASE_TYPE_MESSAGE( name ) name
#define SMPP_BASE_TYPE_MESSAGE_DATA( ... ) smallpp::data_s
#define SMPP_BASE_TYPE_ENUM( name ) name

#define SMPP_DEFAULT_VALUE_VARINT( ... ) 0
#define SMPP_DEFAULT_VALUE_FIXED32( ... ) 0
#define SMPP_DEFAULT_VALUE_FIXED64( ... ) 0
#define SMPP_DEFAULT_VALUE_DATA( ... ) { nullptr, 0 }
#define SMPP_DEFAULT_VALUE_MESSAGE( ... ) { }
#define SMPP_DEFAULT_VALUE_MESSAGE_DATA( ... ) { nullptr, 0 }
#define SMPP_DEFAULT_VALUE_ENUM( name ) ( name )0

#define SMPP_GET_TYPE( base_type, type ) SMPP_BASE_TYPE_##base_type( type )

#define SMPP_DEFINE_MEMBER_ENTRY( a, flag, base_type, type, name, tag ) SMPP_GET_TYPE( base_type, type ) name;
#define SMPP_DEFINE_CONSTRUCTOR_ENTRY( a, flag, base_type, type, name, tag ) name = SMPP_DEFAULT_VALUE_##base_type( type );

#define SMPP_DEFINE_READ_BASE( a, base_type, type, name, function ) \
uint64_t value = 0; \
if ( !( function( bf, value ) ) ) \
	return false; \
\
this->name = ( SMPP_GET_TYPE( base_type, type ) )value;

#define SMPP_DEFINE_READ_TYPE( a, name ) \
if ( !( bf.read( this->name ) ) ) \
	return false;
 
#define SMPP_DEFINE_READ_TYPE_VARINT( a, flag, base_type, type, name, tag ) SMPP_DEFINE_READ_BASE( a, base_type, type, name, this->read_varint )
#define SMPP_DEFINE_READ_TYPE_FIXED32( a, flag, base_type, type, name, tag ) SMPP_DEFINE_READ_TYPE( a, name )
#define SMPP_DEFINE_READ_TYPE_FIXED64( a, flag, base_type, type, name, tag ) SMPP_DEFINE_READ_TYPE( a, name )
#define SMPP_DEFINE_READ_TYPE_ENUM( a, flag, base_type, type, name, tag ) SMPP_DEFINE_READ_BASE( a, base_type, type, name, this->read_varint )

#define SMPP_DEFINE_READ_TYPE_MESSAGE( a, flag, base_type, type, name, tag ) \
uint64_t size = 0; \
if ( !read_varint( bf, size ) ) \
	return false; \
\
auto buff = bf.get_buffer( ); \
if ( !bf.skip( size ) ) \
	return false; \
\
if ( !name.parse_from_buffer( buff, size ) ) \
	return false;

#define SMPP_DEFINE_READ_TYPE_MESSAGE_DATA( a, flag, base_type, type, name, tag ) \
uint64_t size = 0; \
if ( !read_varint( bf, size ) ) \
	return false; \
\
this->name.buffer = bf.get_buffer( ); \
this->name.size = size; \
if ( !bf.skip( size ) ) \
	return false;

#define SMPP_DEFINE_READ_TYPE_DATA_STRING( a, flag, base_type, type, name, tag ) \
uint64_t size = 0; \
if ( !this->read_varint( bf, size ) ) \
	return false; \
\
this->name.buffer = ( const char* )bf.get_buffer( ); \
this->name.length = size; \
if ( !bf.skip( size ) ) \
	return false;

#define SMPP_DEFINE_READ_TYPE_DATA_BYTES( a, flag, base_type, type, name, tag ) \
uint64_t size = 0; \
if ( !this->read_varint( bf, size ) ) \
	return false; \
\
this->name.buffer = bf.get_buffer( ); \
this->name.size = size; \
if ( !bf.skip( size ) ) \
	return false;

#define SMPP_DEFINE_READ_TYPE_DATA( a, flag, base_type, type, name, tag ) SMPP_DEFINE_READ_TYPE_DATA_##type( a, flag, base_type, type, name, tag )

#define SMPP_DEFINE_READ_ENTRY( a, flag, base_type, type_, name, tag ) \
case tag: \
{ \
	if ( field.type != SMPP_WIRE_TYPE_##base_type ) return false; \
	this->__INTERNAL_tags.set( tag, true ); \
	SMPP_DEFINE_READ_TYPE_##base_type( a, flag, base_type, type_, name, tag ) \
	break; \
}


#define SMPP_DEFINE_POST_READ_ENTRY_REQUIRED( a, flag, base_type, type, name, tag ) \
if ( !this->__INTERNAL_tags.is_set( tag ) ) return false;

#define SMPP_DEFINE_POST_READ_ENTRY_OPTIONAL( ... )
#define SMPP_DEFINE_POST_READ_ENTRY_REPEATED( ... )

#define SMPP_DEFINE_POST_READ_ENTRY( a, flag, base_type, type, name, tag ) SMPP_DEFINE_POST_READ_ENTRY_##flag( a, flag, base_type, type, name, tag )


// TODO
#define SMPP_DEFINE_WRITE_ENTRY( )
#define SMPP_DEFINE_GET_SIZE_ENTRY( )


#define SMPP_DEFINE_CLASS_ENTRY_BASE( a, flag, base_type, type, name, tag ) \
bool has_##name( ) const { return this->__INTERNAL_tags.is_set( tag ); } \
\
decltype( name ) get_##name( ) const { \
	return this->name; \
} \
\
void set_##name( decltype( name ) value ) { \
	this->__INTERNAL_tags.set( tag, true ); \
	this->name = value; \
}

#define SMPP_DEFINE_CLASS_ENTRY_BASE_MESSAGE( a, flag, base_type, type, name, tag ) \
bool has_##name( ) const { return this->__INTERNAL_tags.is_set( tag ); } \
\
const type& get_##name( ) const { \
	if ( !this->__INTERNAL_tags.is_set( tag ) ) return { }; \
	return this->name; \
} \
\
//void set_##name( const type& value) { \
//	this->__INTERNAL_tags.set( tag, true ); \
//	this->name = value; \
//}

#define SMPP_DEFINE_CLASS_ENTRY_BASE_MESSAGE_DATA( a, flag, base_type, type, name, tag ) \
bool has_##name( ) const { return this->__INTERNAL_tags.is_set( tag ); } \
\
bool get_##name( type& out ) const { \
	if ( !this->__INTERNAL_tags.is_set( tag ) ) return false; \
	return out.parse_from_buffer( this->name.buffer, this->name.size ); \
} \
\
void set_##name( const uint8_t* buffer, size_t buffer_size ) { \
	this->__INTERNAL_tags.set( tag, true ); \
	this->name.buffer = buffer; \
	this->name.size = buffer_size; \
} \

#define SMPP_DEFINE_CLASS_ENTRY_DATA_BASE( a, flag, base_type, type, name, tag ) \
bool has_##name( ) const { return this->__INTERNAL_tags.is_set( tag ); } \
\
const decltype( name )& get_##name( ) const { \
	return this->name; \
} \
\
void set_##name( decltype( name ) value ) { \
	this->__INTERNAL_tags.set( tag, true ); \
	this->name = value; \
} \

#define SMPP_DEFINE_CLASS_ENTRY_DATA_BYTES( a, flag, base_type, type, name, tag ) SMPP_DEFINE_CLASS_ENTRY_DATA_BASE( a, flag, base_type, type, name, tag )
#define SMPP_DEFINE_CLASS_ENTRY_DATA_STRING( a, flag, base_type, type, name, tag ) SMPP_DEFINE_CLASS_ENTRY_DATA_BASE( a, flag, base_type, type, name, tag )

#define SMPP_DEFINE_CLASS_ENTRY_BASE_VARINT( a, flag, base_type, type, name, tag ) SMPP_DEFINE_CLASS_ENTRY_BASE( a, flag, base_type, type, name, tag )
#define SMPP_DEFINE_CLASS_ENTRY_BASE_FIXED32( a, flag, base_type, type, name, tag ) SMPP_DEFINE_CLASS_ENTRY_BASE( a, flag, base_type, type, name, tag )
#define SMPP_DEFINE_CLASS_ENTRY_BASE_FIXED64( a, flag, base_type, type, name, tag ) SMPP_DEFINE_CLASS_ENTRY_BASE( a, flag, base_type, type, name, tag )
#define SMPP_DEFINE_CLASS_ENTRY_BASE_ENUM( a, flag, base_type, type, name, tag ) SMPP_DEFINE_CLASS_ENTRY_BASE( a, flag, base_type, type, name, tag )
#define SMPP_DEFINE_CLASS_ENTRY_BASE_DATA( a, flag, base_type, type, name, tag ) SMPP_DEFINE_CLASS_ENTRY_DATA_##type( a, flag, base_type, type, name, tag )

#define SMPP_DEFINE_CLASS_ENTRY_OPTIONAL( a, flag, base_type, type, name, tag ) SMPP_DEFINE_CLASS_ENTRY_BASE_##base_type( a, flag, base_type, type, name, tag )
#define SMPP_DEFINE_CLASS_ENTRY_REQUIRED( a, flag, base_type, type, name, tag ) SMPP_DEFINE_CLASS_ENTRY_BASE_##base_type( a, flag, base_type, type, name, tag )

#define SMPP_DEFINE_CLASS_ENTRY_REPEATED( a, flag, base_type, type, name, tag ) \
// TODO

#define SMPP_DEFINE_CLASS_ENTRY( a, flag, base_type, type, name, tag ) \
SMPP_DEFINE_CLASS_ENTRY_##flag( a, flag, base_type, type, name, tag )

#define SMPP_DEFINE_COPY_ENTRY( a, flag, base_type, type, name, tag ) \
this->name = other.name;

#define SMPP_BIND( name, max_tag ) \
struct name : public smallpp::base_message { \
private: \
	smallpp::bit_set<max_tag> __INTERNAL_tags; \
	SMPP_FIELDS_##name( SMPP_DEFINE_MEMBER_ENTRY, 0 ) \
\
public: \
	name( ) { \
		this->clear( ); \
	}\
\
	name( const name& other ) { \
		SMPP_FIELDS_##name( SMPP_DEFINE_COPY_ENTRY, 0 ) \
	} \
\
	bool parse_from_buffer( const uint8_t* buffer, size_t buffer_size ) override { \
		auto bf = smallpp::bf_reader( buffer, buffer_size ); \
\
		this->__INTERNAL_tags.clear( );\
\
		field_header_s field = { }; \
		while ( this->read_field( bf, field ) ) { \
			switch ( field.number ) { \
				SMPP_FIELDS_##name( SMPP_DEFINE_READ_ENTRY, 0 ) \
				default: \
				{\
					switch ( field.type ) { \
						case smallpp::e_wire_type::varint: \
						{ \
							uint64_t dummy = 0; \
							if ( !this->read_varint( bf, dummy ) ) \
								return false; \
							break; \
						} \
						case smallpp::e_wire_type::fixed64: \
						{ \
							bf.skip( 8 ); \
							break; \
						} \
						case smallpp::e_wire_type::fixed32: \
						{ \
							bf.skip( 4 ); \
							break; \
						} \
						case smallpp::e_wire_type::length_delimited: \
						{ \
							uint64_t length = 0; \
							if ( !this->read_varint( bf, length ) ) \
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
\
		SMPP_FIELDS_##name( SMPP_DEFINE_POST_READ_ENTRY, 0 ) \
\
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
\
	void clear( ) override { \
		this->__INTERNAL_tags.clear( ); \
		SMPP_FIELDS_##name( SMPP_DEFINE_CONSTRUCTOR_ENTRY, 0 ) \
} \
\
	SMPP_FIELDS_##name( SMPP_DEFINE_CLASS_ENTRY, 0 ) \
};