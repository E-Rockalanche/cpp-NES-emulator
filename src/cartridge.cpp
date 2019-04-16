#include <fstream>
#include <iostream>
#include <cstring>

#include "cartridge.hpp"
#include "1_mmc1.hpp"
#include "2_uxrom.hpp"
#include "3_cnrom.hpp"
#include "4_mmc3.hpp"
#include "5_mmc5.hpp"
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
		case 1: mapper = new MMC1(data); break;
		case 2: mapper = new UxROM(data); break;
		case 3: mapper = new CNROM(data); break;
		case 4: mapper = new MMC3(data); break;
		case 5: mapper = new MMC5(data); break;
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
	// int ram_size2 = 64 << data[FLAGS_10];
	dout("PRG size: " << toHex(prg_size));
	dout("CHR size: " << toHex(prg_size));
	dout("RAM size: " << toHex(ram_size));

	ram = new Byte[ram_size];
	assert(ram != NULL, "Could not allocate RAM");
	for(int n = 0; n < ram_size; n++) ram[n] = 0;

	prg = data + HEADER_SIZE;

	if (chr_size) {
		has_chr_ram = false;
		chr = prg + prg_size;
	} else {
		dout("using CHR RAM");
		has_chr_ram = true;
		chr_size = 0x2000;
		chr = new Byte[chr_size];
		assert(chr != NULL, "Could not allocate CHR RAM");
	}

	PPU::mapNametable(nameTableMirroring());

	setPRGBank(0, 0, 0x8000);
	setCHRBank(0, 0, 0x2000);
	ram_map[0] = ram;
}

Cartridge::~Cartridge() {
	delete[] data;
	delete[] ram;
	if (has_chr_ram) delete[] chr;
}

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
	return testFlag(data[FLAGS_6], 0x01) ? VERTICAL : HORIZONTAL;
}

Byte Cartridge::readPRG(Word address) {
	if (address >= PRG_START) {
		int rom_address = address - PRG_START;
		Byte* bank = prg_map[rom_address / MIN_PRG_BANK_SIZE];
		return bank[rom_address % MIN_PRG_BANK_SIZE];

	} else if (address >= RAM_START) {
		return ram_map[0][address - RAM_START];

	} else {
		dout("cartridge read unmapped at " << toHex(address, 2));
		return address >> 8; // unmapped
	}
}

Byte Cartridge::readCHR(Word address) {
	Byte* bank = chr_map[address / MIN_CHR_BANK_SIZE];
	return bank[address % MIN_CHR_BANK_SIZE];
}

void Cartridge::writePRG(Word address, Byte value) {}

void Cartridge::writeCHR(Word address, Byte value) {
	if (has_chr_ram) {
		chr[address % chr_size] = value;
	}
}

void Cartridge::setBank(DataSource data_map, DataSource data_src, int slot, int bank, int bank_size) {
	int min_size = 0;
	int src_size = 0;
	Byte* src = NULL;
	Byte** map = NULL;
	Bank* bank_map = NULL;

	switch(data_src) {
		case PRG:
			src_size = prg_size;
			src = prg;
			break;
		case CHR:
			src_size = chr_size;
			src = chr;
			break;
		case RAM:
			src_size = ram_size;
			src = ram;
			break;
		default:
			assert(false, "Invalid data source");
	}

	switch(data_map) {
		case PRG:
			map = prg_map;
			bank_map = prg_bank_map;
			min_size = MIN_PRG_BANK_SIZE;
			break;
		case CHR:
			map = chr_map;
			bank_map = chr_bank_map;
			min_size = MIN_CHR_BANK_SIZE;
			break;
		case RAM:
			map = ram_map;
			bank_map = ram_bank_map;
			min_size = MIN_RAM_BANK_SIZE;
			break;
		default:
			assert(false, "Invalid data map");
	}

	assert((bank_size % min_size) == 0, "Invalid bank size");

	// wrap
	if (bank < 0) {
		bank = (src_size / bank_size) + bank;
	}

	// number of map slots to fill
	int map_size = bank_size / min_size;

	for(int i = 0; i < map_size; i++) {
		int map_index = (slot * map_size) + i;
		map[map_index] = src + (((bank * bank_size) + (i * min_size)) % src_size);
		bank_map[map_index] = { data_src, (bank * map_size + i) };
	}
}

void Cartridge::setPRGBank(int slot, int bank, int bank_size) {
	assert(slot < 4 / (bank_size / MIN_PRG_BANK_SIZE), "Invalid prg slot");
	setBank(PRG, PRG, slot, bank, bank_size);
}

void Cartridge::setCHRBank(int slot, int bank, int bank_size) {
	assert(slot < 8 / (bank_size / MIN_CHR_BANK_SIZE), "Invalid chr slot");
	setBank(CHR, CHR, slot, bank, bank_size);
}

void Cartridge::saveState(std::ostream& out) {
	out.write((char*)prg_bank_map, sizeof(prg_bank_map));
	out.write((char*)chr_bank_map, sizeof(chr_bank_map));
	out.write((char*)ram_bank_map, sizeof(ram_bank_map));
	out.write((char*)ram, ram_size);
	if(has_chr_ram) {
		out.write((char*)chr, chr_size);
	}
}

void Cartridge::loadState(std::istream& in) {
	in.read((char*)prg_bank_map, sizeof(prg_bank_map));
	in.read((char*)chr_bank_map, sizeof(chr_bank_map));
	in.read((char*)ram_bank_map, sizeof(ram_bank_map));
	in.read((char*)ram, ram_size);
	if(has_chr_ram) {
		in.read((char*)chr, chr_size);
	}

	// set data map pointers
	for(int i = 0; i < 4; i++) {
		setBank(PRG, prg_bank_map[i].source, i, prg_bank_map[i].bank, MIN_PRG_BANK_SIZE);
	}
	for(int i = 0; i < 8; i++) {
		setBank(CHR, chr_bank_map[i].source, i, chr_bank_map[i].bank, MIN_CHR_BANK_SIZE);
	}
	setBank(RAM, ram_bank_map[0].source, 0, ram_bank_map[0].bank, MIN_RAM_BANK_SIZE);
}