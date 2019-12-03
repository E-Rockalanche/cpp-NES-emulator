#ifndef APU_HPP
#define APU_HPP

#include "common.hpp"
#include "types.hpp"

#include "Nes_Apu.h"

#include <iostream>

namespace nes::APU
{

	Byte readByte( int elapsed_cycles, Word address );
	void writeByte( int elapsed_cycles, Word address, Byte value );
	void runFrame( int elapsed_cycles );
	void reset();
	void init();
	void setMute( bool muted );

	void setDmcReader( dmc_reader_t reader );

	void saveState( std::ostream& out );
	void loadState( std::istream& in );
	
}

#endif