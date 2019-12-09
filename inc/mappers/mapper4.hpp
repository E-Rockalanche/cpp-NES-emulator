#ifndef MAPPER4_HPP
#define MAPPER4_HPP

#include "ppu.hpp"
#include "cartridge.hpp"

namespace nes
{

class Mapper4 : public Cartridge
{
public:
	Mapper4( Memory data ) : Cartridge( std::move( data ) )
	{
		reset();
	}

	void reset() override;

	void writePRG(Word address, Byte value) override;

	void signalScanline() override;

	void setCPU( Cpu& cpu ) override
	{
		m_cpu = &cpu;
	}

	void saveState( ByteIO::Writer& writer ) override;
	void loadState( ByteIO::Reader& reader ) override;

	const char* getName() const override { return "MMC3"; }

private:

	void applyBankSwitch();

private:

	enum Register
	{
		BANK_SELECT_EVEN = 0x8000,
		BANK_SELECT_ODD = 0x8001,

		MIRRORING = 0xa000,
		PRG_RAM_PROTECT = 0xa001,

		IRQ_LATCH = 0xc000,
		IRQ_RELOAD = 0xc001,

		IRQ_DISABLE = 0xe000,
		IRQ_ENABLE = 0xe001,

		NUM_REGISTERS = 8
	};

	nes::Cpu* m_cpu = nullptr;

	Byte m_bankRegisters[ NUM_REGISTERS ];
	
	Byte m_bankSelect = 0;
	Byte m_irqLatch = 0;
	Byte m_irqCounter = 0;

	bool m_irqEnabled = false;
	bool m_protectRAM = false;
	bool m_enableRAM = false;
};

}

#endif