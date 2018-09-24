#ifndef NES_HPP
#define NES_HPP

#include "cpu.hpp"
#include "ppu.hpp"
#include "apu.hpp"
#include "cartridge.hpp"

class NES {
public:
	NES();
	bool loadFile(const char* filename);
	bool reset();
	bool execute();

	CPU* getCPU() { return &cpu; }

private:
	bool good;

	Cartridge cartridge;
	CPU cpu;
	PPU ppu;
	APU apu;
};

#endif