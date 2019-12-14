#include "Cartridge.hpp"

#include "crc32.hpp"
#include "debug.hpp"
#include "Header.hpp"

#include <fstream>

using namespace nes;

Cartridge::Cartridge( Memory data )
	: m_data( std::move( data ) )
{
	m_prgSize = Rom::getPrgSize( m_data.data() );
	m_chrSize = Rom::getChrSize( m_data.data() );

	dbAssert( Rom::HeaderSize + m_prgSize + m_chrSize <= m_data.size() );

	dbLogHex( "PRG size", m_prgSize );

	m_prg = m_data.data() + Rom::HeaderSize;

	if ( m_chrSize == 0 )
	{
		// use CHR RAM
		m_chrRam = Memory( Rom::ChrSizeUnit );
		m_chr = m_chrRam.data();
		m_chrSize = m_chrRam.size();

		dbLogHex( "CHR RAM size", m_chrSize );
	}
	else
	{
		m_chr = m_prg + m_prgSize;
		dbLogHex( "CHR size", m_chrSize );
	}

	size_t ramSize = Rom::getRamSize( m_data.data() );
	m_ram = Memory( ramSize );

	dbLogHex( "RAM size", ramSize );

	m_checksum = crc32( m_data.data(), m_data.size() );

	dbLog( "checksum: %u", m_checksum );

	m_prgMap.setMemorySize( m_prgSize );
	m_chrMap.setMemorySize( m_chrSize );

	Cartridge::reset();
}

void Cartridge::reset()
{
	m_mirroring = Rom::mirrorNameTableVertical( m_data.data() )
		? NameTableMirroring::Vertical
		: NameTableMirroring::Horizontal;

	m_prgMap.reset();
	m_chrMap.reset();
}


Byte Cartridge::readPRG( Word address )
{
	if ( address >= PrgStart )
	{
		// 0x8000 ... 0xffff
		return m_prg[ m_prgMap[ address - PrgStart ] ];
	}
	else if ( address >= RamStart )
	{
		// 0x6000 .. 0x7fff
		return m_ram[ address - RamStart ];
	}
	else
	{
		// 0x4020 ... 0x5fff
		dbAssert( address >= CartridgeStart );
		return address >> 8;
	}
}

Byte Cartridge::readCHR( Word address )
{
	return m_chr[ m_chrMap[ address ] ];
}

void Cartridge::writePRG( Word address, Byte value )
{
	if ( address >= RamStart  && address < PrgStart )
	{
		m_ram[ address - RamStart ] = value;
	}
}

void Cartridge::writeCHR( Word address, Byte value )
{
	if ( m_chrRam.size() > 0 )
	{
		dbAssert( address < getChrSize() );
		m_chr[ address ] = value;
	}
}

bool Cartridge::hasSRAM() const
{
	return Rom::hasSaveRam( m_data.data() );
}

bool Cartridge::saveGame( const char* filename )
{
	if ( !hasSRAM() )
		return false;

	std::ofstream fout( filename, std::ios::binary );
	if ( !fout.is_open() )
		return false;

	fout.write( (const char*)m_ram.data(), m_ram.size() );
	fout.close();
	return true;
}

bool Cartridge::loadSave( const char* filename )
{
	if ( !hasSRAM() )
		return false;

	std::ifstream fin( filename, std::ios::binary );
	if ( !fin.is_open() )
		return false;

	fin.seekg( 0, std::ios::end );
	auto fileSize = fin.tellg();
	fin.seekg( 0 );

	if ( fileSize != m_ram.size() )
	{
		fin.close();
		return false;
	}

	fin.read( (char*)m_ram.data(), m_ram.size() );
	fin.close();
	return true;
}

void Cartridge::saveState( ByteIO::Writer& writer )
{
	writer.write( getName() );

	writer.write( m_ram.data(), m_ram.size() );
	writer.write( m_chrRam.data(), m_chrRam.size() );
}

void Cartridge::loadState( ByteIO::Reader& reader )
{
	reader.readHeader( getName() );

	reader.read( m_ram.data(), m_ram.size() );
	reader.read( m_chrRam.data(), m_chrRam.size() );
}