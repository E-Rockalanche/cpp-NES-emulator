#ifndef MAPPER5_HPP
#define MAPPER5_HPP

#include "cartridge.hpp"

// MMC5
class Mapper5 : public Cartridge {
public:
	Mapper5(Byte* data);
	~Mapper5() {}

	void writePRG(Word address, Byte value);
	Byte readPRG(Word address);
	void writeCHR(Word address, Byte value);
	void signalScanline();

protected:
	enum Register {
		PRG_MODE = 0x5100,
		CHR_MODE,
		RAM_PROTECT_1,
		RAM_PROTECT_2,
		EXT_RAM_MODE,
		NT_MAPPING,
		FILL_TILE,
		FILL_COLOUR,

		PRG_BANKSWITCH_START = 0x5113,
		PRG_BANKSWITCH_END = 0x5117,

		CHR_BANKSWITCH_START = 0x5120,
		CHR_BANKSWITCH_END = 0x5130,

		VERTICAL_SPLIT_MODE = 0x5200,
		VERTICAL_SPLIT_SCROLL,
		VERTICAL_SPLIT_BANK,

		IRQ_SCANLINE,
		IRQ_STATUS,

		MULTIPLICAND,
		MULTIPLIER,

		// MMC5A only:
		CLSL_DATA_DIRECTION,
		CLSL_STATUS,
		TIMER_IRQ,

		EXP_RAM_START = 0x5c00,
		EXP_RAM_END = 0x5fff,
		EXP_RAM_SIZE = 0x0400
	};

	enum VSplitSide {
		LEFT,
		RIGHT
	};

	Byte registers[8];

	int vsplit_tile;
	VSplitSide vsplit_side;
	bool vsplit_enabled;
	Byte vsplit_scroll;
	Byte vsplit_bank;

	Byte irq_scanline;
	bool irq_enabled;

	Byte multiplicand;
	Byte multiplier;
	Word result;
};

#endif

Mapper5::Mapper5(Byte* date) : Cartridge(date) {
	registers[PRG_MODE] = 3;
	registers[CHR_MODE] = 3;
}

void writePRG(Word address, Byte value) {
	switch(address) {
		case PRG_MODE ... FILL_COLOUR:
			registers[address - PRG_MODE] = value;
			break;

		case PRG_BANKSWITCH_START ... PRG_BANKSWITCH_END:
			prgSwitch(address, value);
			break;

		case CHR_BANKSWITCH_START ... CHR_BANKSWITCH_END:
			chrSwitch(address, value);
			break;

		case VERTICAL_SPLIT_MODE:
			vsplit_tile = value & 0x1f;
			vsplit_side = testFlag(value, 0x40) ? RIGHT : LEFT;
			vsplit_enabled = testFlag(value, 0x80);
			break;

		case VERTICAL_SPLIT_SCROLL:
			vsplit_scroll = value;
			break;

		case VERTICAL_SPLIT_BANK:
			vsplit_bank = value;
			break;

		case IRQ_SCANLINE:
			irq_scanline = value;
			break;

		case IRQ_STATUS:
			irq_enabled = testFlag(value, 0x80);
			break;

		case MULTIPLICAND:
			multilicand = value;
			result = multiplicand * multiplier;
			break;

		case MULTIPLIER:
			multiplier = value;
			result = multiplicand * multiplier;
			break;

		// MMC5A only:
		// TODO
	}
}

Byte readPRG(Word address) {
	Byte value = 0;
	if (address >= 0x6000) {
		value = Cartridge::readPRG(address);
	} else {
		switch(address) {
			IRQ_STATUS:
				value = (irq_pending*0x80) | (in_frame*0x40);
				break;

			case MULTIPLICAND:
				value = result & 0xff;
				break;

			case MULTIPLIER:
				value = result >> 8;
				break;

			// MMC5A only:
			// TODO
		}
	}
	return value;
}

void prgSwitch(Word address, Byte value) {
	bool rom_toggle = testFlag(value, 0x80);
	value &= 0x7f;
	switch(address) {
		case 0x5113: mapRAM(0, value, 0x2000); break;

		case 0x5114:
			if (prgMode() == 3) map
		case 0x5115:
		case 0x5116:

		case 0x5117: 
			switch(prgMode()) {
				case 3: mapROM(3, value, 0x2000); break;
				case 2: mapROM(3, value, 0x2000); break;
				case 1: mapROM(1, value, 0x4000); break;
				case 0: mapROM(0, value, 0x8000); break;
			}
			break;
	}
}