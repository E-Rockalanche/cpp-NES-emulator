#ifndef COMMON_HPP
#define COMMON_HPP

#include <iostream>
#include <iomanip>
#include <bitset>
#include "debugging.hpp"

#define GET_MACRO(_1, _2, _3, MACRO_NAME, ...) MACRO_NAME

#define toHex3(number, bytes, prefix) (prefix) << std::setfill('0') << std::setw((bytes) * 2) \
	<< std::hex << (static_cast<int>(number)) << std::dec << std::setw(0)
#define toHex2(number, bytes) toHex3(number, bytes, '$')
#define toHex1(number) toHex2(number, sizeof(number))
#define toHex(...) GET_MACRO(__VA_ARGS__, toHex3, toHex2, toHex1)(__VA_ARGS__)

#define toBin(number) (std::bitset<8>((number)))

#ifndef NULL
	#define NULL 0
#endif

typedef unsigned char Byte;
typedef unsigned short Word;

#define BL(bit) (1 << (bit))

#define KB 0x400

#undef assert
#define assert(condition, message) \
	if (!(condition)) { \
		std::cout << "ERROR: " << __FILE__ << " (" << __LINE__ << "): " << message << '\n'; \
		exit(0); \
	}

#define assertBounds(index, size) \
	assert((index) < (size), "index " << (index) << " out of bounds " << (size))

template <class T, T V>
struct Constant {
	operator T() const { return V; }
};

struct Pixel {
	Byte red;
	Byte green;
	Byte blue;

	Pixel(Byte red, Byte green, Byte blue)
		: red(red), green(green), blue(blue) {}
		
	Pixel(unsigned int rgb = 0) : red((rgb >> 16) & 0xff),
						  green((rgb >> 8) & 0xff),
						  blue(rgb & 0xff) {}

	operator int() {
		return red << 16 | green << 8 | blue;
	}
};

#define testFlag(mask, flag) ((bool)((mask) & (flag)))

#endif