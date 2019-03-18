#include <bitset>
#include <cstdlib>

#include "ppu.hpp"
#include "debugging.hpp"
#include "common.hpp"

namespace PPU {

#define doutCycle(message) dout(message << ", s: " << scanline << ", c: " << cycle);
int frame = 0;

enum Scanline {
	VISIBLE,
	POSTRENDER,
	PRERENDER,
	VBLANK_LINE
};
template <Scanline s>
void scanlineCycle();

void setStatusFlag(int flag, bool value = true);

int spriteHeight();

void clearOAM();

void incrementYComponent();
void incrementXComponent();
void incrementVRAMAddress();
void updateVRAMX();
void updateVRAMY();

Word nametableAddress();
Word attributeAddress();
Word backgroundAddress();

void writeToControl(Byte value);
void writeToScroll(Byte value);
void writeToAddress(Byte value);
void renderPixel();

void setVBlank();
void clearVBlank();

Byte read(Word address);
void write(Word address, Byte value);

void loadShiftRegisters();
void loadSpritesOnScanline();
void loadSpriteRegisters();

const int PRERENDER_SCANLINE = 261;
const int NUM_SCANLINES = 262;
/*
dummy scanline
purpose is to fill shift registers with the data for the first two tiles to be rendered
no pixels are rendered but the PPU still makes the same memory accesses it would for a regular scanline
*/
const int POSTRENDER_SCANLINE = 240;
/*
The PPU idles during this scanline.
Accessing PPU memory is safe here but VBLANK is set next scanline
*/
const int VBLANK_SCANLINE = 241;
/*
VBLANK flag set on second tick of this scanline (and the NMI occurs)
The PPU makes no memory accesses during the VBLANK scanlines, so the PPU memory can be freely accessed by the program
*/
const int NUM_CYCLES = 341;

const int CHR_START = 0;
const int CHR_END = 0x1fff;

const int NAMETABLE_START = 0x2000;
const int NAMETABLE_END = 0x3eff;
const int NAMETABLE_SIZE = 0x0800;
const int NAMETABLE_MIRROR_SIZE = 0x1000;

const int PALETTE_START = 0x3f00;
const int PALETTE_END = 0x3fff;
const int PALETTE_SIZE = 0x20;

enum Object {
	Y_POS,
	TILE_INDEX,
	ATTRIBUTES,
	X_POS,

