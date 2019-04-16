#include "1_mmc1.hpp"
#include "ppu.hpp"

MMC1::MMC1(Byte* data) : Cartridge(data) {
	dout("MMC1()");

	shift_register = SHIFT_REG_INIT;

	// set initial banks
	setPRGBank(0, 0, 16 * KB);
	setPRGBank(1, -1, 16 * KB);
}

void MMC1::writePRG(Word address, Byte value) {
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

int MMC1::mirrorMode() {
	return registers[CONTROL] & 0x03;
}

int MMC1::prgBankMode() {
	return (registers[CONTROL] >> 2) & 0x03;
}

bool MMC1::chrBankMode() {
	return registers[CONTROL] & 0x10;
}

bool MMC1::ramEnable() {
	return !(registers[PRG_BANK] & 0x10);
}

void MMC1::applyBankSwitch() {
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
			// 0xc000 fixed to last bank
			setPRGBank(1, -1, 16 * KB);
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
		case 2:  PPU::mapNametable(VERTICAL); break;
		case 3:  PPU::mapNametable(HORIZONTAL); break;
		default: dout("mirror mode = " << (int)mirrorMode()); break;
	}
}

void MMC1::saveState(std::ostream& out) {
	Cartridge::saveState(out);
	
	out.write((char*)&shift_register, sizeof(shift_register));
	out.write((char*)registers, sizeof(registers));
}

void MMC1::loadState(std::istream& in) {
	Cartridge::loadState(in);

	in.read((char*)&shift_register, sizeof(shift_register));
	in.read((char*)registers, sizeof(registers));
}