#ifndef JM_XORSTR_HPP
#define JM_XORSTR_HPP

#include <immintrin.h>
#include <cstdint>
#include <cstddef>
#include <utility>
#include <cstring>
#include <array>

#define JM_XORSTR_DISABLE_AVX_INTRINSICS

#if !defined(JM_XORSTR_CIPHER)
// Cipher 0 is the only mode with a constexpr string_storage constructor,
// so it is the only mode that keeps the plaintext literal out of .rdata.
// The salt + second-layer key added below make its XOR much harder to peel
// back statically without switching to a runtime-only cipher.
#define JM_XORSTR_CIPHER 0
#endif

#define JM_XORSTR_SALT_ (static_cast<std::uint32_t>((__COUNTER__ + 1u) * 2654435761u) ^ static_cast<std::uint32_t>(__LINE__ * 40503u) ^ 0x9E3779B9u)

#define xorstr_(str)                                             \
    ::jm::make_xorstr<JM_XORSTR_SALT_>(                          \
        []() { return str; },                                    \
        std::make_index_sequence<sizeof(str) / sizeof(*str)>{},  \
        std::make_index_sequence<::jm::detail::_cipher_key_count<JM_XORSTR_CIPHER, ::jm::detail::_buffer_size<sizeof(str)>()>::value>{})
#define xorstr(str) xorstr_(str).crypt_get()

#ifdef _MSC_VER
#define XORSTR_FORCEINLINE __forceinline
#else
#define XORSTR_FORCEINLINE __attribute__((always_inline))
#endif

#if !defined(XORSTR_ALLOW_DATA)

#if defined(__clang__) || defined(__GNUC__)
#define XORSTR_VOLATILE volatile
#endif

#endif
#ifndef XORSTR_VOLATILE
#define XORSTR_VOLATILE
#endif

namespace jm
{

	namespace detail
	{

		template<std::size_t S>
		struct unsigned_;

		template<>
		struct unsigned_<1>
		{
			using type = std::uint8_t;
		};
		template<>
		struct unsigned_<2>
		{
			using type = std::uint16_t;
		};
		template<>
		struct unsigned_<4>
		{
			using type = std::uint32_t;
		};

		template<auto C, auto...>
		struct pack_value_type
		{
			using type = decltype( C );
		};

		template<std::size_t Size>
		constexpr std::size_t _buffer_size( )
		{
			return ( ( Size / 16 ) + ( Size % 16 != 0 ) ) * 2;
		}

		template<int Cipher, std::size_t Buf>
		struct _cipher_key_count
		{
			static constexpr std::size_t value = ( Cipher == 1 ) ? ( Buf >= 2 ? Buf : 2 ) : ( Cipher == 2 ) ? ( Buf >= 6 ? Buf : 6 ) : ( Cipher == 3 ) ? ( Buf >= 7 ? Buf : 7 ) : Buf;
		};

