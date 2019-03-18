#include "5_mmc5.hpp"
#include "ppu.hpp"

MMC5::MMC5(Byte* data) : Cartridge(data) {
	registers.prg_mode = PRG_MODE_8K;
	registers.chr_mode = CHR_MODE_8K;
	registers.ext_ram_mode = EXTRA_NAMETABLE;
	multiplicand = 0;
	multiplier = 0;

	irq_scanline = 0;
	in_frame = false;
	current_scanline = 240;

	for(int i = 0; i < 5; i++) {
		prg_registers[i] = 0xff;
	}
	for(int i = 0; i < 8; i++) {
		chr_registers_a[i] = i;
	}
	for(int i = 0; i < 4; i++) {
		chr_registers_b[i] = i;
	}
	upper_chr_bits = 0;
	last_chr_set = CHR_SET_A;
	fetch_mode = FETCH_NONE;

	applyPRG();
	applyChrA();

	for(int i = EXT_RAM_START; i <= EXT_RAM_END; i++) {
		ext_ram[i - EXT_RAM_START] = 0;
	}
}

MMC5::~MMC5() {}

void MMC5::reset() {
	irq_enabled = false;
	CPU::setIRQ(false);
}

void MMC5::writePRG(Word address, Byte value) {
	switch(address) {
		case AUDIO_START ... AUDIO_END:
			dout("write to audio " << toHex(address, 2) << ", value: " << toHex(value, 1));
			break;

		case PRG_MODE:
			value &= 0x03;
			if (registers.prg_mode != value) {
				registers.prg_mode = value;
				applyPRG();
			}
			break;

		case CHR_MODE:
			value &= 0x03;
			if (registers.chr_mode != value) {
				registers.chr_mode = value;
				if (spriteSize() == SPR_8x8 || !PPU::renderingEnabled() || !in_frame) {
					if (last_chr_set == CHR_SET_A) {
						applyChrA();
					} else {
						applyChrB();
					}
				}
			}
			break;

		case RAM_PROTECT_1 ... EXT_RAM_MODE:
			if (address == EXT_RAM_MODE) dout("EXT RAM mode: " << (int)(value & 0x03));

			registers.r[address - PRG_MODE] = value & 0x03;
			break;

		case NT_MAPPING:
			if (registers.nt_mapping != value) {
				registers.nt_mapping = value;
				for(int i = 0; i < 4; i++) {
					int mode = (registers.nt_mapping >> i*2) & 0x03;
					switch(mode) {
						case NT_VRAM_0: PPU::mapNametable(i, 0); break;
						case NT_VRAM_1: PPU::mapNametable(i, 1); break;
						case NT_EXT_RAM: PPU::mapNametable(i, ext_ram); break;
						case NT_FILL_MODE: break; // TODO
					}
				}
			}
			break;

		case FILL_TILE ... FILL_COLOUR:
			registers.r[address - PRG_MODE] = value;
			break;

		case PRG_BANKSWITCH_START ... PRG_BANKSWITCH_END:
			if (prg_registers[address - PRG_BANKSWITCH_START] != value) {
				prg_registers[address - PRG_BANKSWITCH_START] = value;
				applyPRG();
			}
			break;

		case CHR_BANKSWITCH_A_START ... CHR_BANKSWITCH_A_END: {
			value |= upper_chr_bits << 2;
			int index = address - CHR_BANKSWITCH_A_START;
			if ((chr_registers_a[index] != value) || last_chr_set != CHR_SET_A) {
				chr_registers_a[index] = value;
				last_chr_set = CHR_SET_A;
				if (spriteSize() == SPR_8x8 || !PPU::renderingEnabled() || !in_frame) {
					applyChrA();
				}
			}
		} break;

		case CHR_BANKSWITCH_B_START ... CHR_BANKSWITCH_B_END: {
			value |= upper_chr_bits << 2;
			int index = address - CHR_BANKSWITCH_B_START;
			if ((chr_registers_b[index] != value) || last_chr_set != CHR_SET_B) {
				chr_registers_b[index] = value;
				last_chr_set = CHR_SET_B;
				if (spriteSize() == SPR_8x8 || !PPU::renderingEnabled() || !in_frame) {
					applyChrB();
				}
			}
		} break;

		case UPPER_CHR_BITS: upper_chr_bits = value & 0x03; break;

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
			if (irq_pending) CPU::setIRQ();
			break;

		case MULTIPLICAND:
			multiplicand = value;
			result = multiplicand * multiplier;
			break;

		case MULTIPLIER:
			multiplier = value;
			result = multiplicand * multiplier;
			break;

		case EXT_RAM_START ... EXT_RAM_END:
			switch(registers.ext_ram_mode) {
				case EXTRA_NAMETABLE:
				case EXT_ATTRIBUTE_DATA:
					break;

				case RAM_RW:
					ext_ram[address - EXT_RAM_START] = value;
					break;

				case RAM_READ_ONLY:
					break;
			}
			break;

		case Cartridge::RAM_START ... Cartridge::RAM_END:
			if (registers.ram_protect_1 == 0x02 && registers.ram_protect_2 == 0x01) {
				Byte* bank = ram_map[0];
				bank[address - Cartridge::RAM_START] = value;
			}
			break;

		default:
			dout("MMC5::writePRG(" << toHex(address, 2) << ", " << toHex(value, 1) << ")");
			break;


		// MMC5A only:
		// TODO
	}
}

