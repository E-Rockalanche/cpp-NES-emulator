#include "mapper3.hpp"
#include "debug.hpp"

using namespace nes;

void Mapper3::writePRG( Word address, Byte value )
{
	if ( address & 0x8000 )
	{
		// ANDing with rom emulates bus conflict
		int bank = value & 0x03 & readPRG( address );
		setChrBank( 0, bank, 8 * KB );
	}
}