		namespace aes
		{
			static constexpr std::uint8_t sbox[256] = {
				0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
				0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
				0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
				0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
				0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
				0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
				0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
				0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
				0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
				0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
				0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
				0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
				0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
				0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
				0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
				0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16
			};
			static constexpr std::uint8_t rsbox[256] = {
				0x52,0x09,0x6a,0xd5,0x30,0x36,0xa5,0x38,0xbf,0x40,0xa3,0x9e,0x81,0xf3,0xd7,0xfb,
				0x7c,0xe3,0x39,0x82,0x9b,0x2f,0xff,0x87,0x34,0x8e,0x43,0x44,0xc4,0xde,0xe9,0xcb,
				0x54,0x7b,0x94,0x32,0xa6,0xc2,0x23,0x3d,0xee,0x4c,0x95,0x0b,0x42,0xfa,0xc3,0x4e,
				0x08,0x2e,0xa1,0x66,0x28,0xd9,0x24,0xb2,0x76,0x5b,0xa2,0x49,0x6d,0x8b,0xd1,0x25,
				0x72,0xf8,0xf6,0x64,0x86,0x68,0x98,0x16,0xd4,0xa4,0x5c,0xcc,0x5d,0x65,0xb6,0x92,
				0x6c,0x70,0x48,0x50,0xfd,0xed,0xb9,0xda,0x5e,0x15,0x46,0x57,0xa7,0x8d,0x9d,0x84,
				0x90,0xd8,0xab,0x00,0x8c,0xbc,0xd3,0x0a,0xf7,0xe4,0x58,0x05,0xb8,0xb3,0x45,0x06,
				0xd0,0x2c,0x1e,0x8f,0xca,0x3f,0x0f,0x02,0xc1,0xaf,0xbd,0x03,0x01,0x13,0x8a,0x6b,
				0x3a,0x91,0x11,0x41,0x4f,0x67,0xdc,0xea,0x97,0xf2,0xcf,0xce,0xf0,0xb4,0xe6,0x73,
				0x96,0xac,0x74,0x22,0xe7,0xad,0x35,0x85,0xe2,0xf9,0x37,0xe8,0x1c,0x75,0xdf,0x6e,
				0x47,0xf1,0x1a,0x71,0x1d,0x29,0xc5,0x89,0x6f,0xb7,0x62,0x0e,0xaa,0x18,0xbe,0x1b,
				0xfc,0x56,0x3e,0x4b,0xc6,0xd2,0x79,0x20,0x9a,0xdb,0xc0,0xfe,0x78,0xcd,0x5a,0xf4,
				0x1f,0xdd,0xa8,0x33,0x88,0x07,0xc7,0x31,0xb1,0x12,0x10,0x59,0x27,0x80,0xec,0x5f,
				0x60,0x51,0x7f,0xa9,0x19,0xb5,0x4a,0x0d,0x2d,0xe5,0x7a,0x9f,0x93,0xc9,0x9c,0xef,
				0xa0,0xe0,0x3b,0x4d,0xae,0x2a,0xf5,0xb0,0xc8,0xeb,0xbb,0x3c,0x83,0x53,0x99,0x61,
				0x17,0x2b,0x04,0x7e,0xba,0x77,0xd6,0x26,0xe1,0x69,0x14,0x63,0x55,0x21,0x0c,0x7d
			};
			static constexpr std::uint8_t rcon[11] = { 0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36 };
			constexpr std::uint8_t gf2_mul(std::uint8_t a, std::uint8_t b) {
				std::uint8_t p = 0;
				for ( int i = 0; i < 8; ++i ) { if ( b & 1 ) p ^= a; a = ( a << 1 ) ^ ( a & 0x80 ? 0x1b : 0 ); b >>= 1; }
				return p;
			}
			constexpr void sub_bytes( std::uint8_t* s ) {
				for ( int i = 0; i < 16; ++i ) s[i] = sbox[s[i]];
			}
			constexpr void inv_sub_bytes( std::uint8_t* s ) {
				for ( int i = 0; i < 16; ++i ) s[i] = rsbox[s[i]];
			}
			constexpr void shift_rows( std::uint8_t* s ) {
				std::uint8_t t = s[1]; s[1]=s[5]; s[5]=s[9]; s[9]=s[13]; s[13]=t;
				t = s[2]; s[2]=s[10]; s[10]=t; t = s[6]; s[6]=s[14]; s[14]=t;
				t = s[3]; s[3]=s[15]; s[15]=s[11]; s[11]=s[7]; s[7]=t;
			}
			constexpr void inv_shift_rows( std::uint8_t* s ) {
				std::uint8_t t = s[1]; s[1]=s[13]; s[13]=s[9]; s[9]=s[5]; s[5]=t;
				t = s[2]; s[2]=s[10]; s[10]=t; t = s[6]; s[6]=s[14]; s[14]=t;
				t = s[3]; s[3]=s[7]; s[7]=s[11]; s[11]=s[15]; s[15]=t;
			}
			constexpr void mix_columns( std::uint8_t* s ) {
				for ( int c = 0; c < 4; ++c ) {
					std::uint8_t a = s[c*4], b = s[c*4+1], c_ = s[c*4+2], d = s[c*4+3];
					s[c*4]   = gf2_mul(a,2)^gf2_mul(b,3)^c_^d;
					s[c*4+1] = a^gf2_mul(b,2)^gf2_mul(c_,3)^d;
					s[c*4+2] = a^b^gf2_mul(c_,2)^gf2_mul(d,3);
					s[c*4+3] = gf2_mul(a,3)^b^c_^gf2_mul(d,2);
				}
			}
			constexpr void inv_mix_columns( std::uint8_t* s ) {
				for ( int c = 0; c < 4; ++c ) {
					std::uint8_t a = s[c*4], b = s[c*4+1], c_ = s[c*4+2], d = s[c*4+3];
					s[c*4]   = gf2_mul(a,0x0e)^gf2_mul(b,0x0b)^gf2_mul(c_,0x0d)^gf2_mul(d,0x09);
					s[c*4+1] = gf2_mul(a,0x09)^gf2_mul(b,0x0e)^gf2_mul(c_,0x0b)^gf2_mul(d,0x0d);
					s[c*4+2] = gf2_mul(a,0x0d)^gf2_mul(b,0x09)^gf2_mul(c_,0x0e)^gf2_mul(d,0x0b);
					s[c*4+3] = gf2_mul(a,0x0b)^gf2_mul(b,0x0d)^gf2_mul(c_,0x09)^gf2_mul(d,0x0e);
				}
			}
			constexpr void add_round_key( std::uint8_t* s, const std::uint8_t* rk ) {
				for ( int i = 0; i < 16; ++i ) s[i] ^= rk[i];
			}
			inline std::array<std::uint8_t, 176> key_expansion( const std::uint8_t* key ) {
				std::array<std::uint8_t, 176> rk{};
				for ( int i = 0; i < 16; ++i ) rk[i] = key[i];
				for ( int i = 4; i < 44; ++i ) {
					std::uint8_t t[4] = { rk[(i-1)*4], rk[(i-1)*4+1], rk[(i-1)*4+2], rk[(i-1)*4+3] };
					if ( i % 4 == 0 ) {
						std::uint8_t u = t[0]; t[0]=sbox[t[1]]^rcon[i/4]; t[1]=sbox[t[2]]; t[2]=sbox[t[3]]; t[3]=sbox[u];
					}
					for ( int j = 0; j < 4; ++j ) rk[i*4+j] = rk[(i-4)*4+j] ^ t[j];
				}
				return rk;
			}
			inline std::array<std::uint8_t, 16> encrypt_block( const std::uint8_t* key, const std::uint8_t* in ) {
				std::uint8_t state[16];
				for ( int i = 0; i < 16; ++i ) state[i] = in[i];
				std::array<std::uint8_t, 176> rk = key_expansion( key );
				add_round_key( state, rk.data() );
				for ( int r = 1; r < 10; ++r ) { sub_bytes(state); shift_rows(state); mix_columns(state); add_round_key(state, rk.data() + r*16 ); }
				sub_bytes(state); shift_rows(state); add_round_key(state, rk.data() + 160 );
				std::array<std::uint8_t, 16> out{};
				for ( int i = 0; i < 16; ++i ) out[i] = state[i];
				return out;
			}
			inline void decrypt_block( const std::uint8_t* key, const std::uint8_t* in, std::uint8_t* out ) {
				std::uint8_t state[16];
				for ( int i = 0; i < 16; ++i ) state[i] = in[i];
				std::array<std::uint8_t, 176> rk = key_expansion( key );
				add_round_key( state, rk.data() + 160 );
				for ( int r = 9; r >= 1; --r ) { inv_sub_bytes(state); inv_shift_rows(state); add_round_key(state, rk.data() + r*16 ); inv_mix_columns(state); }
				inv_sub_bytes(state); inv_shift_rows(state); add_round_key(state, rk.data() );
				for ( int i = 0; i < 16; ++i ) out[i] = state[i];
			}
		}