Byte MMC5::readPRG(Word address) {
	Byte value = 0;
	if (address >= 0x6000) {
		value = Cartridge::readPRG(address);

		// reading from NMI vector resets scanline detection
		if (address >= 0xfffa && address <= 0xfffb) {
			in_frame = false;
			current_scanline = 0;
			irq_pending = false;
		}
	} else {
		switch(address) {
			case AUDIO_START ... AUDIO_END:
				dout("read from audio " << toHex(address, 2));
				break;

			case IRQ_STATUS:
				value = (irq_pending*0x80) | (in_frame*0x40);
				irq_pending = false;
				break;

			case MULTIPLICAND:
				value = result & 0xff;
				break;

			case MULTIPLIER:
				value = result >> 8;
				break;

			case EXT_RAM_START ... EXT_RAM_END:
				if (testFlag(registers.ext_ram_mode, 0x02)) {
					value = ext_ram[address - EXT_RAM_START];
				} else {
					value = (address >> 8);
				}
				break;

			default:
				dout("MMC5::readPRG(" << toHex(address, 2) << ", " << toHex(value, 1) << ")");
				break;

			// MMC5A only:
			// TODO
		}
	}
	return value;
}

void MMC5::writeCHR(Word address, Byte value) {
	dout("writeCHR(" << address << ", " << toHex(value) << ")");

	chr[address % chr_size] = value;
}

Byte MMC5::readCHR(Word address) {
	Byte value = 0;
	switch(registers.ext_ram_mode) {
		case RAM_RW:
		case RAM_READ_ONLY:
		case EXTRA_NAMETABLE:
			value = Cartridge::readCHR(address);
			break;

		case EXT_ATTRIBUTE_DATA:
			if (fetch_mode == FETCH_BACKGROUND) {
				value = chr[((upper_chr_bits << 12) | (address & 0x0fff)) % chr_size];
			} else {
				value = Cartridge::readCHR(address);
			}
			break;
	}
	return value;
}

void MMC5::setPRGBankExt(int slot, int bank, int bank_size) {
	bool map_rom = bank & 0x80;
	if (map_rom) {
		bank = (bank & 0x7f) >> (bank_size / 0x4000);
		setPRGBank(slot, bank, bank_size);
	} else {
		bank &= 0x0f;
		setBank(prg_map, ram, slot, bank, bank_size);
	}
}

