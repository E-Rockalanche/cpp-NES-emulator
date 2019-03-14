#ifndef APU_HPP
#define APU_HPP

#include "common.hpp"

namespace APU {
	Byte readByte(unsigned int index);
	void writeByte(unsigned int index, Byte value);
}

#endif