		namespace chacha
		{
			inline std::uint32_t rotl32( std::uint32_t v, int c ) { return ( v << c ) | ( v >> ( 32 - c ) ); }
			inline void quarter( std::uint32_t& a, std::uint32_t& b, std::uint32_t& c, std::uint32_t& d ) {
				a += b; d ^= a; d = rotl32(d,16);
				c += d; b ^= c; b = rotl32(b,12);
				a += b; d ^= a; d = rotl32(d,8);
				c += d; b ^= c; b = rotl32(b,7);
			}
			inline std::array<std::uint32_t, 16> block( const std::uint32_t* key, const std::uint32_t* nonce, std::uint32_t counter ) {
				std::uint32_t x[16];
				x[0]=0x61707865u; x[1]=0x3320646eu; x[2]=0x79622d32u; x[3]=0x6b206574u;
				for ( int i = 0; i < 8; ++i ) x[4+i] = key[i];
				x[12] = counter; x[13]=nonce[0]; x[14]=nonce[1]; x[15]=nonce[2];
				std::uint32_t w[16]; for ( int i = 0; i < 16; ++i ) w[i] = x[i];
				for ( int r = 0; r < 10; ++r ) {
					quarter(x[0],x[4],x[8],x[12]); quarter(x[1],x[5],x[9],x[13]); quarter(x[2],x[6],x[10],x[14]); quarter(x[3],x[7],x[11],x[15]);
					quarter(x[0],x[5],x[10],x[15]); quarter(x[1],x[6],x[11],x[12]); quarter(x[2],x[7],x[8],x[13]); quarter(x[3],x[4],x[9],x[14]);
				}
				std::array<std::uint32_t, 16> out{};
				for ( int i = 0; i < 16; ++i ) out[i] = w[i] + x[i];
				return out;
			}
			inline std::array<std::uint32_t, 8> hchacha_block( const std::uint32_t* key, const std::uint32_t* in ) {
				std::uint32_t x[16];
				x[0]=0x61707865u; x[1]=0x3320646eu; x[2]=0x79622d32u; x[3]=0x6b206574u;
				for ( int i = 0; i < 8; ++i ) x[4+i] = key[i];
				for ( int i = 0; i < 4; ++i ) x[12+i] = in[i];
				for ( int r = 0; r < 10; ++r ) {
					quarter(x[0],x[4],x[8],x[12]); quarter(x[1],x[5],x[9],x[13]); quarter(x[2],x[6],x[10],x[14]); quarter(x[3],x[7],x[11],x[15]);
					quarter(x[0],x[5],x[10],x[15]); quarter(x[1],x[6],x[11],x[12]); quarter(x[2],x[7],x[8],x[13]); quarter(x[3],x[4],x[9],x[14]);
				}
				return { x[0], x[1], x[2], x[3], x[12], x[13], x[14], x[15] };
			}
		}

