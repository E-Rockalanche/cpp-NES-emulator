#include "mapper2.hpp"

Mapper2::Mapper2(Byte* data) : Cartridge(data) {
	reset();
}

void Mapper2::reset() {
	Cartridge::reset();
	
	setPRGBank(0, 0, 16 * KB); // switchable
	setPRGBank(1, -1, 16 * KB); // fixed

	setCHRBank(0, 0, 8 * KB);
}

void Mapper2::writePRG(Word address, Byte value) {
	if (address & 0x8000) {
		setPRGBank(0, value & 0x0f, 16 * KB);
	}
}

void Mapper2::writeCHR(Word address, Byte value) {
	assert(address < chr_size, "chr address out of bounds");
	chr[address] = value;
}