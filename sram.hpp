#ifndef SRAM_HPP
#define SRAM_HPP

#include "common.hpp"

class SRAM {
public:
	SRAM();
	~SRAM();
	void setData(Byte* data, int size);
	Byte readByte(Word address);
	void writeByte(Word address, Byte value);

private:
	Byte* data;
	int size;
};

#endif