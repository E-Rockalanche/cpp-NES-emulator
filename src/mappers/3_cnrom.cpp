#include "3_cnrom.hpp"
#include "assert.hpp"

CNROM::CNROM(Byte* data) : Cartridge(data) {
	dout("CNROM()");
}

CNROM::~CNROM() {}

void CNROM::writePRG(Word address, Byte value) {
	if (address & 0x8000) {
		// ANDing with rom emulates bus conflict
		setCHRBank(0, value & 0x03 & readPRG(address), 8 * KB);
	}
}

void CNROM::writeCHR(Word address, Byte value) {
	assert(address < chr_size, "chr address out of bounds");
	chr[address] = value;
}