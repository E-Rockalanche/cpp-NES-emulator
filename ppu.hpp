#ifndef PPU_HPP
#define PPU_HPP

class PPU;

#include "cpu.hpp"
#include "common.hpp"

class PPU {
public:
	PPU();
	~PPU();
	void setROM(Byte* data, unsigned int size);
	Byte readByte(Word address);
	void writeByte(Word address, Byte value);
	void clockTick();
	void reset();
	void setCPU(CPU* cpu);
	const Pixel* getSurface();
	bool readyToDraw();
	void writeToOAM(Byte index, Byte value);

	static const int SCREEN_WIDTH = 256;
	static const int SCREEN_HEIGHT = 240;

	/*
	OAM: Object Attribute Memory
	Contains a display list of up to 64 sprites.
	Each sprites' info occupies 4 bytes

	0: y position of top of sprite
	1: tile index number
	2: attributes
	3: x position of left side of sprite
	*/

	enum Register {
		PPU_CONTROL = 0x2000,
		/*
		write
		various flags controlling PPU operation
		*/

		PPU_MASK,
		/*
		write
		register controlling the rendering of sprites and backgrounds,
		as well as colour effects
		*/

		PPU_STATUS,
		/*
		read
		register effects the state of varous functions in PPU. Often used to determine timing.
		To determine when the PPU has reached a given pixel of the screen, put an opaque pixel of sprite 0 there
		*/

		OAM_ADDRESS,
		/*
		write
		write the address of OAM you wanto to access here.
		Most games just write $00 here and use OAM_DMA
		(DMA implemented in 2A03/7 chip. Works by repeatedly writing to OAM_DATA)
		*/

		OAM_DATA,
		/*
		read/write
		write OAM data here. Writes will increment OAM_ADDRESS
		reads during vblank return the value from OAM but do not increment address
		Writes to oam through this register are slow, so are only good for partial updates
		Most games use the DMA feature through OAM_DMA instead

		Reading oam data while the ppu is rendering will expose internal oam accesses.
		Writes to oam data during rendering (on the pre-render line and visible lines)
		do not modify values in OAM, but do perform glitchy increment of oam address,
		bumping only the high 6 bits. This extends to DMA tranfers, since it uses
		writes to OAM_DATA. Probably best to ignore writes during rendering for emulation.
		Old versions of the PPU did not support reading from this port!
		*/

		PPU_SCROLL,
		/*
		write x2
		used to set scroll position. Tells the PPU which pixel offset of the
		nametable selected through PPU_CONTROL should be in the top left corner
		Typically this register is written to during vertical blanking, but it
		can also be written to during rendering to split the screen.
		Changes made to vertical scroll during rendering will only take effect
		on the next frame.
		Write the horizontal and vertical scroll positions after resetting the
		address latch by reading PPU_STATUS

		Horizontal offsets range from 0-255
		Normal vertical offsets range from 0-239 while values of 240-255
		are treated as -16 to -1 in a way, but tile data is fetched incorrectly
		from the attribute table

		note: PPU_SCROLL and PPU_ADDRESS use the same write toggle
		*/

		PPU_ADDRESS,
		/*
		write x2
		CPU and PPU are on seperate busses.
		The CPU writes to VRAM through a pair of registers on the PPU.
		First it loads an address to PPU_ADDRESS, then writes repeatedly to
		PPU_DATA to fill VRAM
		write the 16 bit address of VRAM after reading PPU_STATUS to reset the
		address latch (upper byte first)
		valid addresses are $0000-$3fff. Higher addresses are mirrored down
		*/

		PPU_DATA,
		/*
		read/write
		VRAM read/write data register
		after access, the VRAM address will increment by amount determined by
		PPU_CONTROL bit 2.
		during VBLANK or when rendering is disabled data can be read or written
		to through this port. Accessing PPU_DATA during rendering will create
		graphical glitches since it increments the VRAM address
		VRAM reading/writing uses the same internal address register that
		rendering uses. So after loading data into VRAM the program should
		reload the scroll position afterwards in order to avoid wrong scrolling
		*/

		OAM_DMA = 0x4014
		/*
		write
		Writing $XX will upload 256 bytes of data from CPU page $XX00-$XXff to
		internal PPU OAM.
		This page is typically located in internal RAM, commonly $0200-$02ff,
		but cartridge RAM or ROM can be used as well
		The CPU is suspended during transfer, which takes 513 or 514 cycles
		after $4014 write tick (1 dummy read cycle while waiting for writes to
		complete, +1 if on add odd CPU cycle, then 256 alternating read/write
		cycles)
		Only effective method for transfering all 256 bytes of OAM
		DMA transfer will begin at the current oam write address.
		It is common to initialize oam address to 0 before DMA transfer
		*/
	};

	enum ControlFlag {
		NAMETABLE_0 = BL(0), // 1: x scroll += 256
		NAMETABLE_1 = BL(1), // 1: y scroll += 240
		/*
		base nametable address
		0: $2000
		1: $2400
		2: $2800
		3: $2c00
		*/

