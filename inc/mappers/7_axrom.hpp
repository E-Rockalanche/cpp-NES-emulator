#ifndef MAPPER7_HPP
#define MAPPER7_HPP

#include "cartridge.hpp"

class AxROM : public Cartridge {
public:
	AxROM(Byte* data);
	~AxROM() {}

	void writePRG(Word address, Byte value);
	void writeCHR(Word address, Byte value);
};

#endif