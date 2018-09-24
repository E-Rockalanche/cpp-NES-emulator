#include "sram.hpp"
using namespace std;

SRAM::SRAM() {
	data = NULL;
	size = 0;
}

SRAM::~SRAM() {}

void SRAM::setData(Byte* data, int size) {
	this->data = data;
	this->size = size;
}

Byte SRAM::readByte(Word address) {
	assertBounds(address, size);
	return data[address];
}

void SRAM::writeByte(Word address, Byte value) {
	assertBounds(address, size);
	data[address] = value;
}