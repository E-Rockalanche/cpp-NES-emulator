#ifndef CARTRIDGE_HPP
#define CARTRIDGE_HPP

#include <string>
#include "common.hpp"

class Cartridge {
public:
	Cartridge();
	~Cartridge();

	bool loadFile(const std::string filename);
	bool loaded();

	bool verify();
	/*
	returns true if data is correctly formatted
	*/

	int expectedSize();

	int prgRomSize();
	Byte* getPrgRom();
	/*
	program ROM
	*/

	int chrRomSize();
	Byte* getChrRom();
	/*
	graphics ROM
	*/

	bool hasPrgRam();
	int prgRamSize();
	/*
	battery backed RAM
	*/

	int mapperNumber();

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
	NameTableMirroring nameTableMirroring();

	enum Format {
		ARCHAIC_INES,
		INES,
		NES2
	};
	Format getFormat();

private:
	enum HeaderSection {
		N,
		E,
		S,
		MSDOS_EOF,
		PRG_ROM_SIZE, // 16 KB units
		CHR_ROM_SIZE, // 8 KB units. 0 implies this cartridge has PRG RAM
		FLAGS_6,
		FLAGS_7,
		PRG_RAM_SIZE, // 8 KB units. 0 implies 8 KB for compatibility
		FLAGS_9,
		FLAGS_10,
		/*
		sections 11-15 are zero-filled
		*/
		HEADER_SIZE = 16
	};

	std::string filename;
	Byte* data;
	int data_size;
	
	int prgRomStart();
	int chrRomStart();
	void allocateMemory(int size);
	void freeData();
};

#endif