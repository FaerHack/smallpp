#pragma once
#include <stdint.h>

// TODO: Describe what it does
#ifdef SMPP_ENABLE_STRINGS
#include <string>
#endif

// TODO: Describe what it does
#ifndef SMPP_MEMCPY
#include <string.h>
#define SMPP_MEMCPY( dst, src, size ) memcpy( dst, src, size )
#endif

// TODO: Describe what it does
#ifndef SMPP_STRLEN
#include <string.h>
#define SMPP_STRLEN( str ) strlen( str )
#endif

// TODO: Describe what it does
#ifndef SMPP_ALLOC
#include <malloc.h>
#define SMPP_ALLOC( size ) _alloca( size )
#endif

// Added just in case
#ifndef SMPP_FREE
#define SMPP_FREE( ... )
#endif

// TODO:
// - Support repeated
// - Maybe instead of true/false return error code? so we can differentiate between out_of_memory and required_field_not_set for example

namespace smallpp {

	// 
	// Small struct to hold any data
	// 

	struct data_s {
		const uint8_t* buffer;
		size_t size;

		data_s( ) {
			buffer = nullptr;
			size = 0;
		}

		data_s( const data_s& other ) {
			this->buffer = other.buffer;
			this->size = other.size;
		}
	};

	// 
	// Base message with base types and few virtual functions
	// 

	class base_message {
	protected:

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

			bool read_varint( uint64_t& value ) {
				bool keep_going = false;
				int shift = 0;

				value = 0;
				do {
					uint8_t next_number = 0;
					if ( !read( next_number ) )
						return false;

					keep_going = ( next_number >= 128 );
					value += ( uint64_t )( next_number & 0x7f ) << shift;
					shift += 7;
				} while ( keep_going );

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
				if ( ( m_buffer + size ) > m_end )
					return false;

				SMPP_MEMCPY( m_buffer, &value, size );

				m_buffer += size;
				return true;
			}

			bool write_data( const uint8_t* buffer, size_t size ) {
				if ( ( m_buffer + size ) > m_end )
					return false;

				SMPP_MEMCPY( m_buffer, buffer, size );

				m_buffer += size;
				return true;
			}

			bool write_varint( uint64_t value ) {
				while ( value >= 128 ) {
					if ( ( m_buffer + 1 ) > m_end )
						return false;

					*m_buffer++ = ( value | 0x80 );
					value >>= 7;
				}

				if ( ( m_buffer + 1 ) > m_end )
					return false;

				*m_buffer++ = value;
				return true;
			}
		};

		// 
		// Tiny helper func to calculate VarInt size on compile-time
		// 

		template <uint32_t _value>
		struct varint_size_s {
			static const size_t value = ( _value < ( 1 << 7 ) ) ? 1
				: ( _value < ( 1 << 14 ) ) ? 2
				: ( _value < ( 1 << 21 ) ) ? 3
				: ( _value < ( 1 << 28 ) ) ? 4
				: 5;
		};

		// 
		// Tiny class to simplify working with bits
		// 

		static constexpr auto BITS_PER_INT = 32;
		static constexpr auto BITSET_INT( int n ) { return ( ( n ) >> 5/*log2(32)*/ ); }

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
		// 
		// 

		enum class e_wire_type : uint64_t {
			varint = 0,
			fixed64 = 1,
			length_delimited = 2,
			fixed32 = 5,
		};

		struct field_header_s {
			e_wire_type type : 3;
			uint64_t number : 61;
		};

		bool read_field( bf_reader& bf, field_header_s& field ) {
			uint64_t value = 0;
			if ( !bf.read_varint( value ) )
				return false;
			
			// TODO: seems weird. probably wont work on some compilers in some cases?
			field = *( field_header_s* )&value;
			return true;
		}

		bool write_field( bf_writer& bf, uint64_t tag, e_wire_type type ) {
			return bf.write_varint( ( tag << 3 ) + ( uint8_t )type );
		}

