#ifndef MAPPER3_HPP
#define MAPPER3_HPP

#include "cartridge.hpp"

namespace nes
{

class Mapper3 : public Cartridge
{
public:
	using Cartridge::Cartridge;

	void writePRG( Word address, Byte value ) override;

	const char* getName() const override { return "CNROM"; }
};

}

#endif