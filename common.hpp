#ifndef COMMON_HPP
#define COMMON_HPP

#include <iostream>
#include "debugging.hpp"

typedef unsigned char Byte;
typedef unsigned short Word;

#undef assert

#define BL(bit) (1 << (bit))

#define KB 1024

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

#ifndef NULL
	#define NULL 0
#endif

struct Pixel {
	Byte red;
	Byte green;
	Byte blue;
	Byte alpha;
	Pixel(Byte red = 0, Byte green = 0, Byte blue = 0, Byte alpha = 255)
		: red(red), green(green), blue(blue), alpha(alpha) {}
};

#endif