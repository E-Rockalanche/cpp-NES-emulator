#include "cartridge.hpp"
#include <fstream>

using namespace std;

Cartridge* Cartridge::loadFile(const char* filename) {
	Cartridge* mapper = NULL;
	int data_size = 0;
	Byte* data;

	ifstream fin(filename, ios::binary);
	if (!fin.is_open() || !fin.good() || fin.eof()) {
		dout("Could not open file");
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
		delete[] data;
		return NULL;
	}

	if (!verifyHeader(data)) {
		dout("file verification failed");
		delete[] data;
		return NULL;
	}

	int mapper_number = getMapperNumber(data);
	switch(mapper_number) {
		case 0: mapper = new Cartridge(data); break;
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
	return  (data[FLAGS_6] >> 4)
		| ((getFormat(data) == NES2) ? (data[FLAGS_7] & 0xf0) : 0);
}

Cartridge::Format Cartridge::getFormat(Byte* data) {
	Byte result = data[FLAGS_7] & 0x0c;
	if (result == 0x08) {
		return NES2;
	} else if (result == 0) {
		return INES;
	} else {
		return ARCHAIC_INES;
	}
}

Cartridge::Cartridge(Byte* data) : data(data) {
	rom_size = data[ROM_SIZE] * 0x4000;
	chr_size = data[CHR_SIZE] * 0x2000;
	ram_size = data[RAM_SIZE] ? (data[RAM_SIZE] * 0x2000) : 0x2000;

	ram = new Byte[ram_size];
	rom = data + HEADER_SIZE;

	if (chr_size) {
		has_chr_ram = false;
		chr = rom + rom_size;
	} else {
		has_chr_ram = true;
		chr_size = 0x2000;
		chr = new Byte[chr_size];
	}
}

Cartridge::~Cartridge() {
	delete[] data;
	delete[] ram;
	if (has_chr_ram) delete[] chr;
}

Cartridge::NameTableMirroring Cartridge::nameTableMirroring() {
	return (data[FLAGS_6] & BL(0)) ? VERTICAL : HORIZONTAL;
}

Byte Cartridge::readROM(Word address) {
	assert(address >= 0x6000, "prg rom invalid address: " << toHex(address, 2));
	if (address >= 0x8000) {
		return rom[(address - 0x8000) % rom_size];
	} else {
		return ram[(address - 0x6000) % ram_size];
	}
}

Byte Cartridge::readCHR(Word address) {
	return chr[address % chr_size];
}