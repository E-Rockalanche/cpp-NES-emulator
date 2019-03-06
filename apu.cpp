#include "apu.hpp"
#include "common.hpp"

APU::APU() {}

APU::~APU() {}

Byte APU::readByte(unsigned int index) {
	assert(index < NUM_REGISTERS, "APU register index " << index << " out of bounds " << NUM_REGISTERS);
	assert(index != 0x14, "cannot read from APU at $4014");

	// dout("reading from APU register " << toHex(index, 2));
	return registers[index];
}

void APU::writeByte(unsigned int index, Byte value) {
	assert(index < NUM_REGISTERS, "APU register index " << index << " out of bounds " << NUM_REGISTERS);
	assert(index != 0x14, "cannot write to APU at $4014");

	// dout("writing to APU register " << toHex(index, 2));

	if (index == 0x15) {
		value &= 0x1f;
	}
	
	registers[index] = value;
}