		uint32_t highest_bit( uint32_t n ) {
#if defined(__GNUC__)
			return 63 ^ static_cast< uint32_t >( __builtin_clzll( n ) );
#elif defined(_MSC_VER) && defined(_M_X64)
			unsigned long where;
			_BitScanReverse64( &where, n );
			return where;
#else
			if ( n == 0 )
				return -1;
			int log = 0;
			uint32_t value = n;
			for ( int i = 4; i >= 0; --i ) {
				int shift = ( 1 << i );
				uint32_t x = value >> shift;
				if ( x != 0 ) {
					value = x;
					log += shift;
				}
			}
			return log;
#endif
		}

		size_t sizeof_varint( uint64_t value ) {
			auto n = highest_bit( value | 0x1 );
			return static_cast< size_t >( ( n * 9 + 73 ) / 64 );
		}

	public:
		virtual bool parse_from_buffer( const uint8_t* buffer, size_t buffer_size ) = 0;
		virtual bool write_to_buffer( uint8_t* buffer, size_t buffer_size ) = 0;
		virtual size_t bytes_size( ) = 0;
		virtual void clear( ) = 0;
	};

}

// 
// Types
// 

#define SMPP_TYPE_INT32 int32_t
#define SMPP_TYPE_INT64 int64_t
#define SMPP_TYPE_UINT32 uint32_t
#define SMPP_TYPE_UINT64 uint64_t
#define SMPP_TYPE_BOOL bool
#define SMPP_TYPE_FLOAT float
#define SMPP_TYPE_DOUBLE double

#define SMPP_WIRE_TYPE_VARINT e_wire_type::varint
#define SMPP_WIRE_TYPE_FIXED32 e_wire_type::fixed32
#define SMPP_WIRE_TYPE_FIXED64 e_wire_type::fixed64
#define SMPP_WIRE_TYPE_DATA e_wire_type::length_delimited
#define SMPP_WIRE_TYPE_MESSAGE e_wire_type::length_delimited
#define SMPP_WIRE_TYPE_ENUM e_wire_type::varint

#define SMPP_BASE_TYPE_VARINT( name ) SMPP_TYPE_##name
#define SMPP_BASE_TYPE_FIXED32( name ) SMPP_TYPE_##name
#define SMPP_BASE_TYPE_FIXED64( name ) SMPP_TYPE_##name
#define SMPP_BASE_TYPE_DATA( name ) smallpp::data_s
#define SMPP_BASE_TYPE_MESSAGE( name ) name
#define SMPP_BASE_TYPE_MESSAGE_DATA( ... ) smallpp::data_s
#define SMPP_BASE_TYPE_ENUM( name ) name

#define SMPP_GET_TYPE( base_type, type ) SMPP_BASE_TYPE_##base_type( type )

// 
// Default values
// 

#define SMPP_DEFAULT_VALUE_VARINT( ... ) 0
#define SMPP_DEFAULT_VALUE_FIXED32( ... ) 0
#define SMPP_DEFAULT_VALUE_FIXED64( ... ) 0
#define SMPP_DEFAULT_VALUE_DATA( ... ) { }
#define SMPP_DEFAULT_VALUE_MESSAGE( ... ) { }
#define SMPP_DEFAULT_VALUE_MESSAGE_DATA( ... ) { }
#define SMPP_DEFAULT_VALUE_ENUM( name ) ( name )0

// 
// Member / copy / clear entries
// 

#define SMPP_DEFINE_MEMBER_ENTRY( a, flag, base_type, type, name, tag ) SMPP_GET_TYPE( base_type, type ) name;
#define SMPP_DEFINE_COPY_ENTRY( a, flag, base_type, type, name, tag ) this->name = other.name;
#define SMPP_DEFINE_CLEAR_ENTRY( a, flag, base_type, type, name, tag ) name = SMPP_DEFAULT_VALUE_##base_type( type );

// 
// Reading
// 

