
#include "mapper4.hpp"

#include "Cpu.hpp"
#include "debug.hpp"
#include "main.hpp" // temp

#include <cstring>

using namespace nes;

Mapper4::Mapper4( Byte* data ) : Cartridge( data )
{
	reset();
}

void Mapper4::reset()
{
	Cartridge::reset();

	irq_enabled = false;
	irq_latch = 0;
	irq_counter = 0;
	bank_select = 0;
	nt_mirroring = NameTableMirroring::Horizontal;

	for ( int i = 0; i < 8; i++ )
	{
		bank_registers[i] = 0;
	}
	setPRGBank( 3, -1, 8 * KB );
	applyBankSwitch();
}

void Mapper4::writePRG( Word address, Byte value )
{
	if ( address >= 0x8000 )
	{
		switch ( address & 0xe001 )
		{
			case BANK_SELECT_EVEN:
				bank_select = value;
				break;
			case BANK_SELECT_ODD:
				bank_registers[bank_select & 0x07] = value;
				applyBankSwitch();
				break;

			case MIRRORING:
				nt_mirroring = ( value & 1 ) ? NameTableMirroring::Horizontal : NameTableMirroring::Vertical;
				break;
			case PRG_RAM_PROTECT:
				break;

			case IRQ_LATCH:
				irq_latch = value;
				break;
			case IRQ_RELOAD:
				irq_counter = 0;
				break;

			case IRQ_DISABLE:
				irq_enabled = false;
				m_cpu->setIRQ( false );
				break;
			case IRQ_ENABLE:
				irq_enabled = true;
				break;

			default:
				dbAssertMessage( false, "invalid mapper 4 register" );
		}
	}
	else if ( address >= 0x6000 )
	{
		ram[address - 0x6000] = value;
	}
}

void Mapper4::writeCHR( Word address, Byte value )
{
	dbAssertMessage( address < chr_size, "chr address out of bounds" );
	chr[address] = value;
}

void Mapper4::applyBankSwitch()
{
	setPRGBank( 1, bank_registers[7], 8 * KB );

	if ( bank_select & 0x40 )
	{
		setPRGBank( 0, -2, 8 * KB );
		setPRGBank( 2, bank_registers[6], 8 * KB );
	}
	else
	{
		setPRGBank( 0, bank_registers[6], 8 * KB );
		setPRGBank( 2, -2, 8 * KB );
	}

	if ( bank_select & 0x80 )
	{
		for ( int i = 0; i < 4; i++ )
		{
			setCHRBank( i, bank_registers[2 + i], KB );
		}
		setCHRBank( 2, bank_registers[0] / 2, 2 * KB );
		setCHRBank( 3, bank_registers[1] / 2, 2 * KB );
	}
	else
	{
		setCHRBank( 0, bank_registers[0] / 2, 2 * KB );
		setCHRBank( 1, bank_registers[1] / 2, 2 * KB );
		for ( int i = 0; i < 4; i++ )
		{
			setCHRBank( i + 4, bank_registers[2 + i], KB );
		}
	}
}

void Mapper4::signalScanline()
{
	if ( irq_counter == 0 )
	{
		irq_counter = irq_latch;
	}
	else
	{
		irq_counter--;
	}

	if ( irq_enabled && ( irq_counter == 0 ) )
	{
		m_cpu->setIRQ();
	}
}

struct SaveState
{
	Byte bank_select;
	bool protectRAM;
	bool enableRAM;
	Byte irq_latch;
	Byte irq_counter;
	bool irq_enabled;
	Byte bank_registers[8];
};

void Mapper4::saveState( std::ostream& out )
{
	SaveState ss;
	ss.bank_select = bank_select;
	ss.protectRAM = protectRAM;
	ss.enableRAM = enableRAM;
	ss.irq_latch = irq_latch;
	ss.irq_counter = irq_counter;
	std::memcpy( ss.bank_registers, bank_registers, sizeof( bank_registers ) );

	Cartridge::saveState( out );
	out.write( ( char* )&ss, sizeof( SaveState ) );
}

void Mapper4::loadState( std::istream& in )
{
	SaveState ss;

	Cartridge::loadState( in );
	in.read( ( char* )&ss, sizeof( SaveState ) );

	bank_select = ss.bank_select;
	protectRAM = ss.protectRAM;
	enableRAM = ss.enableRAM;
	irq_latch = ss.irq_latch;
	irq_counter = ss.irq_counter;
	std::memcpy( bank_registers, ss.bank_registers, sizeof( bank_registers ) );
}