		template<auto... Cs>
		struct tstring_
		{
			using value_type = typename pack_value_type<Cs...>::type;
			constexpr static std::size_t size = sizeof...( Cs );
			constexpr static value_type  str[ size ] = { Cs... };

			constexpr static std::size_t buffer_size = _buffer_size<sizeof( str )>( );
			constexpr static std::size_t buffer_align =
#ifndef JM_XORSTR_DISABLE_AVX_INTRINSICS
			( ( sizeof( str ) > 16 ) ? 32 : 16 );
#else
				16;
#endif
		};

		template<std::size_t I, std::uint64_t K>
		struct _ki
		{
			constexpr static std::size_t   idx = I;
			constexpr static std::uint64_t key = K;
		};

		template<std::uint32_t Seed>
		constexpr std::uint32_t key4( ) noexcept
		{
			std::uint32_t value = Seed;
			for ( char c : __TIME__ )
				value = static_cast< std::uint32_t >( ( value ^ c ) * 16777619ull );
			for ( char c : __DATE__ )
				value = static_cast< std::uint32_t >( ( value ^ static_cast<std::uint32_t>( c ) ) * 2246822519ull );
			value ^= ( value >> 13 );
			value = static_cast< std::uint32_t >( value * 3266489917ull );
			value ^= ( value >> 15 );
			return value;
		}

		template<std::size_t S, std::uint32_t Salt>
		constexpr std::uint64_t key8( )
		{
			constexpr auto first_part = key4<2166136261u + static_cast<std::uint32_t>( S ) + Salt>( );
			constexpr auto second_part = key4<first_part ^ ( Salt * 0x85EBCA6Bu )>( );
			constexpr auto third_part = key4<second_part ^ static_cast<std::uint32_t>( S * 0xC2B2AE35u )>( );
			return ( static_cast< std::uint64_t >( first_part ^ third_part ) << 32 ) | ( second_part ^ ( third_part * 0x27D4EB2Fu ) );
		}

		template<std::uint32_t Salt>
		constexpr std::uint64_t second_layer_key( std::size_t idx )
		{
			std::uint64_t v = 0xCBF29CE484222325ull ^ ( static_cast<std::uint64_t>( Salt ) * 0x100000001B3ull );
			v ^= static_cast<std::uint64_t>( idx + 1 );
			v *= 0x100000001B3ull;
			v ^= ( v >> 33 );
			v *= 0xFF51AFD7ED558CCDull;
			v ^= ( v >> 33 );
			v *= 0xC4CEB9FE1A85EC53ull;
			v ^= ( v >> 33 );
			return v;
		}

		template<class T, int Cipher, std::uint32_t Salt, class... KeyTypes>
		struct string_storage
		{
			std::uint64_t storage[ T::buffer_size ];

