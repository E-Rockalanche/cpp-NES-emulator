#ifndef COMMON_HPP
#define COMMON_HPP

#include <iostream>
#include "debugging.hpp"

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

#define testFlag(mask, flag) ((bool)((mask) & (flag)))

#endif