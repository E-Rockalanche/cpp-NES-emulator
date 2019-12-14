#ifndef ROM_FILE_HEADER_HPP
#define ROM_FILE_HEADER_HPP

#include "types.hpp"

namespace nes::Rom
{

	constexpr size_t HeaderSize = 16;
	constexpr size_t PrgSizeUnit = 0x4000;
	constexpr size_t ChrSizeUnit = 0x2000;
	constexpr size_t RamSizeUnit = 0x2000;

	bool isHeader( const Byte* header );
	bool isNes2Format( const Byte* header );
	bool mirrorNameTableVertical( const Byte* header );
	bool hasSaveRam( const Byte* header );
	size_t getPrgSize( const Byte* header );
	size_t getChrSize( const Byte* header );
	size_t getRamSize( const Byte* header );
	int getMapperNumber( const Byte* header );

}

#endif