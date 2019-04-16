#ifndef MAPPER4_HPP
#define MAPPER4_HPP

#include "ppu.hpp"
#include "cartridge.hpp"

class MMC3 : public Cartridge {
public:
	MMC3(Byte* data);
	~MMC3() {}

	void writePRG(Word address, Byte value);
	void signalScanlineMMC3();

	void saveState(std::ostream& out);
	void loadState(std::istream& in);

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