#include <bitset>
#include "ppu.hpp"
#include "debugging.hpp"
#include "common.hpp"

using namespace std;

#define getFlag(flags, flag) ((flags) & (flag))
#define renderingIsEnabled() (mask & (SHOW_SPRITES | SHOW_BACKGROUND))

PPU::PPU() {
	dout("PPU()");
}

PPU::~PPU() {
	dout("~PPU()");
}

void PPU::setCPU(CPU* cpu) {
	assert(cpu != NULL, "PPU::setCPU() cpu is null");
	this->cpu = cpu;
}

void PPU::setROM(Byte* data, unsigned int size) {
	if (size > 0) {
		assert(data != NULL, "PPU::setROM() data is null");
		chr_rom = data;
		chr_rom_size = size;
	} else {
		dout("PPU must use CHR RAM");
	}
}

void PPU::reset() {
	dout("PPU::reset()");
	cycle = MAX_CYCLE;
	skip_cycle = false;
	scanline = MAX_SCANLINE;
	wait_cycles = BOOTUP_CYCLES;
	dot = 0;
	assert(cpu != NULL, "PPU::reset() cpu is null");
}

bool PPU::readyToDraw() {
	if (can_draw) {
		can_draw = false;
		return true;
	}
	return false;
}

const Pixel* PPU::getSurface() {
	return surface;
}

template <class Boolean>
void PPU::setStatusFlag(int flag, Boolean value) {
	if (value) {
		status |= flag;
	} else {
		status &= ~flag;
	}
}

void PPU::setVBlank() {
	setStatusFlag(VERTICAL_BLANK, Constant<bool, true>());
	in_vblank = true;
	if (getFlag(control, NMI_ENABLE)) {
		cpu->setNMI();
	}
}

void PPU::clockTick() {
	if (wait_cycles == 0) {
		cycle = (cycle + 1) % (MAX_CYCLE + 1);

		if (cycle == 0) {
			// increment scanline
			scanline = (scanline + 1) % (MAX_SCANLINE + 1);
			dot = 0;
			if (scanline == PRERENDER_SCANLINE) {
				setStatusFlag(VERTICAL_BLANK, Constant<bool, false>());
				in_vblank = false;
			}
		} else if (cycle == 1 && scanline == VBLANK_SCANLINE) {
			setVBlank();
			can_draw = true;
		}

		if (scanline < POSTRENDER_SCANLINE) {
			if (cycle == 0) {
				skip_cycle = !skip_cycle;
				if (skip_cycle) {
					cycle++;
				}
			}

			if (cycle == 0) {
				// idle cycle
			} else {
				if (cycle <= 256) {
					renderPixel();
				} else if (cycle <= 320) {
					// fetch sprite tile data
				} else if (cycle <= 336) {
					// fetch first two tiles for next scanline
				} else {
					// same bytes fetched as before and start of next scanline
				}
				incrementDot();
			}
		}
	} else {
		wait_cycles--;
		if (wait_cycles == 0) {
			dout("PPU wait cycles over");
		}
	}
}

void PPU::writeToControl(Byte value) {
	control = value;
	if (getFlag(control, NMI_ENABLE) && getFlag(status, VERTICAL_BLANK)) {
		cpu->setNMI();
	}
	temp_vram_address = (temp_vram_address & 0x73ff)
		| ((value & 0x3) << 10); // set name tables bits
}

void PPU::writeToScroll(Byte value) {
	if (write_toggle) {
		// second write
		temp_vram_address = (temp_vram_address & 0x7c1f)
			| ((value & 0x3) << 12)
			| ((value & 0xf8) << 2); // set coarse and fine y
	} else {
		// first write
		temp_vram_address = (temp_vram_address & 0x7fe0)
			| (value >> 3); // set coarse x
		fine_x_scroll = value & 0x7;
	}
	write_toggle = !write_toggle;
}

void PPU::writeToAddress(Byte value) {
	if (write_toggle) {
		// second write
		temp_vram_address = (temp_vram_address & 0x7f00)
			| (value); // set address low
		vram_address = temp_vram_address;
	} else {
		// first write
		temp_vram_address = (temp_vram_address & 0x3fff)
			| ((value & 0x3f) << 8); // set address high
	}
	write_toggle = !write_toggle;
}