#define SMPP_DEFINE_READ_VARINT( a, base_type, type, name ) \
uint64_t value = 0; \
if ( !( bf.read_varint( value ) ) ) \
	return false; \
\
this->name = ( SMPP_GET_TYPE( base_type, type ) )value;

#define SMPP_DEFINE_READ_TYPE( a, name ) \
if ( !( bf.read( this->name ) ) ) \
	return false;
 
#define SMPP_DEFINE_READ_TYPE_VARINT( a, flag, base_type, type, name, tag ) SMPP_DEFINE_READ_VARINT( a, base_type, type, name )
#define SMPP_DEFINE_READ_TYPE_FIXED32( a, flag, base_type, type, name, tag ) SMPP_DEFINE_READ_TYPE( a, name )
#define SMPP_DEFINE_READ_TYPE_FIXED64( a, flag, base_type, type, name, tag ) SMPP_DEFINE_READ_TYPE( a, name )
#define SMPP_DEFINE_READ_TYPE_ENUM( a, flag, base_type, type, name, tag ) SMPP_DEFINE_READ_VARINT( a, base_type, type, name )

#define SMPP_DEFINE_READ_TYPE_MESSAGE( a, flag, base_type, type, name, tag ) \
uint64_t size = 0; \
if ( !bf.read_varint( size ) ) \
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
if ( !bf.read_varint( size ) ) \
	return false; \
\
this->name.buffer = bf.get_buffer( ); \
this->name.size = size; \
if ( !bf.skip( size ) ) \
	return false;

#define SMPP_DEFINE_READ_TYPE_DATA( a, flag, base_type, type, name, tag ) \
uint64_t size = 0; \
if ( !bf.read_varint( size ) ) \
	return false; \
\
this->name.buffer = bf.get_buffer( ); \
this->name.size = size; \
if ( !bf.skip( size ) ) \
	return false;

