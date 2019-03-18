#ifndef MAPPER3_HPP
#define MAPPER3_HPP

#include "cartridge.hpp"

class CNROM : public Cartridge {
public:
	CNROM(Byte* data);
	~CNROM();

	void writePRG(Word address, Byte value);
	void writeCHR(Word address, Byte value);
};

#endif