void PPU::writeByte(Word address, Byte value) {
	switch(address) {
		case PPU_CONTROL:
			writeToControl(value);
			break;

		case PPU_MASK:
			mask = value;
			break;

		case OAM_ADDRESS:
			oam_address = (oam_address << 8) | value;
			break;

		case OAM_DATA:
			assert(oam_address < OAM_SIZE, "WRITE oam address " << toHex(oam_address, 2) << " out of bounds " << toHex(OAM_SIZE, 2));
			oam[oam_address++] = value;
			break;

		case PPU_SCROLL:
			writeToScroll(value);
			break;

		case PPU_ADDRESS:
			writeToAddress(value);
			break;

		case PPU_DATA:
			vram[vram_address] = value;
			incrementVRAMAddress();
			break;

		case OAM_DMA:
			oam_data_high = value;
			break;

		default:
			dout("PPU trying to write to " << toHex(address, 2));
	}
}

Byte PPU::readByte(Word address) {
	Byte value = 0;
	switch(address) {
		case PPU_STATUS:
			value = status;
			setStatusFlag(VERTICAL_BLANK, Constant<bool, false>());
			write_toggle = false;
			if (status & 0x80) dout("PPU reading status vblank is set");
			break;

		case OAM_DATA:
			assert(oam_address < OAM_SIZE, "READ oam address " << toHex(oam_address, 2) << " out of bounds " << toHex(OAM_SIZE, 2));
			value = oam[oam_address];
			break;

		case PPU_DATA:
			assert(vram_address < VRAM_SIZE, "READ vram address " << toHex(vram_address, 2) << " out of bounds " << toHex(VRAM_SIZE, 2));
			value = vram[vram_address];
			incrementVRAMAddress();
			break;

		default:
			dout("PPU trying to read from " << toHex(address, 2));
	}
	return value;
}

void PPU::incrementYComponent() {
	if ((vram_address & 0x7000) != 0x7000) { // if fine Y < 7
		vram_address += 0x1000; // increment fine Y
	} else {
		vram_address &= ~0x7000; // fine Y = 0
		int y = (vram_address & 0x03e0) >> 5; // let y = coarse Y
		if (y == 29) {
			y = 0; // coarse Y = 0
			vram_address ^= 0x0800; // switch vertical nametable
		} else if (y == 31) {
			y = 0; // coarse Y = 0, nametable not switched
		} else {
			y += 1; // increment coarse Y
		}
		vram_address = (vram_address & ~0x03e0) | (y << 5); // put coarse Y back into v
	}
}

void PPU::incrementXComponent() {
	if ((vram_address & 0x001f) == 31) { // if coarse X == 31
		vram_address &= ~0x001f; // coarse X = 0
		vram_address ^= 0x0400; // switch horizontal nametable
	} else {
		vram_address += 1;    
	}
}

Word PPU::getTileAddress() {
	return 0x200 | (vram_address & 0x0fff);
}

Word PPU::getAttributeAddress() {
	return 0x23c0 | (vram_address & 0x0c00) | ((vram_address >> 4) & 0x38) | ((vram_address >> 2) & 0x07);
}

void PPU::incrementVRAMAddress() {
	if (renderingIsEnabled() && in_vblank) {
		// this will happen if PPU_DATA read/writes occur during rendering
		incrementXComponent();
		incrementYComponent();
	} else {
		vram_address += getFlag(control, INCREMENT_MODE) ? 32 : 0;
	}
}

void PPU::incrementDot() {
	dot++;
	if (dot == 256) {
		if (renderingIsEnabled()) {
			incrementYComponent();
		}
	} else if (dot == 257) {
		const int X_COMPONENT = 0x031f;
		vram_address = (vram_address & X_COMPONENT)
			| (temp_vram_address & X_COMPONENT); //
	} else if ((scanline == PRERENDER_SCANLINE)
			&& (dot >= 380)
			&& (dot < 304)
			&& getFlag(mask, SHOW_BACKGROUND | SHOW_SPRITES)) {
		const int Y_COMPONENT = 0x7be0;
		vram_address = (vram_address & Y_COMPONENT)
			| (temp_vram_address & Y_COMPONENT);
	} else if ((dot >= 328 || dot <= 256)) {
		if (((dot & 0xf) == 8) && renderingIsEnabled()) {
			incrementXComponent();
		}
	}
}

void PPU::renderPixel() {
	Pixel pixel;

	assert(scanline < 240, "scanline = " << scanline);
	assert(dot < 256, "dot = " << dot);

	surface[dot + scanline * SCREEN_WIDTH] = pixel;
}

void PPU::writeToOAM(Byte index, Byte value) {
	oam[index] = value;
}