		INCREMENT_MODE = BL(2),
		/*
		VRAM address increment per CPU read/write of PPU_DATA
		0: add 1 (going across)
		1: add 32 (going down)
		*/

		SPRITE_TILE_SELECT = BL(3),
		/*
		sprite pattern table address for 8x8 sprites
		(ignored in 8x16 mode)
		0: $0000
		1: $1000
		*/

		BACKGROUND_TILE_SELECT = BL(4),
		/*
		background pattern table address
		0: $0000
		1: $1000
		*/

		SPRITE_HEIGHT = BL(5),
		/*
		sprite size
		0: 8x8
		1: 8x16
		*/

		MASTER_SLAVE = BL(6),
		/*
		PPU master/slave select
		0: read backdrop from EXT pins
		1: output colour on EXT pins
		*/

		NMI_ENABLE = BL(7)
		/*
		generate an NMI at the start of the vertical blanking interval
		0: off
		1: on
		*/
	};

	enum MaskFlag {
		GREYSCALE = BL(0),
		/*
		0: normal colour
		1: greyscale display
		*/

		SHOW_BKG_LEFT_8 = BL(1),
		/*
		1: show background in leftmost 8 pixels of screen
		0: hide
		*/

		SHOW_SPR_LEFT_8 = BL(2),
		/*
		1: show sprites in leftmost 8 pixels of screen
		0: hide
		*/

		SHOW_BACKGROUND = BL(3),
		SHOW_SPRITES = BL(4),
		RED = BL(5), // green on PAL
		GREEN = BL(6), // red on PAL
		BLUE = BL(7)
	};

	enum StatusFlag {
		SPRITE_OVERFLOW = BL(5),
		/*
		Set when more than 8 sprites appear on a scanline.
		A hardware bug causes this to give false positives and negatives.
		Set during sprite evaluation and cleared at dot 1 (second dot) of the pre-render line.
		*/

		SPRITE_0_HIT = BL(6),
		/*
		Set when a nonzero pixel of sprite 0 overlaps with a nonzero background pixel.
		Cleared at dot 1 of the pre-render line.
		Used for raster timing
		*/

		VERTICAL_BLANK = BL(7),
		/*
		Set at dot 1 of line 241 (line after post-render line).
		Cleared after reading $2002 and at dot 1 of the pre-render line
		0: not in vblank
		1: in vblank
		*/
	};

private:
	template <class Boolean>
	void setStatusFlag(int flag, Boolean value = Constant<bool, true>());
	void incrementDot();
	void incrementYComponent();
	void incrementXComponent();
	void incrementVRAMAddress();
	Word getTileAddress();
	Word getAttributeAddress();
	void writeToControl(Byte value);
	void writeToScroll(Byte value);
	void writeToAddress(Byte value);
	void renderPixel();
	void setVBlank();

	static const int PRERENDER_SCANLINE = 261;
	static const int MAX_SCANLINE = 261;
	/*
	dummy scanline
	purpose is to fill shift registers with the data for the first two tiles to be rendered
	no pixels are rendered but the PPU still makes the same memory accesses it would for a regular scanline
	*/
	static const int POSTRENDER_SCANLINE = 240;
	/*
	The PPU idles during this scanline.
	Accessing PPU memory is safe here but VBLANK is set next scanline
	*/
	static const int VBLANK_SCANLINE = 241;
	/*
	VBLANK flag set on second tick of this scanline (and the NMI occurs)
	The PPU makes no memory accesses during the VBLANK scanlines, so the PPU memory can be freely accessed by the program
	*/
	static const int MAX_CYCLE = 340;

	static const int VRAM_SIZE = 2 * KB;
	Byte vram[VRAM_SIZE];
	static const int OAM_SIZE = 64 * 4;
	Byte oam[OAM_SIZE];

	bool can_draw;

	CPU* cpu;

	Byte* chr_rom;
	unsigned int chr_rom_size;
	Byte* chr_ram;
	unsigned int chr_ram_size;

	static const int BOOTUP_CYCLES = 30000;
	int wait_cycles;

	bool write_toggle; // used to set x_scroll/y_scroll and vram_address
	Byte control;
	Byte mask;
	Byte status;
	Byte oam_address;
	Word vram_address; // 15 bits: yyyNNYYYYYXXXXX (fine y scroll, nametable select, coarse Y scroll, coarse X scroll)
	Word temp_vram_address; // 15 bits
	Byte fine_x_scroll; // 3 bits
	bool in_vblank;
	Byte oam_data_high;

	Word tile_bitmap_shift_registers[2];
	Byte palette_attribute_registers[2];

	Byte sprite_bitmap_registers[8];
	Byte sprite_attribute_latches[8];
	Byte sprite_x_counters[8];

	int cycle;
	int scanline;
	int dot;
	bool skip_cycle;

	Pixel surface[SCREEN_WIDTH * SCREEN_HEIGHT];
};

#endif