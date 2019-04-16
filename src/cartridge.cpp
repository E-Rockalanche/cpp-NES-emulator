#include <fstream>
#include <iostream>
#include <cstring>

#include "cartridge.hpp"
#include "mapper1.hpp"
#include "mapper2.hpp"
#include "mapper3.hpp"
#include "mapper4.hpp"
#include "crc32.hpp"
#include "assert.hpp"

Cartridge* cartridge = NULL;

Cartridge* Cartridge::loadFile(std::string filename) {
	Cartridge* mapper = NULL;
	int data_size = 0;
	Byte* data = NULL;

	std::ifstream fin(filename, std::ios::binary);
	if (!fin.is_open() || !fin.good() || fin.eof()) {
		dout("Could not open " << filename);
		return NULL;
	}

	// check file size
	fin.seekg(0, std::ios::end);
	data_size = fin.tellg();
	fin.seekg(0);

	// allocate memory
	data = new Byte[data_size];

	if (data == NULL) {
		dout("Could not allocate memory");
		fin.close();
		return NULL;
	}

	// read file
	fin.read((char*)data, data_size);
	fin.close();

	if (data_size < (HEADER_SIZE + 8*KB)) {
		dout("File size is too small to be a ROM");
		dout("size: " << toHex(data_size));
		delete[] data;
		return NULL;
	}

	if (!verifyHeader(data)) {
		dout("file verification failed");
		delete[] data;
		return NULL;
	}

	if ((data[FLAGS_7] & 0x0c) == 0x08) {
		dout("NES 2.0 format not supported");
		delete[] data;
		return NULL;
	}

	int mapper_number = getMapperNumber(data);
	switch(mapper_number) {
		case 0: mapper = new Cartridge(data); break;
		case 1: mapper = new Mapper1(data); break;
		case 2: mapper = new Mapper2(data); break;
		case 3: mapper = new Mapper3(data); break;
		case 4: mapper = new Mapper4(data); break;
		default: assert(false, "mapper " << mapper_number << " is not supported");
	}

	return mapper;
}

bool Cartridge::verifyHeader(Byte* data) {
	bool ok = true;
	static const char nes[4] = {'N', 'E', 'S', 0x1a};
	for(int n = 0; n < 4; n++) {
		if (data[n] != nes[n]) {
			ok = false;
			dout("NES verification failed");
			break;
		}
	}

	return ok;
}

int Cartridge::getMapperNumber(Byte* data) {
	int low = (data[FLAGS_6] >> 4);
	int high = (getFormat(data) == NES2) ? (data[FLAGS_7] & 0xf0) : 0;
	return high | low;
}

Cartridge::Format Cartridge::getFormat(Byte* data) {
	switch(data[FLAGS_7] & 0x0c) {
		case 0: return INES;
		case 8: return NES2;
		default: return ARCHAIC_INES;
	}
}

Cartridge::Cartridge(Byte* data) : data(data) {
	prg_size = data[PRG_SIZE] * 0x4000;
	chr_size = data[CHR_SIZE] * 0x2000;
	ram_size = data[RAM_SIZE] ? (data[RAM_SIZE] * 0x2000) : 0x2000;

	checksum = crc32(data, HEADER_SIZE + prg_size + chr_size);

	ram = new Byte[ram_size];
	for(int n = 0; n < ram_size; n++) ram[n] = 0;

	prg = data + HEADER_SIZE;

	if (chr_size) {
		has_chr_ram = false;
		chr = prg + prg_size;
	} else {
		has_chr_ram = true;
		chr_size = 0x2000;
		chr = new Byte[chr_size];
	}

	nt_mirroring = (data[FLAGS_6] & 1) ? VERTICAL : HORIZONTAL;


	setPRGBank(0, 0, 0x8000);
	setCHRBank(0, 0, 0x2000);
}

Cartridge::~Cartridge() {
	delete[] data;
	delete[] ram;
	if (has_chr_ram) delete[] chr;
}

