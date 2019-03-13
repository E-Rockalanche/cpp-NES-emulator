#ifndef MAPPER3_HPP
#define MAPPER3_HPP

#include "cartridge.hpp"

class Mapper3 : public Cartridge {
public:
	Mapper3(Byte* data);
	~Mapper3() {}

	void writePRG(Word address, Byte value);
	void writeCHR(Word address, Byte value);
};

#endif