#define SMPP_DEFINE_READ_ENTRY( a, flag, base_type, type_, name, tag ) \
case tag: \
{ \
	if ( field.type != SMPP_WIRE_TYPE_##base_type ) return false; \
	this->__INTERNAL_tags.set( tag, true ); \
	SMPP_DEFINE_READ_TYPE_##base_type( a, flag, base_type, type_, name, tag ) \
	break; \
}

// 
// Post-read (for example to make sure required fields are set)
// 

#define SMPP_DEFINE_POST_READ_ENTRY_REQUIRED( a, flag, base_type, type, name, tag ) \
if ( !this->__INTERNAL_tags.is_set( tag ) ) return false;

#define SMPP_DEFINE_POST_READ_ENTRY_OPTIONAL( ... )
#define SMPP_DEFINE_POST_READ_ENTRY_REPEATED( ... )

#define SMPP_DEFINE_POST_READ_ENTRY( a, flag, base_type, type, name, tag ) SMPP_DEFINE_POST_READ_ENTRY_##flag( a, flag, base_type, type, name, tag )

// 
// Writing
// 

#define SMPP_DEFINE_WRITE_ENTRY_DATA( a, flag, base_type, type, name, tag ) \
if ( !bf.write_varint( this->name.size ) || !bf.write_data( this->name.buffer, this->name.size ) ) \
	return false;

#define SMPP_DEFINE_WRITE_ENTRY_VARINT( a, flag, base_type, type, name, tag ) if ( !bf.write_varint( this->name ) ) return false;
#define SMPP_DEFINE_WRITE_ENTRY_FIXED32( a, flag, base_type, type, name, tag ) if ( !bf.write( this->name ) ) return false;
#define SMPP_DEFINE_WRITE_ENTRY_FIXED64( a, flag, base_type, type, name, tag ) if ( !bf.write( this->name ) ) return false;
#define SMPP_DEFINE_WRITE_ENTRY_ENUM( a, flag, base_type, type, name, tag ) if ( !bf.write_varint( this->name ) ) return false;

#define SMPP_DEFINE_WRITE_ENTRY_MESSAGE( a, flag, base_type, type, name, tag ) \
const auto size = this->name.bytes_size( ); \
const auto buffer = ( uint8_t* )SMPP_ALLOC( size ); \
if ( buffer == nullptr ) \
	return false; \
if ( !this->name.write_to_buffer( buffer, size ) ) { \
	SMPP_FREE( buffer ); \
	return false; \
} \
if ( !bf.write_varint( size ) || !bf.write_data( buffer, size ) ) { \
	SMPP_FREE( buffer ); \
	return false; \
} \
SMPP_FREE( buffer );

#define SMPP_DEFINE_WRITE_ENTRY_MESSAGE_DATA( a, flag, base_type, type, name, tag ) \
if ( !bf.write_varint( this->name.size ) || !bf.write_data( this->name.buffer, this->name.size ) ) \
	return false;

#define SMPP_DEFINE_WRITE_ENTRY_REQUIRED( a, flag, base_type, type, name, tag ) \
if ( this->__INTERNAL_tags.is_set( tag ) ) { \
	if ( !this->write_field( bf, tag, SMPP_WIRE_TYPE_##base_type ) ) \
		return false; \
	SMPP_DEFINE_WRITE_ENTRY_##base_type( a, flag, base_type, type, name, tag ) \
} else { \
	return false; \
}

#define SMPP_DEFINE_WRITE_ENTRY_OPTIONAL( a, flag, base_type, type, name, tag ) \
if ( this->__INTERNAL_tags.is_set( tag ) ) { \
	if ( !this->write_field( bf, tag, SMPP_WIRE_TYPE_##base_type ) ) \
		return false; \
	SMPP_DEFINE_WRITE_ENTRY_##base_type( a, flag, base_type, type, name, tag ) \
}

#define SMPP_DEFINE_WRITE_ENTRY_REPEATED( a, flag, base_type, type, name, tag ) \
NOT_IMPLEMENTED_YET

#define SMPP_DEFINE_WRITE_ENTRY( a, flag, base_type, type, name, tag ) SMPP_DEFINE_WRITE_ENTRY_##flag( a, flag, base_type, type, name, tag )

// 
// bytes_size entries
// 

#define SMPP_FIELD_HDR_SIZE( tag ) varint_size_s< tag << 3 >::value

#define SMPP_DEFINE_BYTES_SIZE_ENTRY_VARINT( a, flag, base_type, type, name, tag ) this->sizeof_varint( this->name )
#define SMPP_DEFINE_BYTES_SIZE_ENTRY_FIXED32( a, flag, base_type, type, name, tag ) 4
#define SMPP_DEFINE_BYTES_SIZE_ENTRY_FIXED64( a, flag, base_type, type, name, tag ) 8
#define SMPP_DEFINE_BYTES_SIZE_ENTRY_ENUM( a, flag, base_type, type, name, tag ) SMPP_DEFINE_BYTES_SIZE_ENTRY_VARINT( a, flag, base_type, type, name, tag )
#define SMPP_DEFINE_BYTES_SIZE_ENTRY_MESSAGE( a, flag, base_type, type, name, tag ) this->sizeof_varint( this->name.bytes_size( ) ) + this->name.bytes_size( ) // TODO: GET RID OF DOUBLE bytes_size CALL!!!!
#define SMPP_DEFINE_BYTES_SIZE_ENTRY_MESSAGE_DATA( a, flag, base_type, type, name, tag ) this->sizeof_varint( this->name.size ) + this->name.size
#define SMPP_DEFINE_BYTES_SIZE_ENTRY_DATA( a, flag, base_type, type, name, tag ) this->sizeof_varint( this->name.size ) + this->name.size

#define SMPP_DEFINE_BYTES_SIZE_ENTRY( a, flag, base_type, type, name, tag ) \
if ( this->__INTERNAL_tags.is_set( tag ) ) result += SMPP_FIELD_HDR_SIZE( tag ) + ( SMPP_DEFINE_BYTES_SIZE_ENTRY_##base_type( a, flag, base_type, type, name, tag ) );

// 
// Class entries (functions like set_*/get_*/clear_* and etc)
// 

#define SMPP_DEFINE_CLASS_ENTRY_SHARED( a, flag, base_type, type, name, tag ) \
bool has_##name( ) const { return this->__INTERNAL_tags.is_set( tag ); } \
void clear_##name( ) { return this->__INTERNAL_tags.set( tag, false ); }

#define SMPP_DEFINE_CLASS_ENTRY_BASE( a, flag, base_type, type, name, tag ) \
SMPP_DEFINE_CLASS_ENTRY_SHARED( a, flag, base_type, type, name, tag ) \
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
SMPP_DEFINE_CLASS_ENTRY_SHARED( a, flag, base_type, type, name, tag ) \
\
const type& get_##name( ) const { \
	if ( !this->__INTERNAL_tags.is_set( tag ) ) return { }; \
	return this->name; \
} \
\
void set_##name( const type& value) { \
	this->__INTERNAL_tags.set( tag, true ); \
	this->name = value; \
}

#define SMPP_DEFINE_CLASS_ENTRY_BASE_MESSAGE_DATA( a, flag, base_type, type, name, tag ) \
SMPP_DEFINE_CLASS_ENTRY_SHARED( a, flag, base_type, type, name, tag ) \
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
}

#define SMPP_DEFINE_CLASS_ENTRY_DATA_BYTES( a, flag, base_type, type, name, tag ) \
SMPP_DEFINE_CLASS_ENTRY_SHARED( a, flag, base_type, type, name, tag ) \
\
const decltype( name )& get_##name( ) const { \
	return this->name; \
} \
\
void set_##name( decltype( name ) value ) { \
	this->__INTERNAL_tags.set( tag, true ); \
	this->name = value; \
}

#ifdef SMPP_ENABLE_STRINGS
#define SMPP_DEFINE_CLASS_ENTRY_DATA_STRING_IMPLEMENTATION( a, flag, base_type, type, name, tag ) \
std::string get_##name( ) const { \
	return std::string( ( const char* )this->name.buffer, this->name.size ); \
} \
\
void set_##name( const std::string& value ) { \
	this->__INTERNAL_tags.set( tag, true ); \
	this->name.buffer = ( const uint8_t* )value.data( ); \
	this->name.size = value.size( ); \
} \
void set_##name( const char* buffer, int length = -1 ) { \
	this->__INTERNAL_tags.set( tag, true ); \
	this->name.buffer = ( const uint8_t* )buffer; \
	this->name.size = length != -1 ? length : SMPP_STRLEN( buffer ); \
}
#else
#define SMPP_DEFINE_CLASS_ENTRY_DATA_STRING_IMPLEMENTATION( a, flag, base_type, type, name, tag ) \
const decltype( name )& get_##name( ) const { \
	return this->name; \
} \
\
void set_##name( const char* buffer, int length = -1 ) { \
	this->__INTERNAL_tags.set( tag, true ); \
	this->name.buffer = ( const uint8_t* )buffer; \
	this->name.size = length != -1 ? length : SMPP_STRLEN( buffer ); \
}
#endif

