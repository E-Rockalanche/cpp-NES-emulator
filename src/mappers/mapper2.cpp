#include "mapper2.hpp"
#include "debug.hpp"

using namespace nes;

Mapper2::Mapper2( Memory data ) : Cartridge( std::move( data ) )
{
	reset();
}

void Mapper2::reset()
{
	Cartridge::reset();
	
	setPrgBank( 0, 0, 16 * KB ); // switchable
	setPrgBank( 1, -1, 16 * KB ); // fixed

	setChrBank( 0, 0, 8 * KB );
}

void Mapper2::writePRG(Word address, Byte value)
{
	if ( address & 0x8000 )
		setPrgBank( 0, value & 0x0f, 16 * KB );
}