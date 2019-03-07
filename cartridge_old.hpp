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

	// returns true if header is correctly formatted
	bool verify();

	int expectedSize();

	Byte readROM(Word address);
	void writeROM(Word address, Byte value);

	Byte readCHR(Word address);
	void writeCHR(Word address, Byte value);

	bool hasPrgRam();
	int prgRamSize();

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
		NES,
		PRG_ROM_SIZE = 4, // 16 KB units
		CHR_ROM_SIZE, // 8 KB units. 0 implies this cartridge has PRG RAM
		FLAGS_6,
		FLAGS_7,
		PRG_RAM_SIZE, // 8 KB units. 0 implies 8 KB for compatibility
		FLAGS_9,
		FLAGS_10,
		/*
		sections 11-15 should be zero-filled
		*/
		HEADER_SIZE = 16
	};

	std::string filename;
	
	Byte* data;
	Byte* prg_rom;
	Byte* chr_rom;

	int data_size;
	int prg_size;
	int chr_size;
	
	int prgRomStart();
	int chrRomStart();
	int prgRomSize();
	int chrRomSize();
	void allocateMemory(int size);
	void freeData();
};

#endif