#define SMPP_DEFINE_CLASS_ENTRY_DATA_STRING( a, flag, base_type, type, name, tag ) \
SMPP_DEFINE_CLASS_ENTRY_SHARED( a, flag, base_type, type, name, tag ) \
SMPP_DEFINE_CLASS_ENTRY_DATA_STRING_IMPLEMENTATION( a, flag, base_type, type, name, tag )

#define SMPP_DEFINE_CLASS_ENTRY_BASE_VARINT( a, flag, base_type, type, name, tag ) SMPP_DEFINE_CLASS_ENTRY_BASE( a, flag, base_type, type, name, tag )
#define SMPP_DEFINE_CLASS_ENTRY_BASE_FIXED32( a, flag, base_type, type, name, tag ) SMPP_DEFINE_CLASS_ENTRY_BASE( a, flag, base_type, type, name, tag )
#define SMPP_DEFINE_CLASS_ENTRY_BASE_FIXED64( a, flag, base_type, type, name, tag ) SMPP_DEFINE_CLASS_ENTRY_BASE( a, flag, base_type, type, name, tag )
#define SMPP_DEFINE_CLASS_ENTRY_BASE_ENUM( a, flag, base_type, type, name, tag ) SMPP_DEFINE_CLASS_ENTRY_BASE( a, flag, base_type, type, name, tag )
#define SMPP_DEFINE_CLASS_ENTRY_BASE_DATA( a, flag, base_type, type, name, tag ) SMPP_DEFINE_CLASS_ENTRY_DATA_##type( a, flag, base_type, type, name, tag )