			XORSTR_FORCEINLINE string_storage( ) noexcept : storage {}
			{
				if constexpr ( Cipher == 0 ) {
					static_cast<void>( std::initializer_list<std::uint64_t>{ ( storage[ KeyTypes::idx ] = KeyTypes::key )... } );
					using cast_type = typename unsigned_<sizeof( typename T::value_type )>::type;
					constexpr auto value_size = sizeof( typename T::value_type );
					for ( std::size_t i = 0; i < T::size; ++i )
						storage[ i / ( 8 / value_size ) ] ^=
							( std::uint64_t { static_cast< cast_type >( T::str[ i ] ) }
							  << ( ( i % ( 8 / value_size ) ) * 8 * value_size ) );
					for ( std::size_t i = 0; i < T::buffer_size; ++i )
						storage[ i ] ^= second_layer_key<Salt>( i );
				} else if constexpr ( Cipher == 1 ) {
					constexpr std::uint64_t key_arr[ sizeof...( KeyTypes ) ] = { KeyTypes::key... };
					std::uint8_t key[16];
					for ( int i = 0; i < 8; ++i ) key[i] = static_cast<std::uint8_t>( ( key_arr[0] >> ( i * 8 ) ) & 0xff );
					for ( int i = 0; i < 8; ++i ) key[8+i] = static_cast<std::uint8_t>( ( key_arr[1] >> ( i * 8 ) ) & 0xff );
					constexpr std::size_t nblocks = ( T::buffer_size * 8 ) / 16;
					for ( std::size_t b = 0; b < nblocks; ++b ) {
						std::uint8_t in[16] = {};
						for ( int i = 0; i < 16; ++i ) in[i] = ( b * 16 + i < T::size ) ? static_cast<std::uint8_t>( T::str[ b * 16 + i ] ) : 0;
						std::array<std::uint8_t, 16> out = aes::encrypt_block( key, in );
						storage[ b * 2 ] = static_cast<std::uint64_t>( out[0] ) | ( static_cast<std::uint64_t>( out[1] ) << 8 ) | ( static_cast<std::uint64_t>( out[2] ) << 16 ) | ( static_cast<std::uint64_t>( out[3] ) << 24 ) | ( static_cast<std::uint64_t>( out[4] ) << 32 ) | ( static_cast<std::uint64_t>( out[5] ) << 40 ) | ( static_cast<std::uint64_t>( out[6] ) << 48 ) | ( static_cast<std::uint64_t>( out[7] ) << 56 );
						storage[ b * 2 + 1 ] = static_cast<std::uint64_t>( out[8] ) | ( static_cast<std::uint64_t>( out[9] ) << 8 ) | ( static_cast<std::uint64_t>( out[10] ) << 16 ) | ( static_cast<std::uint64_t>( out[11] ) << 24 ) | ( static_cast<std::uint64_t>( out[12] ) << 32 ) | ( static_cast<std::uint64_t>( out[13] ) << 40 ) | ( static_cast<std::uint64_t>( out[14] ) << 48 ) | ( static_cast<std::uint64_t>( out[15] ) << 56 );
					}
					for ( std::size_t i = 0; i < T::buffer_size; ++i )
						storage[ i ] ^= second_layer_key<Salt>( i );
				} else if constexpr ( Cipher == 2 || Cipher == 3 ) {
					constexpr std::uint64_t key_arr[ sizeof...( KeyTypes ) ] = { KeyTypes::key... };
					std::uint32_t key32[8], nonce[6] = {};
					for ( int i = 0; i < 8; ++i ) key32[i] = static_cast<std::uint32_t>( ( key_arr[i/2] >> ( ( i % 2 ) * 32 ) ) & 0xffffffffu );
					for ( int i = 0; i < ( Cipher == 3 ? 6 : 3 ); ++i ) nonce[i] = static_cast<std::uint32_t>( ( key_arr[4 + i/2] >> ( ( i % 2 ) * 32 ) ) & 0xffffffffu );
					std::uint32_t subkey[8];
					if constexpr ( Cipher == 3 ) {
						std::array<std::uint32_t, 8> subkey_arr = chacha::hchacha_block( key32, nonce );
						for ( int i = 0; i < 8; ++i ) subkey[i] = subkey_arr[i];
						nonce[0] = nonce[4]; nonce[1] = nonce[5]; nonce[2] = 0;
					} else {
						for ( int i = 0; i < 8; ++i ) subkey[i] = key32[i];
					}
					constexpr std::size_t total_bytes = T::buffer_size * 8;
					std::size_t off = 0;
					for ( std::uint32_t counter = 0; off < total_bytes; ++counter ) {
						std::array<std::uint32_t, 16> keystream = chacha::block( subkey, nonce, counter );
						for ( int i = 0; i < 16 && off < total_bytes; ++i, off += 4 ) {
							std::uint8_t k[4] = { static_cast<std::uint8_t>( keystream[i] & 0xff ), static_cast<std::uint8_t>( ( keystream[i] >> 8 ) & 0xff ), static_cast<std::uint8_t>( ( keystream[i] >> 16 ) & 0xff ), static_cast<std::uint8_t>( ( keystream[i] >> 24 ) & 0xff ) };
							std::uint8_t p[4];
							for ( int j = 0; j < 4; ++j ) p[j] = ( off + j < T::size ) ? static_cast<std::uint8_t>( T::str[ off + j ] ) : 0;
							std::uint64_t val = static_cast<std::uint64_t>( p[0] ^ k[0] ) | ( static_cast<std::uint64_t>( p[1] ^ k[1] ) << 8 ) | ( static_cast<std::uint64_t>( p[2] ^ k[2] ) << 16 ) | ( static_cast<std::uint64_t>( p[3] ^ k[3] ) << 24 );
							std::uint64_t& st = storage[ off / 8 ];
							if ( off % 8 == 0 ) st = ( st & 0xffffffff00000000ull ) | val; else st = ( st & 0xffffffffull ) | ( val << 32 );
						}
					}
					for ( std::size_t i = 0; i < T::buffer_size; ++i )
						storage[ i ] ^= second_layer_key<Salt>( i );
				}
			}
		};

