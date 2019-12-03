#ifndef NES_PPU_HPP
#define NES_PPU_HPP

#include "Pixel.hpp"
#include "ppu_defs.hpp"
#include "Ram.hpp"
#include "types.hpp"

#include <iostream>

namespace nes
{
	class Cartridge;
	class Cpu;

	class Ppu
	{
	public:

		Ppu() { power(); }

		void setCPU( Cpu& cpu )
		{
			m_cpu = &cpu;
		}

		void setCartridge( Cartridge* cartridge )
		{
			m_cartridge = cartridge;
		}

		void power();
		void reset();

		bool readyToDraw();

		void tick();

		Byte readRegister( size_t reg );
		void writeRegister( size_t reg, Byte value );

		void writeToOAM( Byte value )
		{
			m_primaryOAM[ m_oamAddress++ ] = value;
		}

		bool renderingEnabled()
		{
			return m_mask & ( ShowBackground | ShowSprite );
		}

		bool nmiEnabled()
		{
			return m_control & NmiEnable;
		}

		bool getSpriteFlickering() const { return m_spriteFlickering; }
		void setSpriteFlickering( bool flicker ) { m_spriteFlickering = flicker; }

		void saveState( std::ostream& out );
		void loadState( std::istream& in );

		const Pixel* getPixelBuffer() const
		{
			return m_pixels;
		}

		static constexpr size_t ScreenWidth = 256;
		static constexpr size_t ScreenHeight = 240;

	private:

		enum class Scanline
		{
			Visible,
			PostRender,
			PreRender,
			VBlankLine
		};

		enum ObjectVariable
		{
			YPos,
			TileIndex,
			Attributes,
			XPos,
		};
		static constexpr size_t OBJECT_SIZE = 4;

		enum ObjectAttribute
		{
			SpritePalette = 0x03,
			Priority = 1 << 5, // 0: front, 1: back
			FlipHorizontally = 1 << 6,
			FlipVertically = 1 << 7
		};

		static constexpr size_t NAMETABLE_SIZE = 0x0800;
		static constexpr size_t PALETTE_SIZE = 0x0020;
		static constexpr size_t PRIMARY_OAM_SIZE = 64;
		static constexpr size_t SECONDARY_OAM_SIZE = 8;

	private:

		void clearScreen();
		void randomizeClockSync();
		void clearOAM();
		void setStatusFlag( Byte flag );
		void clearStatusFlag( Byte flag );
		void setStatusFlag( Byte flag, bool value );
		void writeToControl( Byte value );
		void writeToScroll( Byte value );
		void writeToAddress( Byte value );
		void setVBlank();
		void clearVBlank();
		void incrementVRAMAddress();
		void incrementVRAMX();
		void incrementVRAMY();
		void updateVRAMX();
		void updateVRAMY();
		void renderPixel();
		void renderPixelInternal();
		void loadSpritesOnScanline();
		void loadSpriteRegisters();
		void loadShiftRegisters();
		void incrementXComponent();
		void incrementYComponent();

		Byte read( Word address );
		void write( Word address, Byte value );

		Byte getSpriteHeight();

		size_t getSecondaryOamSize();

		Word getNametableAddress();
		Word getAttributeAddress();
		Word getBackgroundAddress();
		Word nametableMirror( Word address );

		template <Scanline s>
		void scanlineCycle();

	private:

		Cpu* m_cpu = nullptr;
		Cartridge* m_cartridge = nullptr;

		int m_frame = 0;
		int m_cycle = 0;
		int m_scanline = 0;

		Pixel m_pixels[ ScreenWidth * ScreenHeight ];

		Word m_renderAddress = 0;
		Word m_vramAddress = 0;
		Word m_tempVRAMAddress = 0;
		Word m_bgShiftLow = 0;
		Word m_bgShiftHigh = 0;

		Ram<NAMETABLE_SIZE> m_nametable;
		Ram<PALETTE_SIZE> m_palette;
		Ram<PRIMARY_OAM_SIZE * OBJECT_SIZE> m_primaryOAM;
		Ram<PRIMARY_OAM_SIZE * OBJECT_SIZE> m_secondaryOAM; // need full size to turn off sprite flickering
		Ram<PRIMARY_OAM_SIZE, false> m_spriteShiftLow;
		Ram<PRIMARY_OAM_SIZE, false> m_spriteShiftHigh;
		Ram<PRIMARY_OAM_SIZE, false> m_spriteXCounter;
		Ram<PRIMARY_OAM_SIZE, false> m_spriteAttributeLatch;

		Byte m_openBus = 0;
		Byte m_readBuffer = 0;
		Byte m_control = 0;
		Byte m_mask = 0;
		Byte m_status = 0;
		Byte m_oamAddress = 0;
		Byte m_fineXScroll = 0;
		Byte m_bgLatchLow = 0;
		Byte m_bgLatchHigh = 0;
		Byte m_attributeLatch = 0;
		Byte m_attributeShiftLow = 0;
		Byte m_attributeShiftHigh = 0;
		Byte m_nametableLatch = 0;

		bool m_canDraw = false;
		bool m_spriteFlickering = true;
		bool m_writeToggle = false;
		bool m_supressVBlank = false;
		bool m_attributeLatchLow = false;
		bool m_attributeLatchHigh = false;
		bool m_spriteZeroNextScanline = false;
		bool m_spriteZeroThisScanline = false;
		bool m_spriteZeroHit = false;
		bool m_oddFrame = false;
	};
}

#endif

