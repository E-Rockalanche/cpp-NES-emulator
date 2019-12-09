#ifndef MAPPER1_HPP
#define MAPPER1_HPP

#include "cartridge.hpp"

#include "ByteIO.hpp"

namespace nes
{

class Mapper1 : public Cartridge
{
public:
	Mapper1( Memory data );

	void reset() override;

	void writePRG( Word address, Byte value ) override;

	void saveState( ByteIO::Writer& writer ) override;
	void loadState( ByteIO::Reader& reader ) override;

	const char* getName() const override { return "MMC1"; }

private:
	enum Register
	{
		CONTROL,
		CHR_BANK_0,
		CHR_BANK_1,
		PRG_BANK,

		NUM_REGISTERS
	};

	static constexpr Byte SHIFT_REG_INIT = 0x10;

	Byte m_shiftRegister;
	Byte m_registers[ NUM_REGISTERS ];

	void applyBankSwitch();
	int getMirrorMode();
	int getPrgBankMode();
	bool getChrBankMode();
	bool isRamEnabled();
};

}

#endif