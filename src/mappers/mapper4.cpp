
#include "mappers/mapper4.hpp"

#include "Cpu.hpp"
#include <stdx/assert.h>
#include "main.hpp" // temp

#include <cstring>

using namespace nes;

void Mapper4::reset()
{
	Cartridge::reset();

	m_irqEnabled = false;
	m_irqLatch = 0;
	m_irqCounter = 0;
	m_bankSelect = 0;
	setNameTableMirroring( NameTableMirroring::Horizontal );

	for( auto& reg : m_bankRegisters )
		reg = 0;

	setPrgBank( 3, -1, 8 * KB );

	applyBankSwitch();
}

void Mapper4::writePRG( Word address, Byte value )
{
	if ( address >= PrgStart )
	{
		switch ( address & 0xe001 )
		{
			case BANK_SELECT_EVEN:
				m_bankSelect = value;
				break;

			case BANK_SELECT_ODD:
				m_bankRegisters[m_bankSelect & 0x07] = value;
				applyBankSwitch();
				break;

			case MIRRORING:
				setNameTableMirroring( ( value & 1 )
					? NameTableMirroring::Horizontal
					: NameTableMirroring::Vertical );
				break;

			case PRG_RAM_PROTECT:
				// TODO?
				break;

			case IRQ_LATCH:
				m_irqLatch = value;
				break;

			case IRQ_RELOAD:
				m_irqCounter = 0;
				break;

			case IRQ_DISABLE:
				m_irqEnabled = false;
				m_cpu->setIRQ( false );
				break;

			case IRQ_ENABLE:
				m_irqEnabled = true;
				break;
		}
	}
	else if ( address >= RamStart )
	{
		getRam()[ address - RamStart ] = value;
	}
}

void Mapper4::applyBankSwitch()
{
	setPrgBank( 1, m_bankRegisters[7], 8 * KB );

	if ( m_bankSelect & 0x40 )
	{
		setPrgBank( 0, -2, 8 * KB );
		setPrgBank( 2, m_bankRegisters[6], 8 * KB );
	}
	else
	{
		setPrgBank( 0, m_bankRegisters[6], 8 * KB );
		setPrgBank( 2, -2, 8 * KB );
	}

	if ( m_bankSelect & 0x80 )
	{
		for ( int i = 0; i < 4; i++ )
		{
			setChrBank( i, m_bankRegisters[2 + i], KB );
		}
		setChrBank( 2, m_bankRegisters[0] / 2, 2 * KB );
		setChrBank( 3, m_bankRegisters[1] / 2, 2 * KB );
	}
	else
	{
		setChrBank( 0, m_bankRegisters[0] / 2, 2 * KB );
		setChrBank( 1, m_bankRegisters[1] / 2, 2 * KB );
		for ( int i = 0; i < 4; i++ )
		{
			setChrBank( i + 4, m_bankRegisters[2 + i], KB );
		}
	}
}

void Mapper4::signalScanline()
{
	if ( m_irqCounter == 0 )
	{
		m_irqCounter = m_irqLatch;
	}
	else
	{
		m_irqCounter--;
	}

	if ( m_irqEnabled && ( m_irqCounter == 0 ) )
	{
		m_cpu->setIRQ();
	}
}

void Mapper4::saveState( ByteIO::Writer& writer  )
{
	Cartridge::saveState( writer );

	writer.write( m_bankRegisters );
	writer.write( m_bankSelect );
	writer.write( m_irqLatch );
	writer.write( m_irqCounter );
	writer.write( m_irqEnabled );
	writer.write( m_protectRAM );
	writer.write( m_enableRAM );
}

void Mapper4::loadState( ByteIO::Reader& reader )
{
	Cartridge::loadState( reader );

	reader.read( m_bankRegisters );
	reader.read( m_bankSelect );
	reader.read( m_irqLatch );
	reader.read( m_irqCounter );
	reader.read( m_irqEnabled );
	reader.read( m_protectRAM );
	reader.read( m_enableRAM );
}