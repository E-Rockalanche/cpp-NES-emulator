#include <fstream>

#include "cartridge.hpp"

#include "1_mmc1.hpp"
#include "2_uxrom.hpp"
#include "3_cnrom.hpp"
#include "4_mmc3.hpp"
#include "5_mmc5.hpp"
#include "7_axrom.hpp"

#include "assert.hpp"

using namespace std;

Cartridge* cartridge = NULL;

Cartridge* Cartridge::loadFile(std::string filename) {
	Cartridge* mapper = NULL;
	int data_size = 0;
	Byte* data = NULL;

	ifstream fin(filename, ios::binary);
	if (!fin.is_open() || !fin.good() || fin.eof()) {
		dout("Could not open " << filename);
		return NULL;
	}

	// check file size
	fin.seekg(0, ios::end);
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
		case 7: mapper = new AxROM(data); break;

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
	dout("~Cartridge()");
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

	ofstream fout(filename, ios::binary);
	if (!(fout.is_open() && fout.good())) {
		dout("Could not open " << filename);
		return false;
	}

	fout.write((const char*)ram, ram_size);
	fout.close();

	dout("saved to " << filename);
	return true;
}

bool Cartridge::loadSave(std::string filename) {
	if (!hasSRAM()) {
		dout("Cartridge does not have SRAM");
		return false;
	}

	ifstream fin(filename, ios::binary);
	if (!(fin.is_open() && fin.good())) {
		dout("Could not open " << filename);
		return false;
	}

	// check file size
	fin.seekg(0, ios::end);
	int file_size = fin.tellg();
	fin.seekg(0);

	if (file_size != ram_size) {
		dout("Save file is not 8KB");
		fin.close();
		return false;
	}

	fin.read((char*)ram, ram_size);
	fin.close();

	dout("loaded " << filename);
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

void Cartridge::setBank(Byte* map[], Byte* src, int slot, int bank, int bank_size) {
	int min_size, src_size;

	if (map == prg_map) min_size = MIN_PRG_BANK_SIZE;
	else if (map == chr_map) min_size = MIN_CHR_BANK_SIZE;
	else if (map == ram_map) min_size = 0x2000;
	else assert(false, "Invalid bank");

	if (src == prg) src_size = prg_size;
	else if (src == chr) src_size = chr_size;
	else if (src == ram) src_size = ram_size;
	else assert(false, "Invalid source");

	if (bank < 0) {
		bank = (src_size / bank_size) + bank;
	}
	int map_size = bank_size / min_size;
	for(int i = 0; i < map_size; i++) {
		map[(slot * map_size) + i] = src + (((bank * bank_size) + (i * min_size)) % src_size);
	}

	
	// DEBUG
	int size;
	if (map == prg_map) size = 4;
	else if (map == chr_map) size = 8;
	else size = 1;

	for(int i = slot*map_size; i < map_size*(slot+1); i++) {
		unsigned long offset = (unsigned long)map[i] - (unsigned long)src;
		assert(offset <= src_size - min_size, "slot " << i << " mapped out of bounds. offset: " << offset);
	}
	
}

void Cartridge::setPRGBank(int slot, int bank, int bank_size) {
	assert(slot < 4 / (bank_size / MIN_PRG_BANK_SIZE), "Invalid prg slot");
	setBank(prg_map, prg, slot, bank, bank_size);
}

void Cartridge::setCHRBank(int slot, int bank, int bank_size) {
	assert(slot < 8 / (bank_size / MIN_CHR_BANK_SIZE), "Invalid chr slot");
	setBank(chr_map, chr, slot, bank, bank_size);
}