		template<class T, std::uint32_t Salt, class... KeyTypes>
		struct string_storage<T, 0, Salt, KeyTypes...>
		{
			std::uint64_t storage[ T::buffer_size ];

			XORSTR_FORCEINLINE constexpr string_storage( ) noexcept : storage { KeyTypes::key... }
			{
				using cast_type = typename unsigned_<sizeof( typename T::value_type )>::type;
				constexpr auto value_size = sizeof( typename T::value_type );
				for ( std::size_t i = 0; i < T::size; ++i )
					storage[ i / ( 8 / value_size ) ] ^=
						( std::uint64_t { static_cast< cast_type >( T::str[ i ] ) }
						  << ( ( i % ( 8 / value_size ) ) * 8 * value_size ) );
				for ( std::size_t i = 0; i < T::buffer_size; ++i )
					storage[ i ] ^= second_layer_key<Salt>( i );
			}
		};

	}

	template<class T, int Cipher, std::uint32_t Salt, class... Keys>
	class xor_string
	{
		alignas( T::buffer_align ) std::uint64_t _storage[ T::buffer_size ];

		XORSTR_FORCEINLINE void _crypt_256_single( const std::uint64_t * keys,
			std::uint64_t * storage ) noexcept
		{
			_mm256_store_si256(
				reinterpret_cast< __m256i * >( storage ),
				_mm256_xor_si256(
					_mm256_load_si256( reinterpret_cast< const __m256i * >( storage ) ),
					_mm256_load_si256( reinterpret_cast< const __m256i * >( keys ) ) ) );
		}

		template<std::size_t... Idxs>
		XORSTR_FORCEINLINE void _crypt_256( const std::uint64_t * keys,
			std::index_sequence<Idxs...> ) noexcept
		{
			( _crypt_256_single( keys + Idxs * 4, _storage + Idxs * 4 ), ... );
		}

		XORSTR_FORCEINLINE void _crypt_128_single( const std::uint64_t * keys,
			std::uint64_t * storage ) noexcept
		{
			_mm_store_si128(
				reinterpret_cast< __m128i * >( storage ),
				_mm_xor_si128( _mm_load_si128( reinterpret_cast< const __m128i * >( storage ) ),
					_mm_load_si128( reinterpret_cast< const __m128i * >( keys ) ) ) );
		}

		template<std::size_t... Idxs>
		XORSTR_FORCEINLINE void _crypt_128( const std::uint64_t * keys,
			std::index_sequence<Idxs...> ) noexcept
		{
			( _crypt_128_single( keys + Idxs * 2, _storage + Idxs * 2 ), ... );
		}

		XORSTR_FORCEINLINE void _copy( ) noexcept
		{
			if constexpr ( Cipher == 0 ) {
				constexpr detail::string_storage<T, Cipher, Salt, Keys...> st;
				for ( std::size_t i = 0; i < T::buffer_size; ++i )
					( const_cast< XORSTR_VOLATILE std::uint64_t * >( _storage ) )[ i ] = st.storage[ i ];
			} else {
				detail::string_storage<T, Cipher, Salt, Keys...> st;
				for ( std::size_t i = 0; i < T::buffer_size; ++i )
					( const_cast< XORSTR_VOLATILE std::uint64_t * >( _storage ) )[ i ] = st.storage[ i ];
			}
		}

		XORSTR_FORCEINLINE void _strip_second_layer( ) noexcept
		{
			for ( std::size_t i = 0; i < T::buffer_size; ++i )
				( const_cast< XORSTR_VOLATILE std::uint64_t * >( _storage ) )[ i ] ^= detail::second_layer_key<Salt>( i );
		}

