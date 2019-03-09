#include "mapper1.hpp"

Mapper1::Mapper1(Byte* data) : Cartridge(data) {
	dout("Mapper1()");

	shift_register = SHIFT_REG_INIT;
}

void Mapper1::writePRG(Word address, Byte value) {
	if (address >= PRG_START) {
		if (value & 0x80) {
			shift_register = SHIFT_REG_INIT;
			registers[CONTROL] |= 0x0c;
			applyBankSwitch();
		} else {
			bool last_write = shift_register & 1;
			shift_register >>= 1;
			shift_register |= (value & 1) << 4;
			if (last_write) {
				registers[(address >> 13) & 0x03] = shift_register;
				shift_register = SHIFT_REG_INIT;
				applyBankSwitch();
			}
		}
	} else if (address >= RAM_START) {
		ram[address - RAM_START] = value;
	}
}

void Mapper1::writeCHR(Word address, Byte value) {
	chr[address] = value;
}

int Mapper1::mirrorMode() {
	return registers[CONTROL] & 0x03;
}

int Mapper1::prgBankMode() {
	return (registers[CONTROL] >> 2) & 0x03;
}

bool Mapper1::chrBankMode() {
	return registers[CONTROL] & 0x10;
}

bool Mapper1::ramEnable() {
	return !(registers[PRG_BANK] & 0x10);
}

void Mapper1::applyBankSwitch() {
	// switch prg rom
	switch(prgBankMode()) {
		case 0:
		case 1:
			setPRGBank(0, (registers[PRG_BANK] & 0xf) >> 1, 32 * KB);
			break;

		case 2:
			// 0x8000 fixed to bank 0x00
			setPRGBank(0, 0, 16 * KB);
			// 0xc000 swappable
			setPRGBank(1, registers[PRG_BANK] & 0x0f, 16 * KB);
			break;

		case 3:
			// 0x8000 swappable
			setPRGBank(0, registers[PRG_BANK] & 0x0f, 16 * KB);
			// 0xc000 fixed to bank 0x0f
			setPRGBank(1, 0x0f, 16 * KB);
			break;
	}

	// switch chr rom
	if (chrBankMode()) {
		setCHRBank(0, registers[CHR_BANK_0], 4 * KB);
		setCHRBank(1, registers[CHR_BANK_1], 4 * KB);
	} else {
		setCHRBank(0, registers[CHR_BANK_0] >> 1, 8 * KB);
	}

	switch(mirrorMode()) {
		case 2: nt_mirroring = VERTICAL; break;
		case 3: nt_mirroring = HORIZONTAL; break;
	}
}