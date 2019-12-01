#ifndef CARTRIDGE_HPP
#define CARTRIDGE_HPP

class Cartridge;

#include "common.hpp"
#include "cpu.hpp"
#include "assert.hpp"

class Cartridge /* aka mapper0 */ {
public:
	enum NameTableMirroring {
		HORIZONTAL,
		/*
		       000 400
		0x2000 A   A
		0x2800 B   B
		*/

		VERTICAL
		/*
		       000 400
		0x2000 A   B
		0x2800 A   B
		*/
	};

	static Cartridge* loadFile(std::string);

	Cartridge(Byte* data);
	virtual ~Cartridge();
	virtual void reset();

	Byte readPRG(Word address);
	virtual void writePRG(Word address, Byte value) {}

	Byte readCHR(Word address);
	virtual void writeCHR(Word address, Byte value) {}

	bool hasSRAM();
	NameTableMirroring nameTableMirroring();
	unsigned int getChecksum();

	bool saveGame(std::string);
	bool loadSave(std::string);

	virtual void signalScanline() {}

	virtual void saveState(std::ostream& out);
	virtual void loadState(std::istream& in);

protected:
	enum Header {
		NES,
		PRG_SIZE = 4, // 16 KB units
		CHR_SIZE, // 8 KB units. 0 implies this cartridge has CHR RAM
		FLAGS_6,
		/*
		76543210
		||||||||
		|||||||+- Mirroring: 0: horizontal (vertical arrangement) (CIRAM A10 = PPU A11)
		|||||||              1: vertical (horizontal arrangement) (CIRAM A10 = PPU A10)
		||||||+-- 1: Cartridge contains battery-backed PRG RAM ($6000-7FFF) or other persistent memory
		|||||+--- 1: 512-byte trainer at $7000-$71FF (stored before PRG data)
		||||+---- 1: Ignore mirroring control or above mirroring bit; instead provide four-screen VRAM
		++++----- Lower nybble of mapper number
		*/
		FLAGS_7,
		/*
		76543210
		||||||||
		|||||||+- VS Unisystem
		||||||+-- PlayChoice-10 (8KB of Hint Screen data stored after CHR data)
		||||++--- If equal to 2, flags 8-15 are in NES 2.0 format
		++++----- Upper nybble of mapper number
		*/
		RAM_SIZE, // 8 KB units. 0 implies 8 KB for compatibility
		FLAGS_8 = 8,
		FLAGS_9,
		/*
		76543210
		||||||||
		|||||||+- TV system (0: NTSC; 1: PAL)
		+++++++-- Reserved, set to zero

		NES2.0
		7654 3210
		---------
		CCCC PPPP
		|||| ++++- PRG-ROM size MSB
		++++------ CHR-ROM size MSB
		*/
		FLAGS_10,
		/*
		76543210
		  ||  ||
		  ||  ++- TV system (0: NTSC; 2: PAL; 1/3: dual compatible)
		  |+----- PRG RAM ($6000-$7FFF) (0: present; 1: not present)
		  +------ 0: Board has no bus conflicts; 1: Board has bus conflicts
		
		NES2.0
		7654 3210
		---------
		pppp PPPP
		|||| ++++- PRG-RAM (volatile) shift count
		++++------ PRG-NVRAM/EEPROM (non-volatile) shift count
		If the shift count is zero, there is no PRG-(NV)RAM.
		If the shift count is non-zero, the actual size is
		"64 SHL shift count" bytes, i.e. 8192 bytes for a shift count of 7.
		*/

		// NES2.0 onwards
		FLAGS_11,
		/*
		7654 3210
		---------
		cccc CCCC
		|||| ++++- CHR-RAM size (volatile) shift count
		++++------ CHR-NVRAM size (non-volatile) shift count
		If the shift count is zero, there is no CHR-(NV)RAM.
		If the shift count is non-zero, the actual size is
		"64 SHL shift count" bytes, i.e. 8192 bytes for a shift count of 7.
		*/
		FLAGS_12,
		/*
		7654 3210
		---------
		.... ..VV
			   ++- CPU/PPU timing mode
			        0: RP2C02 ("NTSC NES")
			        1: RP2C07 ("Licensed PAL NES")
			        2: Multiple-region
			        3: UMC 6527P ("Dendy")
		*/
		FLAGS_13,
		/*
		7654 3210
		---------
		MMMM PPPP
		|||| ++++- Vs. PPU Type
		++++------ Vs. Hardware Type
		*/
		FLAGS_14,
		/*
		7654 3210
		---------
		.... ..RR
               ++- Number of miscellaneous ROMs present
		*/
		FLAGS_15,
		/*
		7654 3210
		---------
		..DD DDDD
          ++-++++- Default Expansion Device
		*/
		HEADER_SIZE
	};

	enum Format {
		ARCHAIC_INES,
		INES,
		NES2
	};

	static const int RAM_START = 0x6000;
	static const int PRG_START = 0x8000;

	static const int MIN_PRG_BANK_SIZE = 0x2000;
	static const int MIN_CHR_BANK_SIZE = 0x0400;

	Byte* data;

	Byte* prg;
	int prg_size;

	Byte* chr;
	int chr_size;
	bool has_chr_ram = false;

	Byte* ram;
	int ram_size;

	NameTableMirroring nt_mirroring;

	unsigned int checksum;

	// bank switching
	int prg_map[4];
	int chr_map[8];

	static Format getFormat(Byte* data);
	static bool verifyHeader(Byte* data);
	static int getMapperNumber(Byte* data);

	void setPRGBank(int slot, int bank, int bank_size);
	void setCHRBank(int slot, int bank, int bank_size);
};

extern Cartridge* cartridge;

#endif