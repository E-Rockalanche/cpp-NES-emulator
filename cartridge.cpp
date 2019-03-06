#include <fstream>
#include "cartridge.hpp"
#include "debugging.hpp"
#include "common.hpp"

using namespace std;

#define inRange(value, low, high) ((value) >= (low) && (value) < (high))

Cartridge::Cartridge() {
	dout("Cartridge()");
	data = NULL;
	filename = "";
}

Cartridge::~Cartridge() {
	dout("~Cartridge()");
	freeData();
}

bool Cartridge::loadFile(const string filename) {
	this->filename = filename;
	ifstream fin(filename.c_str(), ios::binary);
	if (!fin.is_open() || !fin.good() || fin.eof()) {
		freeData();
		return false;
	}

	fin.seekg(0, ios::end);
	allocateMemory(fin.tellg());
	fin.seekg(0);

	fin.read((char*)data, data_size);

	if (!fin.good() || fin.fail() || !verify()) {
		freeData();
		dout("failed to load file");
		fin.close();
		return false;
	}

	prg_size = prgRomSize();
	chr_size = chrRomSize();

	dout("file size = " << toHex(data_size));
	dout("PRG size = " << toHex(prg_size));
	dout("CHR size = " << toHex(chr_size));
	dout("PRG + CHR = " << toHex(prg_size + chr_size));

	prg_rom = data + prgRomStart();
	chr_rom = data + chrRomStart();

	fin.close();
	return true;
}

bool Cartridge::loaded() {
	return data != NULL;
}

void Cartridge::allocateMemory(int size) {
	dout("Cartridge::allocateMemory(" << size << ")");
	freeData();
	data = new Byte[size];
	data_size = size;
}

void Cartridge::freeData() {
	dout("Cartridge::freeData()");
	if (data != NULL) {
		delete[] data;
		data = NULL;
	}
	data_size = 0;
}

bool Cartridge::verify() {
	return ((data != NULL)
		&& (data_size > 16)
		&& (data[N] == 'N')
		&& (data[E] == 'E')
		&& (data[S] == 'S')
		&& (data[MSDOS_EOF] == 0x1a)
		&& (data[11] == 0)
		&& (data[12] == 0)
		&& (data[13] == 0)
		&& (data[14] == 0)
		&& (data[15] == 0)
		&& (data_size >= expectedSize())
		&& (data_size <= (expectedSize() + 128)));
}

int Cartridge::prgRomSize() {
	return data[PRG_ROM_SIZE] * 16 * KB;
}

int Cartridge::chrRomSize() {
	return data[CHR_ROM_SIZE] * 8 * KB;
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

int Cartridge::prgRamSize() {
	return data[PRG_RAM_SIZE] * 8 * KB;
}

int Cartridge::prgRomStart() {
	return HEADER_SIZE;
}

int Cartridge::chrRomStart() {
	return prgRomStart() + prgRomSize();
}

Byte Cartridge::readROM(Word address) {
	return prg_rom[address % prg_size];
}

Byte Cartridge::readCHR(Word address) {
	return chr_rom[address % chr_size];
}

void Cartridge::writeROM(Word address, Byte value) {
	prg_rom[address % prg_size] = value;
}

void Cartridge::writeCHR(Word address, Byte value) {
	chr_rom[address % chr_size] = value;
}