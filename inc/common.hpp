#ifndef COMMON_HPP
#define COMMON_HPP

#include <iostream>
#include <iomanip>
#include <bitset>
#include "debugging.hpp"
#include "assert.hpp"

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

#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))

typedef unsigned char Byte;
typedef unsigned short Word;
typedef void(*Callback)(void);

#define BL(bit) (1 << (bit))

#define KB 0x400

template <class T, T V>
struct Constant {
	operator T() const { return V; }
};

#define testFlag(mask, flag) ((bool)((mask) & (flag)))

#endif