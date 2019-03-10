#include <bitset>
#include "ppu.hpp"
#include "debugging.hpp"
#include "common.hpp"

using namespace std;

const int PPU::nes_palette[] = {
	0x7C7C7C, 0x0000FC, 0x0000BC, 0x4428BC, 0x940084, 0xA80020, 0xA81000, 0x881400,
	0x503000, 0x007800, 0x006800, 0x005800, 0x004058, 0x000000, 0x000000, 0x000000,
	0xBCBCBC, 0x0078F8, 0x0058F8, 0x6844FC, 0xD800CC, 0xE40058, 0xF83800, 0xE45C10,
	0xAC7C00, 0x00B800, 0x00A800, 0x00A844, 0x008888, 0x000000, 0x000000, 0x000000,
	0xF8F8F8, 0x3CBCFC, 0x6888FC, 0x9878F8, 0xF878F8, 0xF85898, 0xF87858, 0xFCA044,
	0xF8B800, 0xB8F818, 0x58D854, 0x58F898, 0x00E8D8, 0x787878, 0x000000, 0x000000,
	0xFCFCFC, 0xA4E4FC, 0xB8B8F8, 0xD8B8F8, 0xF8B8F8, 0xF8A4C0, 0xF0D0B0, 0xFCE0A8,
	0xF8D878, 0xD8F878, 0xB8F8B8, 0xB8F8D8, 0x00FCFC, 0xF8D8F8, 0x000000, 0x000000 };

const char* PPU::register_names[] = {
	"PPU CONTROL",
	"PPU MASK",
	"PPU STATUS",
	"OAM ADDRESS",
	"OAM DATA",
	"PPU SCROLL",
	"PPU ADDRESS",
	"PPU DATA"
};

PPU::PPU() {
}

PPU::~PPU() {
}

void PPU::setCPU(CPU* cpu) {
	assert(cpu != NULL, "PPU::setCPU() cpu is null");
	this->cpu = cpu;
}

void PPU::setCartridge(Cartridge* cartridge) {
	assert(cartridge != NULL, "PPU::setCartridge() cartridge is null");
	this->cartridge = cartridge;
	nt_mirror = cartridge->nameTableMirroring();
	dout("nametable mirroring: " << nt_mirror);
}

void PPU::power() {
	dout("PPU::power()");

	assert(cpu != NULL, "PPU::power() cpu is null");
	assert(cartridge != NULL, "PPU::power() cpu is null");

	cycle = 0;
	scanline = 0;
	odd_frame = false;

	control = 0;
	mask = 0;
	status = 0xc0;
	oam_address = 0;
	vram_address = 0;
	temp_vram_address = 0;
	fine_x_scroll = 0;

	in_vblank = false;
	write_toggle = false;
	open_bus = 0;

	can_draw = false;
	
	sprite_zero_next_scanline = false;
	sprite_zero_this_scanline = false;
	sprite_zero_hit = false;

	for(int n = 0; n < PRIMARY_OAM_SIZE; n++) {
		primary_oam[n] = 0xff;
	}

	for(int x = 0; x < SCREEN_WIDTH; x++) {
		for(int y = 0; y < SCREEN_HEIGHT; y++) {
			surface[x + y*SCREEN_WIDTH] = Pixel(0);
		}
	}
}

