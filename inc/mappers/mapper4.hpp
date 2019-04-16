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

	void saveState(std::ostream& out);
	void loadState(std::istream& in);

protected:
	enum {
		BANK_SELECT_EVEN = 0x8000,
		BANK_SELECT_ODD = 0x8001,

		MIRRORING = 0xa000,
		PRG_RAM_PROTECT = 0xa001,

		IRQ_LATCH = 0xc000,
		IRQ_RELOAD = 0xc001,

		IRQ_DISABLE = 0xe000,
		IRQ_ENABLE = 0xe001
	};
	
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