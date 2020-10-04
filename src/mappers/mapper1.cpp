#include "mappers/mapper1.hpp"

#include <stdx/assert.h>

using namespace nes;

Mapper1::Mapper1( Memory data ) : Cartridge( std::move( data ) )
{
	reset();
}

void Mapper1::reset()
{
	Cartridge::reset();

	m_shiftRegister = SHIFT_REG_INIT;

	setPrgBank( 0, 0, 0x4000 );
	setPrgBank( 1, -1, 0x4000 );
}

void Mapper1::writePRG( Word address, Byte value )
{
	if ( address >= PrgStart )
	{
		if ( value & 0x80 )
		{
			m_shiftRegister = SHIFT_REG_INIT;
			m_registers[CONTROL] |= 0x0c;
			applyBankSwitch();
		}
		else
		{
			bool last_write = m_shiftRegister & 1;
			m_shiftRegister >>= 1;
			m_shiftRegister |= ( value & 1 ) << 4;
			if ( last_write )
			{
				m_registers[( address >> 13 ) & 0x03] = m_shiftRegister;
				m_shiftRegister = SHIFT_REG_INIT;
				applyBankSwitch();
			}
		}
	}
	else if ( address >= RamStart )
	{
		getRam()[ address - RamStart ] = value;
	}
}

int Mapper1::getMirrorMode()
{
	return m_registers[ CONTROL ] & 0x03;
}

int Mapper1::getPrgBankMode()
{
	return ( m_registers[ CONTROL ] >> 2 ) & 0x03;
}

bool Mapper1::getChrBankMode()
{
	return m_registers[ CONTROL ] & 0x10;
}

bool Mapper1::isRamEnabled()
{
	return !( m_registers[ PRG_BANK ] & 0x10 );
}

void Mapper1::applyBankSwitch()
{
	// switch prg rom
	switch ( getPrgBankMode() )
	{
		case 0:
		case 1:
		{
			int bank = ( m_registers[ PRG_BANK ] & 0xf ) >> 1;
			setPrgBank( 0, bank, 32 * KB );
			break;
		}

		case 2:
		{
			// 0x8000 fixed to bank 0x00
			setPrgBank( 0, 0, 16 * KB );
			// 0xc000 swappable
			int bank = m_registers[ PRG_BANK ] & 0x0f;
			setPrgBank( 1, bank, 16 * KB );
			break;
		}

		case 3:
			// 0x8000 swappable
			int bank = m_registers[PRG_BANK] & 0x0f;
			setPrgBank( 0, bank, 16 * KB );
			// 0xc000 fixed to last bank
			setPrgBank( 1, -1, 16 * KB );
			break;
	}

	// switch chr rom
	if ( getChrBankMode() )
	{
		setChrBank( 0, m_registers[ CHR_BANK_0 ], 4 * KB );
		setChrBank( 1, m_registers[ CHR_BANK_1 ], 4 * KB );
	}
	else
	{
		setChrBank( 0, m_registers[CHR_BANK_0] >> 1, 8 * KB );
	}

	switch ( getMirrorMode() )
	{
		case 2:
			setNameTableMirroring( NameTableMirroring::Vertical );
			break;

		case 3:
			setNameTableMirroring( NameTableMirroring::Horizontal );
			break;
	}
}

void Mapper1::saveState(  ByteIO::Writer& writer )
{
	Cartridge::saveState( writer );

	writer.write( m_shiftRegister );
	writer.write( m_registers );
}

void Mapper1::loadState( ByteIO::Reader& reader )
{
	Cartridge::loadState( reader );

	reader.read( m_shiftRegister );
	reader.read( m_registers );
}