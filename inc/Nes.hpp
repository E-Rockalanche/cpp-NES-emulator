#ifndef NES_HPP
#define NES_HPP

#include "apu.hpp"
#include "Cartridge.hpp"
#include "Cpu.hpp"
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
			cpu.setPPU( ppu );
			ppu.setCPU( cpu );
			APU::init();
			APU::setDmcReader( [this]( void*, cpu_addr_t address ){ return (int)cpu.read( address ); } );
		}

		void power()
		{
			cpu.power();
			ppu.power();
			APU::reset();
		}

		void reset()
		{
			cpu.reset();
			ppu.reset();
			APU::reset();
		}

		void setCartridge( Cartridge* cartridge_ )
		{
			cartridge = cartridge_;

			cpu.setCartridge( cartridge );
			ppu.setCartridge( cartridge );
			
			if ( cartridge )
				cartridge->setCPU( cpu );
		}

		void setController( Controller* controller, size_t port )
		{
			cpu.setController( controller, port );
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
			APU::setMute( mute );
		}

		void saveState( std::ostream& out )
		{
			cpu.saveState( out );
			ppu.saveState( out );
			APU::saveState( out );
			cartridge->saveState( out );
		}

		void loadState( std::istream& in )
		{
			cpu.loadState( in );
			ppu.loadState( in );
			APU::loadState( in );
			cartridge->loadState( in );
		}

		void dump()
		{
			cpu.dumpState();
			// cpu.dumpStack();
			cpu.dump( cpu.getProgramCounter() );
		}

		Cpu cpu;
		Ppu ppu;
		Cartridge* cartridge = nullptr;
	};

}

#endif