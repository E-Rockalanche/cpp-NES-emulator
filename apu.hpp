#ifndef APU_HPP
#define APU_HPP

#include "common.hpp"

class APU {
public:
	APU();
	~APU();
	Byte readByte(unsigned int index);
	void writeByte(unsigned int index, Byte value);

	static const unsigned int NUM_REGISTERS = 0x18;

private:
	enum Register {
		PULSE_1 = 0x00,
		PULSE_2 = 0x04,
		TRIANLGE = 0x08,
		NOISE = 0x0c,
		DMC = 0x10,
		STATUS = 0x15,
		FRAME_COUNTER = 0x17
	};

	Byte registers[NUM_REGISTERS];
	// registers 0x14 & 0x16 are unused
};

#endif