#define SMPP_DEFINE_CLASS_ENTRY_OPTIONAL( a, flag, base_type, type, name, tag ) SMPP_DEFINE_CLASS_ENTRY_BASE_##base_type( a, flag, base_type, type, name, tag )
#define SMPP_DEFINE_CLASS_ENTRY_REQUIRED( a, flag, base_type, type, name, tag ) SMPP_DEFINE_CLASS_ENTRY_BASE_##base_type( a, flag, base_type, type, name, tag )
#define SMPP_DEFINE_CLASS_ENTRY_REPEATED( a, flag, base_type, type, name, tag ) NOT_IMPLEMENTED_YET

#define SMPP_DEFINE_CLASS_ENTRY( a, flag, base_type, type, name, tag ) SMPP_DEFINE_CLASS_ENTRY_##flag( a, flag, base_type, type, name, tag )

// 
// Message struct
// 

#define SMPP_BIND( name, max_tag ) \
struct name : public smallpp::base_message { \
private: \
	bit_set<max_tag> __INTERNAL_tags; \
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
		auto bf = bf_reader( buffer, buffer_size ); \
\
		this->__INTERNAL_tags.clear( ); \
\
		field_header_s field = { }; \
		while ( this->read_field( bf, field ) ) { \
			switch ( field.number ) { \
				SMPP_FIELDS_##name( SMPP_DEFINE_READ_ENTRY, 0 ) \
				default: \
				{ \
					switch ( field.type ) { \
						case e_wire_type::varint: \
						{ \
							uint64_t dummy = 0; \
							if ( !bf.read_varint( dummy ) ) \
								return false; \
							break; \
						} \
						case e_wire_type::fixed64: \
						{ \
							bf.skip( 8 ); \
							break; \
						} \
						case e_wire_type::fixed32: \
						{ \
							bf.skip( 4 ); \
							break; \
						} \
						case e_wire_type::length_delimited: \
						{ \
							uint64_t length = 0; \
							if ( !bf.read_varint( length ) ) \
								return false; \
							bf.skip( length ); \
							break; \
						} \
						default: \
							return false; \
					} \
				} \
			} \
		} \
\
		SMPP_FIELDS_##name( SMPP_DEFINE_POST_READ_ENTRY, 0 ) \
\
		return true; \
	} \
\
	bool write_to_buffer( uint8_t* buffer, size_t buffer_size ) override { \
		auto bf = bf_writer( buffer, buffer_size ); \
		SMPP_FIELDS_##name( SMPP_DEFINE_WRITE_ENTRY, 0 ) \
		return true; \
	} \
\
	size_t bytes_size( ) override { \
		size_t result = 0; \
		SMPP_FIELDS_##name( SMPP_DEFINE_BYTES_SIZE_ENTRY, 0 ) \
		return result; \
	} \
\
	void clear( ) override { \
		this->__INTERNAL_tags.clear( ); \
		SMPP_FIELDS_##name( SMPP_DEFINE_CLEAR_ENTRY, 0 ) \
	} \
\
	SMPP_FIELDS_##name( SMPP_DEFINE_CLASS_ENTRY, 0 ) \
};