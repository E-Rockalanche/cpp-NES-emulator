#include "rom_loader.hpp"

#include "Cartridge.hpp"

#include "Header.hpp"

#include "mappers/Mapper1.hpp"
#include "mappers/Mapper2.hpp"
#include "mappers/Mapper3.hpp"
#include "mappers/Mapper4.hpp"

#include "Memory.hpp"
#include "types.hpp"

#include "message.hpp" // TODO: remove

#include <fstream>

using namespace nes;
using namespace nes::Rom;

std::unique_ptr<Cartridge> nes::Rom::load( const char* filename )
{

	std::ifstream fin( filename, std::ios::binary );
	if ( !fin.is_open() || !fin.good() || fin.eof() )
	{
		showError( "Error", std::string( "Could not open " ) + filename );
		return nullptr;
	}

	// check file size
	fin.seekg( 0, std::ios::end );
	size_t dataSize = static_cast<size_t>( fin.tellg() );
	fin.seekg( 0 );

	if ( dataSize < ( Rom::HeaderSize + 8 * KB ) )
	{
		fin.close();
		showError( "Error", "File too small" );
		return nullptr;
	}

	Memory data( dataSize );

	// read file
	fin.read( ( char* )data.data(), data.size() );
	fin.close();

	if ( !Rom::isHeader( data.data() ) )
	{
		showError( "Error", "ROM header is invalid" );
		return nullptr;
	}

	if ( Rom::isNes2Format( data.data() ) )
	{
		showError( "Error", "NES 2.0 formatted ROMs are not supported yet" );
		return nullptr;
	}

	auto mapper_number = getMapperNumber( data.data() );
	switch ( mapper_number )
	{
		case 0: return std::make_unique<Cartridge>( std::move( data ) );
		case 1: return std::make_unique<Mapper1>( std::move( data ) );
		case 2: return std::make_unique<Mapper2>( std::move( data ) );
		case 3: return std::make_unique<Mapper3>( std::move( data ) );
		case 4: return std::make_unique<Mapper4>( std::move( data ) );

		default:
			showError( "Error", "Mapper " + std::to_string( mapper_number ) + " is not supported" );
			return nullptr;
	}
}