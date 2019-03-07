#ifndef MAPPER0_HPP
#define MAPPER0_HPP

#include "../cartridge.hpp"

class Mapper0 : public Cartridge {
public:
	Mapper0(Byte* data) : Cartridge(data) {
	}
}

#endif