unsigned int Cartridge::getChecksum() { return checksum; }

bool Cartridge::hasSRAM() {
	return data[FLAGS_6] & 0x02;
}

bool Cartridge::saveGame(std::string filename) {
	if (!hasSRAM()) {
		dout("Cartridge does not have SRAM");
		return false;
	}

	std::ofstream fout(filename, std::ios::binary);
	if (!fout.is_open()) {
		dout("Could not save to " << filename);
		return false;
	}

	fout.write((const char*)(const char*)ram, ram_size);
	fout.close();

	return true;
}

bool Cartridge::loadSave(std::string filename) {
	if (!hasSRAM()) {
		dout("Cartridge does not have SRAM");
		return false;
	}

	std::ifstream fin(filename, std::ios::binary);
	if (!fin.is_open()) {
		dout("Could not load save from " << filename);
		return false;
	}

	// check file size
	fin.seekg(0, std::ios::end);
	int file_size = fin.tellg();
	fin.seekg(0);

	if (file_size != ram_size) {
		dout("Save file is not 8KB");
		fin.close();
		return false;
	}

	fin.read((char*)ram, ram_size);
	fin.close();

	return true;
}

Cartridge::NameTableMirroring Cartridge::nameTableMirroring() {
	return nt_mirroring;
}

Byte Cartridge::readPRG(Word address) {
	if (address >= PRG_START) {
		int rom_address = address - PRG_START;
		int offset = prg_map[rom_address / MIN_PRG_BANK_SIZE];
		return prg[offset + (rom_address % MIN_PRG_BANK_SIZE)];

	} else if (address >= RAM_START) {
		return ram[address - RAM_START];

	} else {
		dout("cartridge read unmapped at " << toHex(address, 2));
		return address >> 8; // unmapped
	}
}

Byte Cartridge::readCHR(Word address) {
	int offset = chr_map[address / MIN_CHR_BANK_SIZE];
	return chr[offset + (address % MIN_CHR_BANK_SIZE)];
}

void Cartridge::setPRGBank(int slot, int bank, int bank_size) {
	if (bank < 0) {
		bank = (prg_size / bank_size) + bank;
	}
	int map_size = bank_size / MIN_PRG_BANK_SIZE;
	for(int i = 0; i < map_size; i++) {
		prg_map[(slot * map_size) + i] = ((bank * bank_size) + (i * MIN_PRG_BANK_SIZE)) % prg_size;
	}
}

void Cartridge::setCHRBank(int slot, int bank, int bank_size) {
	int map_size = bank_size / MIN_CHR_BANK_SIZE;
	for(int i = 0; i < map_size; i++) {
		chr_map[(slot * map_size) + i] = ((bank * bank_size) + (i * MIN_CHR_BANK_SIZE)) % chr_size;
	}
}

struct SaveState {
	Cartridge::NameTableMirroring nt_mirroring;
	int prg_map[4];
	int chr_map[8];
};

void Cartridge::saveState(std::ostream& out) {
	SaveState ss;
	ss.nt_mirroring = nt_mirroring;
	std::memcpy(ss.prg_map, prg_map, sizeof(prg_map));
	std::memcpy(ss.chr_map, chr_map, sizeof(chr_map));

	out.write((char*)&ss, sizeof(SaveState));
	out.write((char*)ram, ram_size);

	if (has_chr_ram) {
		out.write((char*)chr, chr_size);
	}
}

void Cartridge::loadState(std::istream& in) {
	SaveState ss;

	in.read((char*)&ss, sizeof(SaveState));
	in.read((char*)ram, ram_size);

	if (has_chr_ram) {
		in.read((char*)chr, chr_size);
	}

	nt_mirroring = ss.nt_mirroring;
	std::memcpy(prg_map, ss.prg_map, sizeof(prg_map));
	std::memcpy(chr_map, ss.chr_map, sizeof(chr_map));
}