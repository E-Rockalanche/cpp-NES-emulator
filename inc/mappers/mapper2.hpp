#ifndef MAPPER2_HPP
#define MAPPER2_HPP

#include "cartridge.hpp"

class Mapper2 : public Cartridge {
public:
	Mapper2(Byte* data);
	~Mapper2() {}

	void writePRG(Word address, Byte value);
	void writeCHR(Word address, Byte value);
};

#endif