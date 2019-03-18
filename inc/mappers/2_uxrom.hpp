#ifndef MAPPER2_HPP
#define MAPPER2_HPP

#include "cartridge.hpp"

class UxROM : public Cartridge {
public:
	UxROM(Byte* data);
	~UxROM();

	void writePRG(Word address, Byte value);
	void writeCHR(Word address, Byte value);
};

#endif