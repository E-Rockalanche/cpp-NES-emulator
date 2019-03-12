#ifndef MAPPER1_HPP
#define MAPPER1_HPP

#include "cartridge.hpp"

class Mapper1 : public Cartridge {
public:
	Mapper1(Byte* data);
	~Mapper1() {}

	void writePRG(Word address, Byte value);
	void writeCHR(Word address, Byte value);

protected:
	enum Register {
		CONTROL,
		CHR_BANK_0,
		CHR_BANK_1,
		PRG_BANK,

		NUM_REGISTERS
	};

	static const int SHIFT_REG_INIT = 0x10;

	int shift_register;
	int registers[NUM_REGISTERS];

	void applyBankSwitch();
	int mirrorMode();
	int prgBankMode();
	bool chrBankMode();
	bool ramEnable();
};

#endif