		XORSTR_FORCEINLINE void _decrypt_aes( ) noexcept
		{
			std::uint64_t key_arr[ sizeof...( Keys ) ] = { Keys::key... };
			std::uint8_t key[16];
			for ( int i = 0; i < 8; ++i ) key[i] = static_cast<std::uint8_t>( ( key_arr[0] >> ( i * 8 ) ) & 0xff );
			for ( int i = 0; i < 8; ++i ) key[8+i] = static_cast<std::uint8_t>( ( key_arr[1] >> ( i * 8 ) ) & 0xff );
			constexpr std::size_t nblocks = ( T::buffer_size * 8 ) / 16;
			for ( std::size_t b = 0; b < nblocks; ++b ) {
				std::uint8_t in[16], out[16];
				std::uint64_t lo = _storage[ b * 2 ], hi = _storage[ b * 2 + 1 ];
				for ( int i = 0; i < 8; ++i ) { in[i] = static_cast<std::uint8_t>( lo >> ( i * 8 ) ); in[8+i] = static_cast<std::uint8_t>( hi >> ( i * 8 ) ); }
				detail::aes::decrypt_block( key, in, out );
				_storage[ b * 2 ] = static_cast<std::uint64_t>( out[0] ) | ( static_cast<std::uint64_t>( out[1] ) << 8 ) | ( static_cast<std::uint64_t>( out[2] ) << 16 ) | ( static_cast<std::uint64_t>( out[3] ) << 24 ) | ( static_cast<std::uint64_t>( out[4] ) << 32 ) | ( static_cast<std::uint64_t>( out[5] ) << 40 ) | ( static_cast<std::uint64_t>( out[6] ) << 48 ) | ( static_cast<std::uint64_t>( out[7] ) << 56 );
				_storage[ b * 2 + 1 ] = static_cast<std::uint64_t>( out[8] ) | ( static_cast<std::uint64_t>( out[9] ) << 8 ) | ( static_cast<std::uint64_t>( out[10] ) << 16 ) | ( static_cast<std::uint64_t>( out[11] ) << 24 ) | ( static_cast<std::uint64_t>( out[12] ) << 32 ) | ( static_cast<std::uint64_t>( out[13] ) << 40 ) | ( static_cast<std::uint64_t>( out[14] ) << 48 ) | ( static_cast<std::uint64_t>( out[15] ) << 56 );
			}
		}

		XORSTR_FORCEINLINE void _decrypt_chacha( ) noexcept
		{
			std::uint64_t key_arr[ sizeof...( Keys ) ] = { Keys::key... };
			std::uint32_t key32[8], nonce[6] = {};
			for ( int i = 0; i < 8; ++i ) key32[i] = static_cast<std::uint32_t>( ( key_arr[i/2] >> ( ( i % 2 ) * 32 ) ) & 0xffffffffu );
			for ( int i = 0; i < ( Cipher == 3 ? 6 : 3 ); ++i ) nonce[i] = static_cast<std::uint32_t>( ( key_arr[4 + i/2] >> ( ( i % 2 ) * 32 ) ) & 0xffffffffu );
			const std::uint32_t* subkey_ptr = key32;
			std::array<std::uint32_t, 8> subkey_arr;
			if constexpr ( Cipher == 3 ) {
				subkey_arr = detail::chacha::hchacha_block( key32, nonce );
				subkey_ptr = subkey_arr.data();
				nonce[0] = nonce[4]; nonce[1] = nonce[5]; nonce[2] = 0;
			}
			std::size_t total_bytes = T::buffer_size * 8, off = 0;
			for ( std::uint32_t counter = 0; off < total_bytes; ++counter ) {
				std::array<std::uint32_t, 16> keystream = detail::chacha::block( subkey_ptr, nonce, counter );
				for ( int i = 0; i < 16 && off < total_bytes; ++i, off += 4 ) {
					std::uint64_t& st = _storage[ off / 8 ];
					std::uint64_t val = ( off % 8 == 0 ) ? ( st & 0xffffffffull ) : ( st >> 32 );
					std::uint8_t k[4] = { static_cast<std::uint8_t>( keystream[i] & 0xff ), static_cast<std::uint8_t>( ( keystream[i] >> 8 ) & 0xff ), static_cast<std::uint8_t>( ( keystream[i] >> 16 ) & 0xff ), static_cast<std::uint8_t>( ( keystream[i] >> 24 ) & 0xff ) };
					std::uint8_t c[4] = { static_cast<std::uint8_t>( val & 0xff ), static_cast<std::uint8_t>( ( val >> 8 ) & 0xff ), static_cast<std::uint8_t>( ( val >> 16 ) & 0xff ), static_cast<std::uint8_t>( ( val >> 24 ) & 0xff ) };
					std::uint64_t dec = static_cast<std::uint64_t>( c[0] ^ k[0] ) | ( static_cast<std::uint64_t>( c[1] ^ k[1] ) << 8 ) | ( static_cast<std::uint64_t>( c[2] ^ k[2] ) << 16 ) | ( static_cast<std::uint64_t>( c[3] ^ k[3] ) << 24 );
					if ( off % 8 == 0 ) st = ( st & 0xffffffff00000000ull ) | dec; else st = ( st & 0xffffffffull ) | ( dec << 32 );
				}
			}
		}

	public:
		using value_type = typename T::value_type;
		using size_type = std::size_t;
		using pointer = value_type *;
		using const_pointer = const value_type *;

		XORSTR_FORCEINLINE xor_string( ) noexcept
		{
			_copy( );
		}

		XORSTR_FORCEINLINE constexpr size_type size( ) const noexcept
		{
			return T::size - 1;
		}

