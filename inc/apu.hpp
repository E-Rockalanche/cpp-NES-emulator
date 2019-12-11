#ifndef APU_HPP
#define APU_HPP

// blargg apu
#include "Nes_Apu.h"
#include "Sound_Queue.h"

#include "types.hpp"

namespace ByteIO
{
	class Writer;
	class Reader;
}

namespace nes
{

	class Apu
	{
	public:
		Apu();

		Byte read( cpu_time_t elapsedCycles, Word address );
		void write( cpu_time_t elapsedCycles, Word address, Byte value );
		void runFrame( cpu_time_t elapsedCycles );
		void reset();
		void setMute( bool mute );
		void setDmcReader( dmc_reader_t func );

		void saveState( ByteIO::Writer& writer ) const;
		void loadState( ByteIO::Reader& reader );

	private:
		static const size_t OutBufferSize = 4096;

	    Nes_Apu m_apu;
	    Blip_Buffer m_buffer;

	    Sound_Queue m_soundQueue;

	    blip_sample_t m_outBuf[ OutBufferSize ];

	    bool m_muted = false;
	};

}

#endif