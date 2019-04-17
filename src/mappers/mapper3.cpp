#include "mapper3.hpp"
#include "assert.hpp"

Mapper3::Mapper3(Byte* data) : Cartridge(data) {}

void Mapper3::writePRG(Word address, Byte value) {
	if (address & 0x8000) {
		// ANDing with rom emulates bus conflict
		setCHRBank(0, value & 0x03 & readPRG(address), 8 * KB);
	}
}

void Mapper3::writeCHR(Word address, Byte value) {
	assert(address < chr_size, "chr address out of bounds");
	chr[address] = value;
}