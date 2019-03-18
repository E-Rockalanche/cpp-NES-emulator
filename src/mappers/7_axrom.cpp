#include "7_axrom.hpp"

AxROM::AxROM(Byte* data) : Cartridge(data) {
	dout("AxROM()");
}

void AxROM::writePRG(Word address, Byte value) {
	value &= readPRG(value); // bus conflict

	int prg_bank = value & 0x07;
	int nt_page = (value >> 4) & 0x01;

	setPRGBank(0, prg_bank, 32 * KB);

	for(int i = 0; i < 4; i++) {
		PPU::mapNametable(i, nt_page);
	}
}

void AxROM::writeCHR(Word address, Byte value) {
	assert(address < chr_size, "chr address out of bounds");
	chr[address] = value;
}