	OBJECT_SIZE
};

enum ObjectAttribute {
	SPR_PALETTE = 0x03, // 4 to 7
	PRIORITY = BL(5), // 0: front, 1: back
	FLIP_HOR = BL(6),
	FLIP_VER = BL(7)
};

const int PRIMARY_OAM_SIZE = 64;
const int SECONDARY_OAM_SIZE = 8;

Byte nametable[NAMETABLE_SIZE];
Byte palette[PALETTE_SIZE];
Byte primary_oam[PRIMARY_OAM_SIZE * OBJECT_SIZE];
Byte secondary_oam[PRIMARY_OAM_SIZE * OBJECT_SIZE]; // uses secondary size if sprite flickering is on

// name table mapping set by cartridge
Byte* nt_map[4];

// lets main know when the screen can be drawn
bool can_draw;

// remove sprite flickering when there are more than 8 on a scanline
bool sprite_flickering = true;

Byte open_bus;
int open_bus_decay_timer;

bool write_toggle; // used to set x_scroll/y_scroll and vram_address
Byte control;
Byte mask;
Byte status;
Byte oam_address;
Word vram_address; // 15 bits: -yyyNNYYYYYXXXXX (fine y scroll, nametable select, coarse Y scroll, coarse X scroll)
Word temp_vram_address; // 15 bits
Byte fine_x_scroll; // 3 bits

bool suppress_vblank;

Byte bg_latch_low;
Byte bg_latch_high;
// VVV
Word bg_shift_low;
Word bg_shift_high;

Byte attribute_latch; // 2 bit latch
// VVV
bool attribute_latch_low;
bool attribute_latch_high;
// VVV
Byte attribute_shift_low;
Byte attribute_shift_high;

Byte nametable_latch;

// uses secondary oam size if sprite flickering
Byte sprite_shift_low[PRIMARY_OAM_SIZE];
Byte sprite_shift_high[PRIMARY_OAM_SIZE];
Byte sprite_x_counter[PRIMARY_OAM_SIZE];
Byte sprite_attribute_latch[PRIMARY_OAM_SIZE];

bool sprite_zero_next_scanline;
bool sprite_zero_this_scanline;
bool sprite_zero_hit;

int cycle;
int scanline;
bool odd_frame;

const int nes_palette[] = {
	0x7C7C7C, 0x0000FC, 0x0000BC, 0x4428BC, 0x940084, 0xA80020, 0xA81000, 0x881400,
	0x503000, 0x007800, 0x006800, 0x005800, 0x004058, 0x000000, 0x000000, 0x000000,
	0xBCBCBC, 0x0078F8, 0x0058F8, 0x6844FC, 0xD800CC, 0xE40058, 0xF83800, 0xE45C10,
	0xAC7C00, 0x00B800, 0x00A800, 0x00A844, 0x008888, 0x000000, 0x000000, 0x000000,
	0xF8F8F8, 0x3CBCFC, 0x6888FC, 0x9878F8, 0xF878F8, 0xF85898, 0xF87858, 0xFCA044,
	0xF8B800, 0xB8F818, 0x58D854, 0x58F898, 0x00E8D8, 0x787878, 0x000000, 0x000000,
	0xFCFCFC, 0xA4E4FC, 0xB8B8F8, 0xD8B8F8, 0xF8B8F8, 0xF8A4C0, 0xF0D0B0, 0xFCE0A8,
	0xF8D878, 0xD8F878, 0xB8F8B8, 0xB8F8D8, 0x00FCFC, 0xF8D8F8, 0x000000, 0x000000 };

const char* register_names[] = {
	"PPU CONTROL",
	"PPU MASK",
	"PPU STATUS",
	"OAM ADDRESS",
	"OAM DATA",
	"PPU SCROLL",
	"PPU ADDRESS",
	"PPU DATA"
};

void power() {
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

	suppress_vblank = false;

	write_toggle = false;
	open_bus = 0;

	can_draw = false;
	
	sprite_zero_next_scanline = false;
	sprite_zero_this_scanline = false;

	for(int i = 0; i < 4; i++) {
		nt_map[i] = nametable + ((i % 2) * KB); // vertical mirroring
	}

	for(int n = 0; n < PRIMARY_OAM_SIZE * OBJECT_SIZE; n++) {
		primary_oam[n] = 0xff;
	}

	for(int x = 0; x < SCREEN_WIDTH; x++) {
		for(int y = 0; y < SCREEN_HEIGHT; y++) {
			screen[x + y*SCREEN_WIDTH] = Pixel(0);
		}
	}

	// clock can start in one of 4 different cpu synchronization alignments
	for(int i = rand() % 4; i < 3; i++) clockTick();
}

void reset() {
	cycle = 0;
	scanline = 0;
	odd_frame = false;

	control = 0;
	mask = 0;

	suppress_vblank = false;

	write_toggle = false;

	can_draw = false;
	
	sprite_zero_next_scanline = false;
	sprite_zero_this_scanline = false;

	for(int x = 0; x < SCREEN_WIDTH; x++) {
		for(int y = 0; y < SCREEN_HEIGHT; y++) {
			screen[x + y*SCREEN_WIDTH] = Pixel(0);
		}
	}

	// clock can start in one of 4 different cpu synchronization alignments
	for(int i = rand() % 4; i < 3; i++) clockTick();
}

bool readyToDraw() {
	if (can_draw) {
		can_draw = false;
		return true;
	}
	return false;
}

void writeByte(Word address, Byte value) {
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

Byte readByte(Word address) {
	static Byte buffer;

	switch(address) {
		case PPU_STATUS:
			open_bus = (open_bus & 0x1f) | status;
			clearVBlank();
			write_toggle = false;

			if (scanline == VBLANK_SCANLINE) {
				if (cycle == 0) {
					suppress_vblank = true;
				} else if (cycle == 1 || cycle == 2) {
					CPU::setNMI(false);
				}
			}
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

		default: break;
	}

	return open_bus;
}

Byte getControl() { return control; }
Byte getMask() { return mask; }
int getScanline() { return scanline; }

void mapNametable(int from_page, int to_page) {
	assert(from_page < 4, "invalid from_page");
	assert(to_page < 2, "invalid to_page");
	nt_map[from_page] = nametable + (to_page * KB);
}

void mapNametable(int page, Byte* location) {
	assert(page < 4, "Invalid nametable page index");
	assert(location != NULL, "nametable page location is NULL")
	nt_map[page] = location;
}

void mapNametable(Cartridge::NameTableMirroring nt_mirroring) {
	mapNametable(0, 0);
	mapNametable(3, 1);
	switch(nt_mirroring) {
		case Cartridge::HORIZONTAL:
			mapNametable(1, 0);
			mapNametable(2, 1);
			break;

		case Cartridge::VERTICAL:
			mapNametable(1, 1);
			mapNametable(2, 0);
			break;

		default: assert(false, "Invalid mirroring mode");
	}
}

void writeToControl(Byte value) {
	// manual NMI trigger during vblank
	if (!testFlag(control, NMI_ENABLE)
			&& testFlag(value, NMI_ENABLE)
			&& testFlag(status, VBLANK)
			&& (scanline != PRERENDER_SCANLINE)) {
		CPU::setNMI();

	// NMI suppression near vblank
	} else if (testFlag(control, NMI_ENABLE)
			&& !testFlag(value, NMI_ENABLE)
			&& (scanline == VBLANK_SCANLINE)
			&& (cycle <= 2)) {
		CPU::setNMI(false);
	}

	control = value;

	temp_vram_address = (temp_vram_address & 0x73ff)
		| ((value & 0x3) << 10); // set name tables bits
}

void writeToScroll(Byte value) {
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

void writeToAddress(Byte value) {
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

void setStatusFlag(int flag, bool value) {
	if (value) {
		status |= flag;
	} else {
		status &= ~flag;
	}
}

bool renderingEnabled() {
	return mask & (SHOW_BACKGROUND | SHOW_SPRITES);
}

int spriteHeight() {
	return testFlag(control, SPRITE_HEIGHT) ? 16 : 8;
}

void setVBlank() {
	if (!suppress_vblank) {
		setStatusFlag(VBLANK, true);
		if (testFlag(control, NMI_ENABLE)) {
			CPU::setNMI();
		}
	}
	suppress_vblank = false;
}

void clearVBlank() {
	setStatusFlag(VBLANK, false);
}

int secondaryOamSize() {
	return sprite_flickering ? SECONDARY_OAM_SIZE : PRIMARY_OAM_SIZE;
}

void clearOAM() {
	int size = secondaryOamSize() * OBJECT_SIZE;
	for(int i = 0; i < size; i++) {
		secondary_oam[i] = 0xff;
	}
}

void clockTick() {
	cycle = (cycle + 1) % 341;
	if (odd_frame
			&& (scanline == PRERENDER_SCANLINE)
			&& renderingEnabled()
			&& (cycle == 340)) {
		cycle = 0;
	}
	if (cycle == 0) {
		scanline = (scanline + 1) % 262;
		if (scanline == 0) {
			odd_frame = !odd_frame;
			frame++;
		}
	}

	switch(scanline) {
		case 0 ... 239: scanlineCycle<VISIBLE>(); break;
		case 240: scanlineCycle<POSTRENDER>(); break;
		case 241: scanlineCycle<VBLANK_LINE>(); break;
		case 261: scanlineCycle<PRERENDER>(); break;
		default: break;
	}
}

template <Scanline s>
void scanlineCycle() {
	static Word address = 0;

	if (s == VBLANK_LINE && cycle == 1) {
		setVBlank();
		cartridge->signalVBlank();
	} else if (s == POSTRENDER && cycle == 0) {
		can_draw = true;
	} else if (s == VISIBLE || s == PRERENDER) {
		// sprites
		switch(cycle) {
			case 1:
				// signal MMC5 to switch to background data
				clearOAM();
				if (s == PRERENDER) {
					setStatusFlag(SPRITE_OVERFLOW, false);
					setStatusFlag(SPRITE_0_HIT, false);
					clearVBlank();
				}
				break;

			case 257:
				// signal MMC5 to switch to sprite data
				cartridge->signalHBlank();
				loadSpritesOnScanline();
				break;

			case 321:
				loadSpriteRegisters();
				cartridge->signalHRender();
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
						if (testFlag(vram_address, 0x40))
							attribute_latch >>= 4;
						if (testFlag(vram_address, 0x02))
							attribute_latch >>= 2;
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
					clearVBlank();
				}
				break;
			case 321:
			case 339: address = nametableAddress(); break;

			// nametable fetch instead of attribute
			case 338: nametable_latch = read(address); break;
			case 340:
				nametable_latch = read(address);
		}

		// signal scanline to MMC3
		if (renderingEnabled()
				&& ((control & 0x10)
				? (cycle == 324 || cycle == 4)
				: (cycle == 260))) {
			cartridge->signalScanlineMMC3();
		}
	}

	// signal scanline to MMC5
	if ((scanline <= 240) && cycle == 4) {
		cartridge->signalScanlineMMC5();
	}
}

void incrementXComponent() {
	if (!renderingEnabled()) return;

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

void incrementYComponent() {
	if (!renderingEnabled()) return;

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

void updateVRAMX() {
	if (!renderingEnabled()) return;
	static const int X_COMPONENT = 0x041f;
	vram_address = (vram_address & ~X_COMPONENT) | (temp_vram_address & X_COMPONENT);
}

void updateVRAMY() {
	if (!renderingEnabled()) return;
	static const int Y_COMPONENT = 0x7be0;
	vram_address = (vram_address & ~Y_COMPONENT) | (temp_vram_address & Y_COMPONENT);
}

Word nametableAddress() {
	return 0x2000 | (vram_address & 0x0fff);
}

Word attributeAddress() {
	return 0x23c0
		| (vram_address & 0x0c00)
		| ((vram_address >> 4) & 0x38)
		| ((vram_address >> 2) & 0x07);
}

Word backgroundAddress() {
	return (testFlag(control, BACKGROUND_TILE_SELECT) * 0x1000)
		+ (nametable_latch * 16)
		+ ((vram_address >> 12) & 0x07);
}

void incrementVRAMAddress() {
	if (((scanline < 240) || (scanline == PRERENDER_SCANLINE)) && renderingEnabled()) {
		// this will happen if PPU_DATA read/writes occur during rendering
		incrementXComponent();
		incrementYComponent();
	} else {
		vram_address += testFlag(control, INCREMENT_MODE) ? 32 : 1;
	}
}

void writeToOAM(Byte value) {
	primary_oam[oam_address++] = value;
}

// PPU read from cartridge / ram
Byte read(Word address) {
	Byte value;
	switch (address) {
		case CHR_START ... CHR_END:
			value = cartridge->readCHR(address);
			break;

		case NAMETABLE_START ... NAMETABLE_END: {
			int nt_index = (address - NAMETABLE_START) % NAMETABLE_MIRROR_SIZE;
			Byte* nt_data = nt_map[nt_index / KB];
			value = nt_data[nt_index % KB];
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
void write(Word address, Byte value) {
	switch(address) {
		case CHR_START ... CHR_END:
			cartridge->writeCHR(address, value);
			break;

		case NAMETABLE_START ... NAMETABLE_END: {
			int nt_index = (address - NAMETABLE_START) % NAMETABLE_MIRROR_SIZE;
			Byte* nt_data = nt_map[nt_index / KB];
			nt_data[nt_index % KB] = value;
			break;
		}

		case PALETTE_START ... PALETTE_END:
			if ((address & 0x13) == 0x10) address &= ~0x10;
			palette[address % PALETTE_SIZE] = value;
			break;
	}
}

void loadShiftRegisters() {
	bg_shift_low = (bg_shift_low & 0xff00) | bg_latch_low;
	bg_shift_high = (bg_shift_high & 0xff00) | bg_latch_high;

	attribute_latch_low = attribute_latch & 1;
	attribute_latch_high = attribute_latch & 2;
}

// load secondary oam for sprites in next scanline
void loadSpritesOnScanline() {
	sprite_zero_next_scanline = false;
	int j = 0;
	int sec_size = secondaryOamSize();
	for(int i = 0; i < PRIMARY_OAM_SIZE; i++) {
		Byte* object = primary_oam + i * OBJECT_SIZE;
		int line = ((scanline == 261) ? -1 : scanline)
			- object[Y_POS];

		if ((line >= 0) && (line < spriteHeight())) {
			// sprite on scanline
			if (i == 0) {
				sprite_zero_next_scanline = true;
			}

			if (j < sec_size) {
				Byte* secondary_object = secondary_oam + j * OBJECT_SIZE;

				// copy sprite data
				for(int a = 0; a < OBJECT_SIZE; a++) {
					secondary_object[a] = object[a];
				}
			}

			// increment secondary OAM index
			if (++j == (SECONDARY_OAM_SIZE + 1)) {
				if (renderingEnabled()) {
					setStatusFlag(SPRITE_OVERFLOW);
				}
				if (sprite_flickering) break;
			}
		}
	}
}

void loadSpriteRegisters() {
	Word address;
	sprite_zero_this_scanline = sprite_zero_next_scanline;

	int size = secondaryOamSize();
	for(int i = 0; i < size; i++) {
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

void renderPixel() {
	if (renderingEnabled()) {
		Byte palette = 0;
		Byte obj_palette = 0;
		bool obj_priority = 0;
		int x = cycle - 2;

		if (scanline < 240 && x >= 0 && x < 256) {
			
			// background
			bool show_background = testFlag(mask, SHOW_BACKGROUND);
			if (show_background && (testFlag(mask, SHOW_BKG_LEFT_8) || (x >= 8))) {
				palette = (bit(bg_shift_high, 15 - fine_x_scroll) << 1)
					| bit(bg_shift_low, 15 - fine_x_scroll);
				if (palette) {
					palette |= ((bit(attribute_shift_high, 7 - fine_x_scroll) << 1)
						| bit(attribute_shift_low, 7 - fine_x_scroll)) << 2;
				}
			}

			//sprites
			bool show_sprites = testFlag(mask, SHOW_SPRITES);
			if (show_sprites && (testFlag(mask, SHOW_SPR_LEFT_8) || (x >= 8))) {
				int size = secondaryOamSize();
				for(int i = size-1; i >= 0; i--) {
					Byte attributes = sprite_attribute_latch[i];

					int sprite_x = x - sprite_x_counter[i];
					if (sprite_x < 0 || sprite_x >= 8 || x >= 255) continue; // not in range

					if (testFlag(attributes, FLIP_HOR)) {
						sprite_x ^= 0x07;
					}

					Byte spr_palette = (bit(sprite_shift_high[i], 7 - sprite_x) << 1)
						| bit(sprite_shift_low[i], 7 - sprite_x);

					if ((spr_palette & 0x3) == 0) continue; // transparent

					if (sprite_zero_this_scanline && (i == 0) // hit sprite 0
							&& show_background && palette // background not transparent
							&& (x < 255)) { // doesn't check at x=255
						sprite_zero_this_scanline = false;
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

			int palette_index = read(PALETTE_START + (renderingEnabled() ? palette : 0));
			int y = (SCREEN_HEIGHT - 1 - scanline);
			screen[y * SCREEN_WIDTH + x] = Pixel(nes_palette[palette_index]);
		}
	}

	// background_shifts

	bg_shift_low <<= 1;
	bg_shift_high <<= 1;

	attribute_shift_low = (attribute_shift_low << 1) | attribute_latch_low;
	attribute_shift_high = (attribute_shift_high << 1) | attribute_latch_high;
}

void dump() {
	print("===== PPU =====");
	print("rendering enabled: " << renderingEnabled());
	print("nt_map: [" << (void*)nt_map[0] << ", " << (void*)nt_map[1] << ", " << (void*)nt_map[2] << ", " << (void*)nt_map[3] << "]");

	print("\nCONTROL: " << toHex(control));
	int base_nt_addr = 0x2000 + (control & 0x03) * 0x0400;
	print("base NT addr: " << toHex(base_nt_addr, 2));
	print("vram inc: " << ((control & 0x04) ? "32" : "1"));
	bool spr16 = control & 0x20;
	print("spr addr: " << ((control & 0x80) ? "$1000" : "$0000") << (spr16 ? " (ignored)" : ""));
	print("bkg addr: " << ((control & 0x10) ? "$1000" : "$0000"));
	print("sprite size: " << (spr16 ? "8x16" : "8x8"));
	print("master/slave: " << (bool)(control & 0x40));
	print("nmi enabled: " << (bool)(control & 0x80));

	print("\nMASK: " << toHex(mask));
	print("greyscale: " << (bool)(mask & 0x01));
	print("bkr left 8: " << (bool)(mask & 0x02));
	print("spr left 8: " << (bool)(mask & 0x04));
	print("show bkr: " << (bool)(mask & 0x08));
	print("show spr: " << (bool)(mask & 0x10));
	print("emphasize red: " << (bool)(mask & 0x20));
	print("emphasize green: " << (bool)(mask & 0x40));
	print("emphasize blue: " << (bool)(mask & 0x80));

	print("\nSTATUS: " << toHex(status));
	print("sprite overflow: " << (bool)(status & 0x20));
	print("sprite 0 hit: " << (bool)(status & 0x40));
	print("vblank: " << (bool)(status & 0x80));
	print("===============");
}

bool nmiEnabled() {
	return testFlag(control, NMI_ENABLE);
}

} // end namespace