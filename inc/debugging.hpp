#ifndef DEBUGGING_HPP
#define DEBUGGING_HPP

#include <iostream>
#include <iomanip>

#define GET_MACRO(_1, _2, _3, MACRO_NAME, ...) MACRO_NAME

#define toHex3(number, bytes, prefix) (prefix) << std::setfill('0') << std::setw((bytes) * 2) \
	<< std::hex << (static_cast<int>(number)) << std::dec << std::setw(0)
#define toHex2(number, bytes) toHex3(number, bytes, '$')
#define toHex1(number) toHex2(number, sizeof(number))
	
#define toHex(...) GET_MACRO(__VA_ARGS__, toHex3, toHex2, toHex1)(__VA_ARGS__)

#define DEBUG true

#if (DEBUG)
	#define dout(message) std::cout << "DEBUG: " << __FILE__ << " (" << __LINE__ << "): " << message << '\n'
#else
	#define dout(message)
#endif

#endif