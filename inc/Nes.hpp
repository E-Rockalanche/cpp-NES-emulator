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
			m_cpu.setPPU( m_ppu );
			m_ppu.setCPU( m_cpu );
			APU::init();
			APU::setDmcReader( [this]( void*, cpu_addr_t address ){ return (int)m_cpu.read( address ); } );
		}

		void power()
		{
			m_cpu.power();
			m_ppu.power();
			APU::reset();
		}

		void reset()
		{
			m_cpu.reset();
			m_ppu.reset();
			APU::reset();
		}

		void setCartridge( Cartridge* cartridge )
		{
			m_cartridge = cartridge;

			m_cpu.setCartridge( cartridge );
			m_ppu.setCartridge( cartridge );
			
			if ( cartridge )
				cartridge->setCPU( m_cpu );
		}

		void setController( Controller* controller, size_t port )
		{
			m_cpu.setController( controller, port );
		}

		const Pixel* getPixelBuffer() const
		{
			return m_ppu.getPixelBuffer();
		}

		void runFrame()
		{
			m_cpu.runFrame();
		}

		bool halted() const
		{
			return m_cpu.halted();
		}

		bool getSpriteFlickering() const
		{
			return m_ppu.getSpriteFlickering();
		}

		void setSpriteFlickering( bool on )
		{
			m_ppu.setSpriteFlickering( on );
		}

		void setMute( bool mute )
		{
			APU::setMute( mute );
		}

		void saveState( std::ostream& out )
		{
			m_cpu.saveState( out );
			m_ppu.saveState( out );
			APU::saveState( out );
			m_cartridge->saveState( out );
		}

		void loadState( std::istream& in )
		{
			m_cpu.loadState( in );
			m_ppu.loadState( in );
			APU::loadState( in );
			m_cartridge->loadState( in );
		}

	private:

		Cpu m_cpu;
		Ppu m_ppu;
		Cartridge* m_cartridge = nullptr;
	};

}

#endif