		XORSTR_FORCEINLINE void crypt( ) noexcept
		{
			if constexpr ( Cipher == 0 ) {
				_copy( );
				alignas( T::buffer_align ) std::uint64_t keys[ T::buffer_size ];
				static_cast< void >( std::initializer_list<std::uint64_t>{
					( const_cast< XORSTR_VOLATILE std::uint64_t * >( keys ) )[ Keys::idx ] = Keys::key... } );
#ifndef JM_XORSTR_DISABLE_AVX_INTRINSICS
				_crypt_256( keys, std::make_index_sequence<T::buffer_size / 4>{} );
				if constexpr ( T::buffer_size % 4 != 0 )
					_crypt_128( keys, std::index_sequence<T::buffer_size / 2 - 1>{} );
#else
				_crypt_128( keys, std::make_index_sequence<T::buffer_size / 2>{} );
#endif
				_strip_second_layer( );
			} else if constexpr ( Cipher == 1 ) {
				_strip_second_layer( );
				_decrypt_aes( );
			} else if constexpr ( Cipher == 2 || Cipher == 3 ) {
				_strip_second_layer( );
				_decrypt_chacha( );
			}
		}

		XORSTR_FORCEINLINE const_pointer get( ) const noexcept
		{
			return reinterpret_cast< const_pointer >( _storage );
		}

		XORSTR_FORCEINLINE pointer get( ) noexcept
		{
			return reinterpret_cast< pointer >( _storage );
		}

		XORSTR_FORCEINLINE pointer crypt_get( ) noexcept
		{
			crypt( );
			return reinterpret_cast< pointer >( _storage );
		}
	};

	template<std::uint32_t Salt, class Tstr, std::size_t... StringIndices, std::size_t... KeyIndices>
	XORSTR_FORCEINLINE constexpr auto
		make_xorstr( Tstr str_lambda,
			std::index_sequence<StringIndices...>,
			std::index_sequence<KeyIndices...> ) noexcept
	{
		return xor_string<detail::tstring_<str_lambda( )[ StringIndices ]...>,
			JM_XORSTR_CIPHER,
			Salt,
			detail::_ki<KeyIndices, detail::key8<KeyIndices, Salt>( )>...>{};
	}

	namespace lite {

		template<std::size_t N>
		struct encrypted
		{
			char data[N];

			XORSTR_FORCEINLINE void decrypt( char* out, std::uint32_t salt ) const noexcept
			{
				std::uint32_t k = salt ^ 0x9E3779B9u;
				for ( std::size_t i = 0; i < N; ++i ) {
					k = k * 1103515245u + 12345u;
					k ^= ( k >> 13 );
					out[i] = static_cast<char>( static_cast<std::uint8_t>( data[i] ) ^ static_cast<std::uint8_t>( ( k >> 16 ) & 0xffu ) );
				}
			}
		};

		template<std::size_t N>
		constexpr encrypted<N> encrypt( const char ( &s )[N], std::uint32_t salt ) noexcept
		{
			encrypted<N> r{};
			std::uint32_t k = salt ^ 0x9E3779B9u;
			for ( std::size_t i = 0; i < N; ++i ) {
				k = k * 1103515245u + 12345u;
				k ^= ( k >> 13 );
				r.data[i] = static_cast<char>( static_cast<std::uint8_t>( s[i] ) ^ static_cast<std::uint8_t>( ( k >> 16 ) & 0xffu ) );
			}
			return r;
		}

	}

}

// Lightweight xorstr variant. Same compile-time-encrypt / runtime-decrypt
// principle as xorstr(), but the template only depends on the string LENGTH,
// not the content. That collapses N unique instantiations to O(distinct
// lengths), so a header with tens of thousands of entries (native patterns,
// hash names) compiles in seconds instead of hours.
//
// Each call site gets a distinct salt via __COUNTER__ + __LINE__, so identical
// strings at different sites still encrypt to different bytes. The decrypt
// buffer is thread_local so returned pointers stay valid until the next
// xorstr_lite call in the same thread.
#define JM_XORSTR_LITE_SALT_ (static_cast<std::uint32_t>((__COUNTER__ + 1u) * 2246822519u) ^ static_cast<std::uint32_t>(__LINE__ * 40503u) ^ 0xBF58476Du)

#define xorstr_lite(str) ([]() -> const char* {                            \
        constexpr std::uint32_t _xls_salt = JM_XORSTR_LITE_SALT_;          \
        static constexpr auto _xls_enc = ::jm::lite::encrypt( str, _xls_salt ); \
        thread_local char _xls_dec[sizeof(str)];                           \
        _xls_enc.decrypt( _xls_dec, _xls_salt );                           \
        return _xls_dec;                                                   \
    }())

#endif