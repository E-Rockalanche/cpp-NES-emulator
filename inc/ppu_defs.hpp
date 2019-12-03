#ifndef PPU_DEFS_HPP
#define PPU_DEFS_HPP

namespace nes
{

	// strict order
	enum class PpuRegister
	{
		Control,
		/*
		write
		various flags controlling PPU operation
		*/

		Mask,
		/*
		write
		register controlling the rendering of sprites and backgrounds,
		as well as colour effects
		*/

		Status,
		/*
		read
		register effects the state of varous functions in PPU. Often used to determine timing.
		To determine when the PPU has reached a given pixel of the screen, put an opaque pixel of sprite 0 there
		*/

		OamAddress,
		/*
		write
		write the address of OAM you wanto to access here.
		Most games just write $00 here and use OAM_DMA
		(DMA implemented in 2A03/7 chip. Works by repeatedly writing to OAM_DATA)
		*/

		OamData,
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

		Scroll,
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

		Address,
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

		Data,
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

		OamDma = 0x4014
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

	enum PPUControlFlag
	{
		Nametable0 = 1 << 0, // 1: x scroll += 256
		Nametable1 = 1 << 1, // 1: y scroll += 240
		/*
		base nametable address
		0: $2000
		1: $2400
		2: $2800
		3: $2c00
		*/

		IncrementMode = 1 << 2,
		/*
		VRAM address increment per CPU read/write of PPU_DATA
		0: add 1 (going across)
		1: add 32 (going down)
		*/

		SpriteTileSelect = 1 << 3,
		/*
		sprite pattern table address for 8x8 sprites
		(ignored in 8x16 mode)
		0: $0000
		1: $1000
		*/

		BackgroundTileSelect = 1 << 4,
		/*
		background pattern table address
		0: $0000
		1: $1000
		*/

		SpriteHeight = 1 << 5,
		/*
		sprite size
		0: 8x8
		1: 8x16
		*/

		MasterSlave = 1 << 6,
		/*
		PPU master/slave select
		0: read backdrop from EXT pins
		1: output colour on EXT pins
		*/

		NmiEnable = 1 << 7
					 /*
					 generate an NMI at the start of the vertical blanking interval
					 0: off
					 1: on
					 */
	};

	enum PPUMaskFlag
	{
		GreyScale = 1 << 0,
		/*
		0: normal colour
		1: greyscale display
		*/

		ShowBackgroundLeft8 = 1 << 1,
		/*
		1: show background in leftmost 8 pixels of screen
		0: hide
		*/

		ShowSpriteLeft8 = 1 << 2,
		/*
		1: show sprites in leftmost 8 pixels of screen
		0: hide
		*/

		ShowBackground = 1 << 3,
		ShowSprite = 1 << 4,
		Red = 1 << 5, // green on PAL
		Green = 1 << 6, // red on PAL
		Blue = 1 << 7
	};

	enum PPUStatusFlag
	{
		SpriteOverflow = 1 << 5,
		/*
		Set when more than 8 sprites appear on a scanline.
		A hardware bug causes this to give false positives and negatives.
		Set during sprite evaluation and cleared at dot 1 (second dot) of the pre-render line.
		*/

		Sprite0Hit = 1 << 6,
		/*
		Set when a nonzero pixel of sprite 0 overlaps with a nonzero background pixel.
		Cleared at dot 1 of the pre-render line.
		Used for raster timing
		*/

		VBlank = 1 << 7,
		/*
		Set at dot 1 of line 241 (line after post-render line).
		Cleared after reading $2002 and at dot 1 of the pre-render line
		0: not in vblank
		1: in vblank
		*/
	};

} // namespace nes

#endif