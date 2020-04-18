#ifndef COMMON_HPP
#define COMMON_HPP

#include <iostream>
#include <iomanip>
#include <bitset>
#include <string>

#define toHex3(number, bytes, prefix) (prefix) << std::setfill('0') << std::setw(static_cast<std::streamsize>( (bytes) * 2 )) \
	<< std::hex << (static_cast<int>(number)) << std::dec << std::setw(0)

#define toHex2(number, bytes) toHex3(number, bytes, '$')

#define toHex(number) toHex2(number, sizeof(number))

#define toByte(number) (std::bitset<8>((number)))

template <typename T>
bool inBounds( T x, T y, T left, T top, T width, T height )
{
	return x >= left
		&& y >= top
		&& x < left + width
		&& y < top + height;
}

typedef unsigned int uint;
typedef void(*Callback)(void);

constexpr size_t KB = 0x400;

template<typename T>
inline void writeBinary(std::ostream& out, const T& value) {
	out.write((const char*)&value, sizeof(T));
}

template<typename T>
inline void readBinary(std::istream& in, T& value)
{
	in.read((char*)&value, sizeof(T));
}

template <typename T>
constexpr bool testAnyFlag( T flagset, T flags )
{
	return ( flagset & flags ) != 0;
}

template <typename T>
constexpr bool testAllFlag( T flagset, T flags )
{
	return ( flagset & flags ) == flags;
}

#endif