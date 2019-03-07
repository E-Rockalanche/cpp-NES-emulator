#include <fstream>
#include "cartridge.hpp"
#include "debugging.hpp"
#include "common.hpp"

using namespace std;

#define inRange(value, low, high) ((value) >= (low) && (value) < (high))

Cartridge::Cartridge() {
	data = NULL;
	mapper = NULL;
	filename = "";
}

Cartridge::~Cartridge() {
	freeData();
}

bool Cartridge::loadFile(const string filename) {
	freeData();

	this->filename = filename;
	ifstream fin(filename.c_str(), ios::binary);
	if (!fin.is_open() || !fin.good() || fin.eof()) {
		dout("Could not open file");
		return false;
	}

	// copy file data
	fin.seekg(0, ios::end);
	allocateMemory(fin.tellg());
	fin.seekg(0);
	fin.read((char*)data, data_size);
	fin.close();

	if (data == NULL) {
		dout("Could not allocate memory");
		return false;
	}

	if (data_size < (HEADER_SIZE + 8*KB)) {
		dout("File size is too small to be a ROM");
		freeData();
		return false;
	}

	if (!verify()) {
		dout("file verification failed");
		freeData();
		return false;
	}

	switch(mapperNumer()) {
		case 0: mapper = new Mapper0(data); break;
		default: assert(false, "mapper " << mapperNumber() << " is not supported");
	}

	/*
	prg_size = prgRomSize();
	chr_size = chrRomSize();

	dout("PRG size = " << toHex(prg_size));
	dout("CHR size = " << toHex(chr_size));

	prg_rom = data + prgRomStart();
	chr_rom = data + chrRomStart();
	*/

	return true;
}

bool Cartridge::loaded() {
	return data != NULL && mapper != NULL;
}

void Cartridge::allocateMemory(int size) {
	freeData();
	data = new Byte[size];
	data_size = size;
}

void Cartridge::freeData() {
	if (data != NULL) {
		delete[] data;
		data = NULL;
	}
	data_size = 0;

	if (mapper != NULL) {
		delete mapper;
		mapper = NULL;
	}
}





int Cartridge::expectedSize() {
	return HEADER_SIZE + prgRomSize() + chrRomSize();
}

Cartridge::NameTableMirroring Cartridge::nameTableMirroring() {
	return (data[FLAGS_6] & BL(0)) ? VERTICAL : HORIZONTAL;
}

bool Cartridge::hasPrgRam() {
	return data[FLAGS_6] & BL(1);
}

int Cartridge::mapperNumber() {
	return  (data[FLAGS_6] >> 4)
		| ((getFormat() == NES2) ? (data[FLAGS_7] & 0xf0) : 0);
}

Cartridge::Format Cartridge::getFormat() {
	Byte result = data[FLAGS_7] & 0x0c;
	if (result == 0x08) {
		return NES2;
	} else if (result == 0) {
		return INES;
	} else {
		return ARCHAIC_INES;
	}
}

int Cartridge::prgRomStart() {
	return HEADER_SIZE;
}

int Cartridge::chrRomStart() {
	return prgRomStart() + prgRomSize();
}

Byte Cartridge::readROM(Word address) {
	assert(address >= 0x6000, "prg rom invalid address: " << toHex(address, 2));
	if (address >= 0x8000) {
		return prg_rom[(address - 0x8000) % prg_size];
	} else {
		return prg_rom[address - 0x6000];
	}
}

void Cartridge::writeROM(Word address, Byte value) {
	if (address >= 0x4018) dout("writing " << (int)value << " to ROM at "
		<< toHex(address, 2));
}

Byte Cartridge::readCHR(Word address) {
	return chr_rom[address % chr_size];
}

void Cartridge::writeCHR(Word address, Byte value) {
	dout("writing " << (int)value << " to CHR at " << toHex(address, 2));
}