#include "apu.hpp"
#include "common.hpp"

const Byte APU::length_table[] = {
	0x0a, 0xfe, 0x14, 0x02, 0x28, 0x04, 0x50, 0x06,
	0xa0, 0x08, 0x3c, 0x0a, 0x0e, 0x0c, 0x1a, 0x0e, 
	0x0c, 0x10, 0x18, 0x12, 0x30, 0x14, 0x60, 0x16,
	0xc0, 0x18, 0x48, 0x1a, 0x10, 0x1c, 0x20, 0x1e
};

APU::APU() {
	for(int n = 0; n < NUM_REGISTERS; n++) {
		registers[n] = 0;
	}
}

APU::~APU() {}

Byte APU::readByte(unsigned int index) {
	assert(index < NUM_REGISTERS, "APU register index " << index << " out of bounds " << NUM_REGISTERS);
	assert(index != 0x14, "cannot read from APU at $4014");

	// dout("APU read " << toHex(registers[index]) << " from register " << index);

	return registers[index];
}

void APU::writeByte(unsigned int index, Byte value) {
	assert(index < NUM_REGISTERS, "APU register index " << index << " out of bounds " << NUM_REGISTERS);
	assert(index != 0x14, "cannot write to APU at $4014");

	// dout("APU write " << toHex(value) << " to register " << index);
	
	registers[index] = value;
}