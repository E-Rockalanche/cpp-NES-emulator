#ifndef APU_HPP
#define APU_HPP

#include <iostream>

#include "Nes_Apu.h"

#include "common.hpp"

namespace APU {
	Byte readByte(int elapsed_cycles, Word address);
	void writeByte(int elapsed_cycles, Word address, Byte value);
	void runFrame(int elapsed_cycles);
	void reset();
	void init();
	void mute(bool muted);

	void saveState(std::ostream& out);
	void loadState(std::istream& in);
}

#endif