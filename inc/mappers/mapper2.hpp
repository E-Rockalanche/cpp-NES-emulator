#ifndef MAPPER2_HPP
#define MAPPER2_HPP

#include "cartridge.hpp"

namespace nes
{

class Mapper2 : public Cartridge
{
public:
	Mapper2(Byte* data);
	~Mapper2() {}
	void reset();

	void writePRG(Word address, Byte value);
	void writeCHR(Word address, Byte value);
};

}

#endif