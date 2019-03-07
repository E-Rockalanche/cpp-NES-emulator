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

	Byte readROM(Word address);
	virtual void writeROM(Word address, Byte value) {}

	Byte readCHR(Word address);
	virtual void writeCHR(Word address, Byte value) {}

	virtual void signal_scanline() {};

	NameTableMirroring nameTableMirroring();

protected:
	enum HeaderSection {
		NES,
		ROM_SIZE = 4, // 16 KB units
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

	Byte* data;

	Byte* rom;
	int rom_size;

	Byte* chr;
	int chr_size;
	bool has_chr_ram = false;

	Byte* ram;
	int ram_size;

	static Format getFormat(Byte* data);
	static bool verifyHeader(Byte* data);
	static int getMapperNumber(Byte* data);
};

#endif