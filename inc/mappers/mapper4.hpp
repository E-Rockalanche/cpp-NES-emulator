#ifndef MAPPER4_HPP
#define MAPPER4_HPP

#include "ppu.hpp"
#include "cartridge.hpp"

class Mapper4 : public Cartridge {
public:
	Mapper4(Byte* data);
	~Mapper4() {}

	void writePRG(Word address, Byte value);
	void writeCHR(Word address, Byte value);
	void signalScanline();

protected:
	Byte bank_select;
	Byte bank_registers[8];
	bool protectRAM;
	bool enableRAM;
	Byte irq_latch;
	Byte irq_counter;
	bool irq_enabled;

	void applyBankSwitch();
};

#endif