#ifndef MAPPER5_HPP
#define MAPPER5_HPP

#include "cartridge.hpp"

// MMC5
class Mapper5 : public Cartridge {
public:
	Mapper5(Byte* data);
	~Mapper5() {}

	void reset();

	Byte readPRG(Word address);
	void writePRG(Word address, Byte value);

	Byte readCHR(Word address);
	void writeCHR(Word address, Byte value);

	void signalVBlank();
	void signalHBlank();
	void signalHRender();
	void signalScanlineMMC5();

protected:
	enum Register {
		AUDIO_START = 0x5000,
		AUDIO_END = 0x5015,

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

		CHR_BANKSWITCH_A_START = 0x5120,
		CHR_BANKSWITCH_A_END = 0x5127,
		
		CHR_BANKSWITCH_B_START = 0x5128,
		CHR_BANKSWITCH_B_END = 0x512b,

		UPPER_CHR_BITS = 0x5130,

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

		EXT_RAM_START = 0x5c00,
		EXT_RAM_END = 0x5fff,
		EXT_RAM_SIZE = 0x0400
	};

	enum {
		PRG_MODE_32K,
		PRG_MODE_16K,
		PRG_MODE_16K_8K,
		PRG_MODE_8K
	};

	enum {
		CHR_MODE_8K,
		CHR_MODE_4K,
		CHR_MODE_2K,
		CHR_MODE_1K
	};

	enum ChrSet {
		CHR_SET_A,
		CHR_SET_B
	};

	enum VSplitSide {
		LEFT,
		RIGHT
	};

	enum SpriteSize {
		SPR_8x8,
		SPR_8x16
	};

	enum ExtRamMode {
		EXTRA_NAMETABLE,
		EXT_ATTRIBUTE_DATA,
		RAM_RW,
		RAM_READ_ONLY
	};

	enum NametableMode {
		NT_VRAM_0,
		NT_VRAM_1,
		NT_EXT_RAM,
		NT_FILL_MODE,
	};

	enum FetchMode {
		FETCH_NONE,
		FETCH_BACKGROUND,
		FETCH_SPRITE
	};

	SpriteSize sprite_size;
	FetchMode fetch_mode;
	bool rendering_enabled;

	union Registers {
		Byte r[8];
		struct {
			Byte prg_mode;
			Byte chr_mode;
			Byte ram_protect_1;
			Byte ram_protect_2;
			Byte ext_ram_mode;
			Byte nt_mapping;
			Byte fill_tile;
			Byte fill_colour;
		};
	};

	Registers registers;

	Byte prg_registers[5];
	Byte chr_registers_a[8];
	Byte chr_registers_b[4];
	int upper_chr_bits;
	ChrSet last_chr_set;

	int vsplit_tile;
	VSplitSide vsplit_side;
	bool vsplit_enabled;
	Byte vsplit_scroll;
	Byte vsplit_bank;

	Byte irq_scanline;
	bool irq_enabled;
	bool irq_pending;
	bool in_frame;
	int current_scanline;

	Byte multiplicand;
	Byte multiplier;
	Word result;

	Byte ext_ram[EXT_RAM_SIZE];

	SpriteSize spriteSize();

	void setPRGBankExt(int slot, int bank, int bank_size);
	void applyPRG();
	void applyChrA();
	void applyChrB();
};

#endif