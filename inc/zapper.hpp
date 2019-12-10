#ifndef ZAPPER_HPP
#define ZAPPER_HPP

#include "controller.hpp"

#include "Pixel.hpp"

namespace nes
{

	class Zapper : public Controller
	{
	public:

		Zapper( const Pixel* screen ) : m_screen( screen ) {}

		Byte read() override
		{
			return ( ( m_triggerHeld >= 1 ) ? 0x10 : 0 ) | ( detectingLight() ? 0 : 0x08 );
		}

		void write( Byte ) override {}

		void reset() override
		{
			m_triggerHeld = 0;
		}

		void pull()
		{
			// initiate fire
			if ( m_triggerHeld == 0 )
				m_triggerHeld++;
		}

		void update()
		{
			// trigger releases after 2 frames (min amount required by games)
			if ( m_triggerHeld > 0 )
				m_triggerHeld = ( m_triggerHeld + 1 ) % 3;
		}

		bool detectingLight()
		{
			if ( m_x >= 0 && m_x < 256 && m_y >= 0 && m_y < 240 )
			{
				Pixel p = m_screen[ m_x + m_y * 256 ];
				return ( p.r >= 0xf8 ) && ( p.g >= 0xf8 ) && ( p.b >= 0xf8 );
			}

			return false;
		}

		void aim( int x, int y )
		{
			m_x = x;
			m_y = y;
		}

		void setScreenBuffer( const Pixel* screen )
		{
			m_screen = screen;
		}

	private:

		int m_triggerHeld = 0;
		int m_x = 0;
		int m_y = 0;

		const Pixel* m_screen = nullptr;
	};

}

#endif