#ifndef CARTRIDGE_HPP
#define CARTRIDGE_HPP

#include "common.hpp"
#include "debugging.hpp"

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

	static Cartridge* loadFile(const char* filename);

	Cartridge(Byte* data);
	virtual ~Cartridge();

	Byte readPRG(Word address);
	virtual void writePRG(Word address, Byte value) {}

	Byte readCHR(Word address);
	virtual void writeCHR(Word address, Byte value) {}

	NameTableMirroring nameTableMirroring();

protected:
	enum HeaderSection {
		NES,
		PRG_SIZE = 4, // 16 KB units
		CHR_SIZE, // 8 KB units. 0 implies this cartridge has PRG RAM
		FLAGS_6,
		FLAGS_7,
		RAM_SIZE, // 8 KB units. 0 implies 8 KB for compatibility
		FLAGS_9,
		FLAGS_10,
		/*
		sections 11-15 should be zero-filled
		*/
		HEADER_SIZE = 16
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

	// bank switching
	int prg_map[4];
	int chr_map[8];

	static Format getFormat(Byte* data);
	static bool verifyHeader(Byte* data);
	static int getMapperNumber(Byte* data);

	void setPRGBank(int slot, int bank, int bank_size);
	void setCHRBank(int slot, int bank, int bank_size);
};

#endif