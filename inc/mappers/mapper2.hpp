#ifndef MAPPER2_HPP
#define MAPPER2_HPP

#include "cartridge.hpp"

namespace nes
{

class Mapper2 : public Cartridge
{
public:
	Mapper2( Memory data );
	void reset() override;

	void writePRG( Word address, Byte value ) override;

	const char* getName() const override { return "UxROM"; }
};

}

#endif