void MMC5::applyPRG() {
	// map ram
	setBank(ram_map, ram, 0, prg_registers[0] & 0x0f, 8*KB);

	// map prg rom
	switch(registers.prg_mode) {
		case PRG_MODE_32K:
			setPRGBank(0, prg_registers[4] >> 2, 32*KB); // cannot be RAM
			break;

		case PRG_MODE_16K:
			setPRGBankExt(0, prg_registers[2], 16*KB);
			setPRGBank(1, prg_registers[4] >> 1, 16*KB); // cannot be RAM
			break;

		case PRG_MODE_16K_8K:
			setPRGBankExt(0, prg_registers[2], 16*KB);
			setPRGBankExt(2, prg_registers[3], 8*KB);
			setPRGBank(3, prg_registers[4], 8*KB); //cannot be RAM
			break;

		case PRG_MODE_8K:
			for(int i = 0; i < 3; i++) {
				setPRGBankExt(i, prg_registers[i+1], 8*KB);
			}
			setPRGBank(3, prg_registers[4], 8*KB); // cannot be RAM
			break;

		default: assert(false, "Invalid prg mode"); break;
	}
}

void MMC5::applyChrB() {
	switch(registers.chr_mode) {
		case CHR_MODE_8K: setCHRBank(0, chr_registers_b[3], 8*KB); break;

		case CHR_MODE_4K ... CHR_MODE_1K: {
			int slots = 1;
			for(int i = 1; i < registers.chr_mode; i++) slots *= 2;
				
			for(int i = 0; i < slots; i++) {
				int bank = chr_registers_b[(i+1) * 4/slots - 1];
				setCHRBank(i, bank, 4 * KB / slots);
				setCHRBank(i + slots, bank, 4 * KB / slots);
			}
		}
			break;

		default: assert(false, "Invalid chr mode: " << (int)registers.chr_mode); break;
	}
}

void MMC5::applyChrA() {
	int slots = 1;
	for(int i = 0; i < registers.chr_mode; i++) slots *= 2;

	for(int i = 0; i < slots; i++) {
		int bank = (upper_chr_bits << 8) | chr_registers_a[(i+1) * 8/slots - 1];
		setCHRBank(i, bank, 8 * KB / slots);
	}
}

MMC5::SpriteSize MMC5::spriteSize() {
	return (testFlag(PPU::getControl(), 0x20) && testFlag(PPU::getMask(), 0x18))
		? SPR_8x16 : SPR_8x8;
}

void MMC5::signalHBlank() {
	fetch_mode = FETCH_SPRITE;
	if (PPU::renderingEnabled()) {
		if ((last_chr_set == CHR_SET_A) || spriteSize() == SPR_8x16) {
			applyChrA();
		} else {
			applyChrB();
		}
	}
}

void MMC5::signalHRender() {
	fetch_mode = FETCH_BACKGROUND;
	if (PPU::renderingEnabled()) {
		if ((last_chr_set == CHR_SET_B) || spriteSize() == SPR_8x16) {
			applyChrB();
		} else {
			applyChrA();
		}
	}
}

void MMC5::signalVBlank() {
	in_frame = false;
	irq_pending = false;
	current_scanline = 0;
	
	if (last_chr_set == CHR_SET_A || spriteSize() == SPR_8x8) {
		applyChrA();
	} else {
		applyChrB();
	}
}

void MMC5::signalScanlineMMC5() {
	if (!PPU::renderingEnabled() || (PPU::getScanline() >= 240)) {
		in_frame = false;
	} else if (!in_frame) {
		in_frame = true;
		current_scanline = 0;
		irq_pending = false;
	} else {
		if (++current_scanline == irq_scanline) {
			irq_pending = true;
			if (irq_enabled) {
				CPU::setIRQ();
			}
		} else {
			irq_pending = false;
		}
	}
}