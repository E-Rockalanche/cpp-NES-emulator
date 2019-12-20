#ifndef NES_HPP
#define NES_HPP

#include "apu.hpp"
#include "Cartridge.hpp"
#include "Cpu.hpp"
#include "debug.hpp"
#include "Ppu.hpp"

#include <iostream>

class Pixel;

namespace nes
{
	
	class Nes
	{
	public:

		Nes()
		{
			cpu.setAPU( apu );
			cpu.setPPU( ppu );
			ppu.setCPU( cpu );
			apu.setDmcReader( [this]( void*, cpu_addr_t address ) -> int { return cpu.read( address ); } );
		}

		void power()
		{
			ppu.power();
			apu.reset();
			if ( cartridge )
				cartridge->reset();

			cpu.power();
		}

		void reset()
		{
			ppu.reset();
			apu.reset();
			if ( cartridge )
				cartridge->reset();

			cpu.reset();
		}

		void setCartridge( std::unique_ptr<Cartridge> cartridge_ )
		{
			cartridge = std::move( cartridge_ );

			cpu.setCartridge( cartridge.get() );
			ppu.setCartridge( cartridge.get() );
			
			if ( cartridge )
				cartridge->setCPU( cpu );
		}

		void setController( size_t port, Controller* controller )
		{
			cpu.setController( port, controller );
		}

		const Pixel* getPixelBuffer() const
		{
			return ppu.getPixelBuffer();
		}

		void runFrame()
		{
			cpu.runFrame();
		}

		bool halted() const
		{
			return cpu.halted();
		}

		bool getSpriteFlickering() const
		{
			return ppu.getSpriteFlickering();
		}

		void setSpriteFlickering( bool on )
		{
			ppu.setSpriteFlickering( on );
		}

		void setMute( bool mute )
		{
			apu.setMute( mute );
		}

		void saveState( std::ostream& out )
		{
			dbAssert( cartridge );

			ByteIO::Writer writer( out );

			cpu.saveState( out );
			ppu.saveState( out );
			apu.saveState( writer );
			cartridge->saveState( writer );
		}

		void loadState( std::istream& in )
		{
			dbAssert( cartridge );

			ByteIO::Reader reader( in );

			cpu.loadState( in );
			ppu.loadState( in );
			apu.loadState( reader );
			cartridge->loadState( reader );
		}

		void dump()
		{
			cpu.dumpState();
			// cpu.dumpStack();
			cpu.dump( cpu.getProgramCounter() );
		}

		Cartridge* getCartridge() { return cartridge.get(); }
		const Cartridge* getCartridge() const { return cartridge.get(); }

		Cpu cpu;
		Ppu ppu;
		Apu apu;
		std::unique_ptr<Cartridge> cartridge;
	};

}

#endif