void PPU::reset() {
	dout("PPU::reset()");

	assert(cpu != NULL, "PPU::reset() cpu is null");
	assert(cartridge != NULL, "PPU::reset() cpu is null");

	cycle = 0;
	scanline = 0;
	odd_frame = false;

	control = 0;
	mask = 0;

	in_vblank = false;
	write_toggle = false;

	can_draw = false;
	
	sprite_zero_next_scanline = false;
	sprite_zero_this_scanline = false;
	sprite_zero_hit = false;

	for(int x = 0; x < SCREEN_WIDTH; x++) {
		for(int y = 0; y < SCREEN_HEIGHT; y++) {
			surface[x + y*SCREEN_WIDTH] = Pixel(0);
		}
	}
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

void PPU::writeByte(Word address, Byte value) {
	open_bus = value;
	switch(address) {
		case PPU_CONTROL:
			writeToControl(value);
			break;

		case PPU_MASK:
			mask = value;
			break;

		case OAM_ADDRESS:
			oam_address = value;
			break;

		case OAM_DATA:
			primary_oam[oam_address++] = value;
			break;

		case PPU_SCROLL:
			writeToScroll(value);
			break;

		case PPU_ADDRESS:
			writeToAddress(value);
			break;

		case PPU_DATA:
			write(vram_address, value);
			incrementVRAMAddress();
			break;

		default:
			dout("writing to " << register_names[address]);
	}
}

Byte PPU::readByte(Word address) {
	static Byte buffer;

	switch(address) {
		case PPU_STATUS:
			open_bus = (open_bus & 0x1f) | status;
			setStatusFlag(VERTICAL_BLANK, false);
			write_toggle = false;
			break;

		case OAM_DATA:
			open_bus = primary_oam[oam_address];
			break;

		case PPU_DATA:
			if (vram_address >= PALETTE_START) {
				// no buffering for palette ram
				buffer = read(vram_address);
				open_bus = buffer;
			} else {
				open_bus = buffer;
				buffer = read(vram_address);
			}
			incrementVRAMAddress();
			break;

		default:
			// dout("reading from " << register_names[address]);
		break;
	}

	return open_bus;
}

void PPU::setStatusFlag(int flag, bool value) {
	if (value) {
		status |= flag;
	} else {
		status &= ~flag;
	}
}

bool PPU::rendering() {
	return mask & (SHOW_BACKGROUND | SHOW_SPRITES);
}

int PPU::spriteHeight() {
	return testFlag(control, SPRITE_HEIGHT) ? 16 : 8;
}

void PPU::setVBlank() {
	setStatusFlag(VERTICAL_BLANK, true);
	in_vblank = true;
	if (testFlag(control, NMI_ENABLE)) {
		cpu->setNMI();
	}
}

void PPU::clearOAM() {
	for(int i = 0; i < 8 * OBJECT_SIZE; i++) {
		secondary_oam[i] = 0xff;
	}
}


void PPU::clockTick() {
	switch(scanline) {
		case 0 ... 239: scanlineCycle<VISIBLE>(); break;
		case 240: scanlineCycle<POSTRENDER>(); break;
		case 241: scanlineCycle<VBLANK>(); break;
		case 261: scanlineCycle<PRERENDER>(); break;
		default: break;
	}

	if (++cycle > 340) {
		cycle -= 341;
		if (++scanline == 262) {
			scanline = 0;
			odd_frame = !odd_frame;
		}
	}
}

template <PPU::Scanline s>
void PPU::scanlineCycle() {
	static Word address = 0;

	if (s == VBLANK && cycle == 1) {
		setVBlank();
	} else if (s == POSTRENDER && cycle == 0) {
		can_draw = true;
	} else if (s == VISIBLE || s == PRERENDER) {
		// sprites
		switch(cycle) {
			case 1:
				clearOAM();
				if (s == PRERENDER) {
					setStatusFlag(SPRITE_OVERFLOW, false);
					setStatusFlag(SPRITE_0_HIT, false);
					sprite_zero_hit = false;
				}
				break;

			case 257:
				loadSpritesOnScanline();
				break;

			case 321:
				loadSpriteRegisters();
				break;
		}

		// background
		switch (cycle) {
			case 2 ... 255:
			case 322 ... 337:
				renderPixel();
				switch (cycle % 8) {
					// nametable:
					case 1:
						address = nametableAddress();
						loadShiftRegisters();
						break;
					case 2: nametable_latch = read(address); break;

					// attribute:
					case 3: address = attributeAddress(); break;
					case 4:
						attribute_latch = read(address);
						if (testFlag(vram_address, 0x40)) attribute_latch >>= 4;
						if (testFlag(vram_address, 0x02)) attribute_latch >>= 2;
						break;

					// background
					case 5: address = backgroundAddress(); break;
					case 6: bg_latch_low = read(address); break;
					case 7: address += 8; break;
					case 0:
						bg_latch_high = read(address);
						incrementXComponent();
						break;
				}
				break;

			case 256:
				renderPixel();
				bg_latch_high = read(address);
				incrementYComponent();
				break;

			case 257:
				renderPixel();
				loadShiftRegisters();
				updateVRAMX();
				break;

			case 280 ... 304:
				if (s == PRERENDER) {
					updateVRAMY();
				}
				break;

			// no shift loading
			case 1:
				address = nametableAddress();
				if (s == PRERENDER) {
					setStatusFlag(VERTICAL_BLANK, false);
				}
				break;
			case 321:
			case 339: address = nametableAddress(); break;

			// nametable fetch instead of attribute
			case 338: nametable_latch = read(address); break;
			case 340:
				nametable_latch = read(address);
				if (s == PRERENDER && rendering() && odd_frame) {
					cycle++;
				}
		}

		// signal scanline to mapper
		// if (cycle == 260 && rendering()) cartridge->signalScanline();
	}
}

void PPU::writeToControl(Byte value) {
	control = value;
	if (testFlag(control, NMI_ENABLE) && testFlag(status, VERTICAL_BLANK)) {
		cpu->setNMI();
	}
	temp_vram_address = (temp_vram_address & 0x73ff)
		| ((value & 0x3) << 10); // set name tables bits
}

void PPU::writeToScroll(Byte value) {
	if (!write_toggle) {
		// first write
		fine_x_scroll = value & 0x7; // fine x
		// coarse x
		temp_vram_address = (temp_vram_address & 0x7fe0) | (value >> 3);
	} else {
		// second write
		int fine_y = value & 0x7;
		int coarse_y = value >> 3;
		temp_vram_address = (temp_vram_address & 0x0c1f)
			| (fine_y << 12) | (coarse_y << 5);
	}
	write_toggle = !write_toggle;
}

void PPU::writeToAddress(Byte value) {
	if (!write_toggle) {
		// first write
		temp_vram_address = (temp_vram_address & 0x00ff)
			| ((value & 0x3f) << 8); // set address high
	} else {
		// second write
		temp_vram_address = (temp_vram_address & 0x7f00)
			| (value & 0xff); // set address low
		vram_address = temp_vram_address;
	}
	write_toggle = !write_toggle;
}

void PPU::incrementXComponent() {
	if (!rendering()) return;

	if ((vram_address & 0x001f) == 31) { // if coarse X == 31
		vram_address ^= 0x041f;
		/*
		vram_address &= ~0x001f; // coarse X = 0
		vram_address ^= 0x0400; // switch horizontal nametable
		*/
	} else {
		vram_address++;
	}
}

void PPU::incrementYComponent() {
	if (!rendering()) return;

	if ((vram_address & 0x7000) != 0x7000) { // if fine Y < 7
		vram_address += 0x1000; // increment fine Y
	} else {
		vram_address &= ~0x7000; // fine Y = 0
		int y = (vram_address >> 5) & 0x1f; // let y = coarse Y
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

void PPU::updateVRAMX() {
	if (!rendering()) return;
	static const int X_COMPONENT = 0x041f;
	vram_address = (vram_address & ~X_COMPONENT) | (temp_vram_address & X_COMPONENT);
}

void PPU::updateVRAMY() {
	if (!rendering()) return;
	static const int Y_COMPONENT = 0x7be0;
	vram_address = (vram_address & ~Y_COMPONENT) | (temp_vram_address & Y_COMPONENT);
}

Word PPU::nametableAddress() {
	return 0x2000 | (vram_address & 0x0fff);
}

Word PPU::attributeAddress() {
	return 0x23c0
		| (vram_address & 0x0c00)
		| ((vram_address >> 4) & 0x38)
		| ((vram_address >> 2) & 0x07);
}

Word PPU::backgroundAddress() {
	return (testFlag(control, BACKGROUND_TILE_SELECT) * 0x1000)
		+ (nametable_latch * 16)
		+ ((vram_address >> 12) & 0x07);
}

void PPU::incrementVRAMAddress() {
	
	if (rendering() && !in_vblank) {
		// this will happen if PPU_DATA read/writes occur during rendering
		dout("increment VRAM address outside vblank");
		incrementXComponent();
		incrementYComponent();
	} else {
		vram_address += testFlag(control, INCREMENT_MODE) ? 32 : 1;
	}
	
	// vram_address += testFlag(control, INCREMENT_MODE) ? 32 : 1;
}

void PPU::writeToOAM(Byte value) {
	assert(oam_address < PRIMARY_OAM_SIZE, "primary oam address out of bounds");
	primary_oam[oam_address++] = value;
}

Word PPU::nametableMirror(Word address) {
	Word new_address;
	switch(cartridge->nameTableMirroring()) {
		case Cartridge::HORIZONTAL:
			new_address = ((address / 2) & 0x400) + (address % 0x400);
			break;

		case Cartridge::VERTICAL:
			new_address = address % 0x800;
			break;

		default:
			assert(false, "invalid nametable mirroring: " << (int)nt_mirror);
			break;
	}
	return new_address;
}

// PPU read from cartridge / ram
Byte PPU::read(Word address) {
	Byte value;
	switch (address) {
		case CHR_START ... CHR_END:
			value = cartridge->readCHR(address);
			break;

		case NAMETABLE_START ... NAMETABLE_END: {
			Word nt_index = nametableMirror(address);
			assert(nt_index < NAMETABLE_SIZE, "read nametable index out of bounds");
			value = nametable[nt_index];
			break;
		}

		case PALETTE_START ... PALETTE_END:
			if ((address & 0x13) == 0x10) address &= ~0x10;
			value = palette[address % PALETTE_SIZE]
				& (testFlag(mask, GREYSCALE) ? 0x30 : 0xff);
			break;

		default:
			value = 0;
			break;
	}
	return value;
}

// PPU write to cartridge / ram
void PPU::write(Word address, Byte value) {
	switch(address) {
		case CHR_START ... CHR_END:
			cartridge->writeCHR(address, value);
			break;

		case NAMETABLE_START ... NAMETABLE_END: {
			Word nt_index = nametableMirror(address);
			assert(nt_index < NAMETABLE_SIZE, "write nametable index out of bounds");
			nametable[nt_index] = value;
			break;
		}

		case PALETTE_START ... PALETTE_END:
			if ((address & 0x13) == 0x10) address &= ~0x10;
			palette[address % PALETTE_SIZE] = value;
			break;
	}
}

void PPU::loadShiftRegisters() {
	bg_shift_low = (bg_shift_low & 0xff00) | bg_latch_low;
	bg_shift_high = (bg_shift_high & 0xff00) | bg_latch_high;

	attribute_latch_low = attribute_latch & 1;
	attribute_latch_high = attribute_latch & 2;
}

// load secondary oam for sprites in next scanline
void PPU::loadSpritesOnScanline() {
	sprite_zero_next_scanline = false;
	int j = 0;
	for(int i = 0; i < 64; i++) {
		Byte* object = primary_oam + i * OBJECT_SIZE;
		int line = ((scanline == 261) ? -1 : scanline)
			- object[Y_POS];

		if ((line >= 0) && (line < spriteHeight())) {
			// sprite on scanline
			if (i == 0) {
				sprite_zero_next_scanline = true;
			}

			if (j < 8) {
				Byte* secondary_object = secondary_oam + j * OBJECT_SIZE;

				// copy sprite data
				for(int a = 0; a < OBJECT_SIZE; a++) {
					secondary_object[a] = object[a];
				}
			}

			// increment secondary OAM index
			if (++j > 8) {
				if (rendering()) {
					setStatusFlag(SPRITE_OVERFLOW);
				}
				break;
			}
		}
	}
}

void PPU::loadSpriteRegisters() {
	Word address;
	sprite_zero_this_scanline = sprite_zero_next_scanline;

	for(int i = 0; i < 8; i++) {
		Byte* object = secondary_oam + (i * OBJECT_SIZE);
		Byte tile = object[TILE_INDEX];

		if (spriteHeight() == 16) {
			address = ((tile & 1) * 0x1000) + ((tile & ~1) * 16);
		} else {
			address = (testFlag(control, SPRITE_TILE_SELECT) * 0x1000)
				+ (tile * 16);
		}

		// line inside sprite
		unsigned int sprite_y = (scanline - object[Y_POS]) % spriteHeight();

		// flip vertically
		if (testFlag(object[ATTRIBUTES], FLIP_VER)) {
			sprite_y ^= (spriteHeight() - 1);
		}

		address += sprite_y + (sprite_y & 8); // select second tile if on 8x16

		// load registers
		sprite_shift_low[i] = read(address);
		sprite_shift_high[i] = read(address + 8);
		sprite_x_counter[i] = object[X_POS];
		sprite_attribute_latch[i] = object[ATTRIBUTES];
	}
}

#define bit(value, n) (((value) >> (n)) & 1)

void PPU::renderPixel() {
	if (rendering()) {
		Byte palette = 0;
		Byte obj_palette = 0;
		bool obj_priority = 0;
		int x = cycle - 2;

		if (scanline < 240 && x >= 0 && x < 256) {
			// background
			if (testFlag(mask, SHOW_BACKGROUND)
					&& (testFlag(mask, SHOW_BKG_LEFT_8) || (x >= 8))) {
				palette = (bit(bg_shift_high, 15 - fine_x_scroll) << 1)
					| bit(bg_shift_low, 15 - fine_x_scroll);
				if (palette) {
					palette |= ((bit(attribute_shift_high, 7 - fine_x_scroll) << 1)
						| bit(attribute_shift_low, 7 - fine_x_scroll)) << 2;
				}
			}

			//sprites
			if (testFlag(mask, SHOW_SPRITES)
					&& (testFlag(mask, SHOW_SPR_LEFT_8) || (x >= 8))) {
				for(int i = 7; i >= 0; i--) {
					Byte attributes = sprite_attribute_latch[i];

					/*
					if (attributes == 0xff && sprite_x_counter[i] == 0xff) {
						continue; // empty entry
					}
					*/

					unsigned int sprite_x = x - sprite_x_counter[i];
					if (sprite_x >= 8) continue; // not in range

					if (testFlag(attributes, FLIP_HOR)) {
						sprite_x ^= 0x07;
					}

					Byte spr_palette = (bit(sprite_shift_high[i], 7 - sprite_x) << 1)
						| bit(sprite_shift_low[i], 7 - sprite_x);

					if (spr_palette == 0) continue; // transparent

					if ((i == 0) && sprite_zero_this_scanline && palette
						&& (x != 255) && !sprite_zero_hit) {
						sprite_zero_hit = true;
						setStatusFlag(SPRITE_0_HIT);
					}

					spr_palette |= (attributes & SPR_PALETTE) << 2;
					obj_palette = spr_palette + 16;
					obj_priority = testFlag(attributes, PRIORITY);
				}
			}

			if (obj_palette && (palette == 0 || obj_priority == 0)) {
				palette = obj_palette;
			}

			int palette_index = read(PALETTE_START + (rendering() ? palette : 0));
			int y = (SCREEN_HEIGHT - 1 - scanline);
			surface[y * SCREEN_WIDTH + x] = Pixel(nes_palette[palette_index]);
		}
	}

	// background_shifts

	bg_shift_low <<= 1;
	bg_shift_high <<= 1;

	attribute_shift_low = (attribute_shift_low << 1) | attribute_latch_low;
	attribute_shift_high = (attribute_shift_high << 1) | attribute_latch_high;
}

void PPU::dump() {
	std::cout << "===== NAMETABLE =====\n";
	for(int n = 0; n < 2; n++) {
		for(int y = 0; y < 32; y++) {
			for(int x = 0; x < 32; x++) {
				Byte value = nametable[n*1024 + y*32 + x*8];
				std::cout << toHex(value, 1) << ' ';
			}
			std::cout << '\n';
		}
		std::cout << '\n';
	}
	std::cout << "====== PALETTE ======\n";
	for(int n = 0; n < PALETTE_SIZE; n++) {
		std::cout << "[" << n << "]: " << (int)palette[n] << '\n';
	}
	std::cout << "=====================\n";
}