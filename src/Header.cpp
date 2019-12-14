

#include "Header.hpp"

#include <algorithm>

namespace nes::Rom
{

bool isHeader( const Byte* header )
{
	static const char* s_nesStr = "NES\x1a";

	for( size_t i = 0; i < 4; ++i )
	{
		if ( header[ i ] != s_nesStr[ i ] )
			return false;
	}

	return true;
}

bool isNes2Format( const Byte* header )
{
	return ( header[ 7 ] & 0x0c ) == 0x08;
}

bool mirrorNameTableVertical( const Byte* header )
{
	return header[ 6 ] & 0x01;
}

bool hasSaveRam( const Byte* header )
{
	return header[ 6 ] & 0x02;
}

int getMapperNumber( const Byte* header )
{
	int low = header[ 6 ] >> 4;
	int high = isNes2Format( header )
		? ( header[ 7 ] & 0xf0 )
		: 0;
	return high | low;
}

size_t getPrgSize( const Byte* header )
{
	return header[ 4 ] * PrgSizeUnit;
}

size_t getChrSize( const Byte* header )
{
	return header[ 5 ] * ChrSizeUnit;
}

size_t getRamSize( const Byte* header )
{
	if ( isNes2Format( header ) )
		return RamSizeUnit; // TODO
	else
		return std::max<Byte>( header[ 8 ], 1 ) * RamSizeUnit;
}

}