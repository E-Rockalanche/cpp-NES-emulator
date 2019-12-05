#include "ppu.hpp"

#include "cartridge.hpp"
#include "cpu.hpp"

using namespace nes;

namespace
{
	constexpr int PRERENDER_SCANLINE = 261;
	constexpr int NUM_SCANLINES = 262;
	/*
	dummy scanline
	purpose is to fill shift registers with the data for the first two tiles to be rendered
	no pixels are rendered but the PPU still makes the same memory accesses it would for a regular scanline
	*/
	constexpr int POSTRENDER_SCANLINE = 240;
	/*
	The PPU idles during this scanline.
	Accessing PPU memory is safe here but VBLANK is set next scanline
	*/
	constexpr int VBLANK_SCANLINE = 241;
	/*
	VBLANK flag set on second tick of this scanline (and the NMI occurs)
	The PPU makes no memory accesses during the VBLANK scanlines, so the PPU memory can be freely accessed by the program
	*/
	constexpr int NUM_CYCLES = 341;

	constexpr int CHR_START = 0;
	constexpr int CHR_END = 0x1fff;

	constexpr int NAMETABLE_START = 0x2000;
	constexpr int NAMETABLE_END = 0x3eff;

	constexpr int PALETTE_START = 0x3f00;
	constexpr int PALETTE_END = 0x3fff;

	const Pixel s_nesColourPalette[] =
	{
		0x7C7C7C, 0x0000FC, 0x0000BC, 0x4428BC, 0x940084, 0xA80020, 0xA81000, 0x881400,
		0x503000, 0x007800, 0x006800, 0x005800, 0x004058, 0x000000, 0x000000, 0x000000,
		0xBCBCBC, 0x0078F8, 0x0058F8, 0x6844FC, 0xD800CC, 0xE40058, 0xF83800, 0xE45C10,
		0xAC7C00, 0x00B800, 0x00A800, 0x00A844, 0x008888, 0x000000, 0x000000, 0x000000,
		0xF8F8F8, 0x3CBCFC, 0x6888FC, 0x9878F8, 0xF878F8, 0xF85898, 0xF87858, 0xFCA044,
		0xF8B800, 0xB8F818, 0x58D854, 0x58F898, 0x00E8D8, 0x787878, 0x000000, 0x000000,
		0xFCFCFC, 0xA4E4FC, 0xB8B8F8, 0xD8B8F8, 0xF8B8F8, 0xF8A4C0, 0xF0D0B0, 0xFCE0A8,
		0xF8D878, 0xD8F878, 0xB8F8B8, 0xB8F8D8, 0x00FCFC, 0xF8D8F8, 0x000000, 0x000000
	};

	const Byte s_paletteRamBootValues[] = {
		0x09, 0x01, 0x00, 0x01, 0x00, 0x02, 0x02, 0x0D, 0x08, 0x10, 0x08, 0x24, 0x00, 0x00, 0x04, 0x2C,
		0x09, 0x01, 0x34, 0x03, 0x00, 0x04, 0x00, 0x14, 0x08, 0x3A, 0x00, 0x02, 0x00, 0x20, 0x2C, 0x08
	};

	template<typename INT>
	inline bool getBit( INT mask, size_t bit )
	{
		return mask & ( 1 << bit );
	}

	template <typename MASK, typename FLAGS>
	inline bool testFlag( MASK mask, FLAGS flag )
	{
		return mask & flag;
	}

}

inline void Ppu::setStatusFlag( Byte flag )
{
	m_status |= flag;
}

inline void Ppu::clearStatusFlag( Byte flag )
{
	m_status &= ~flag;
}

inline void Ppu::setStatusFlag( Byte flag, bool value )
{
	if ( value )
		setStatusFlag( flag );
	else
		clearStatusFlag( flag );
}

inline Byte Ppu::getSpriteHeight()
{
	return testFlag( m_control, SpriteHeight ) ? 16 : 8;
}

inline void Ppu::clearVBlank()
{
	clearStatusFlag( VBlank );
}

inline size_t Ppu::getSecondaryOamSize()
{
	return m_spriteFlickering ? SECONDARY_OAM_SIZE : PRIMARY_OAM_SIZE;
}

inline Word Ppu::getNametableAddress()
{
	return 0x2000 | ( m_vramAddress & 0x0fff );
}

inline Word Ppu::getAttributeAddress()
{
	return 0x23c0
		| ( m_vramAddress & 0x0c00 )
		| ( ( m_vramAddress >> 4 ) & 0x38 )
		| ( ( m_vramAddress >> 2 ) & 0x07 );
}

inline Word Ppu::getBackgroundAddress()
{
	// used to be added together
	return ( testFlag( m_control, BackgroundTileSelect ) * 0x1000 )
		| ( m_nametableLatch * 16 )
		| ( ( m_vramAddress >> 12 ) & 0x07 );
}

void Ppu::clearScreen()
{
	for( auto& pixel : m_pixels )
		pixel = Pixel( 0, 0, 0 );
}

void Ppu::randomizeClockSync()
{
	// clock can start in one of 4 different cpu synchronization alignments
	for( int i = 0, end = rand() % 4; i < end; ++i )
		tick();
}

void Ppu::power()
{
	m_cycle = NUM_CYCLES - 1;
	m_scanline = PRERENDER_SCANLINE;
	m_oddFrame = false;
	m_control = 0;
	m_mask = 0;
	m_status = 0xc0;
	m_oamAddress = 0;
	m_vramAddress = 0;
	m_tempVRAMAddress = 0;
	m_fineXScroll = 0;
	m_supressVBlank = false;
	m_writeToggle = false;
	m_openBus = 0;
	m_canDraw = false;
	m_spriteZeroNextScanline = false;
	m_spriteZeroThisScanline = false;

	for( auto& value : m_primaryOAM )
		value = 0xff;

	static_assert( PALETTE_SIZE == std::size( s_paletteRamBootValues ) );
	for( size_t i = 0; i < m_palette.size(); ++i )
		m_palette[ i ] = s_paletteRamBootValues[ i ];

	clearScreen();
	randomizeClockSync();
}

void Ppu::reset()
{
	m_cycle = NUM_CYCLES - 1;
	m_scanline = PRERENDER_SCANLINE;
	m_oddFrame = false;
	m_control = 0;
	m_mask = 0;
	m_supressVBlank = false;
	m_writeToggle = false;
	m_canDraw = false;
	m_spriteZeroNextScanline = false;
	m_spriteZeroThisScanline = false;

	clearScreen();
	randomizeClockSync();
}

bool Ppu::readyToDraw()
{
	if ( !m_canDraw )
		return false;

	m_canDraw = false;
	return true;
}

Byte Ppu::readRegister( size_t reg )
{
	switch( static_cast<PpuRegister>( reg ) )
	{
		case PpuRegister::Status:
		{
			m_openBus = ( m_openBus & 0x1f ) | m_status;
			clearVBlank();
			m_writeToggle = false;

			// vblank supression
			if ( m_scanline == VBLANK_SCANLINE )
			{
				switch( m_cycle )
				{
					case 0:
						m_supressVBlank = true;
						break;

					case 1 ... 2:
						m_cpu->setNMI( false );
						break;
				}
			}
			break;
		}

		case PpuRegister::OamData:
			m_openBus = m_primaryOAM[ m_oamAddress ];
			break;

		case PpuRegister::Data:
		{
			Word address = m_vramAddress & 0x3fff;
			if ( address >= PALETTE_START )
			{
				// read buffer is set to mirrored nametable "underneath" palette
				m_readBuffer = m_nametable[ nametableMirror( address ) ];

				// no buffering
				m_openBus = read( address );
			}
			else
			{
				m_openBus = m_readBuffer;
				m_readBuffer = read( address );
			}
			incrementVRAMAddress();
			break;
		}

		default:
			break;
	}

	return m_openBus;
}

void Ppu::writeRegister( size_t reg, Byte value )
{
	m_openBus = value;
	switch( static_cast<PpuRegister>( reg ) )
	{
		case PpuRegister::Control:
			writeToControl( value );
			break;

		case PpuRegister::Mask:
			if ( !testFlag( m_mask, ShowBackground) && testFlag( value, ShowBackground ) )
				dbLog( "enable bg. s=%i, c=%i", m_scanline, m_cycle );

			m_mask = value;
			break;

		case PpuRegister::OamAddress:
			m_oamAddress = value;
			break;

		case PpuRegister::OamData:
			m_primaryOAM[ m_oamAddress++ ] = value;
			break;

		case PpuRegister::Scroll:
			writeToScroll( value );
			break;

		case PpuRegister::Address:
			writeToAddress( value );
			break;

		case PpuRegister::Data:
			write( m_vramAddress & 0x3fff, value );
			incrementVRAMAddress();
			break;

		default:
			break;
	}
}

void Ppu::writeToControl( Byte value )
{
	if ( !testFlag( m_control, NmiEnable )
		&& testFlag( value, NmiEnable )
		&& testFlag( m_status, VBlank )
		&& ( m_scanline != PRERENDER_SCANLINE ) )
	{
		// manual NMI trigger during vblank
		m_cpu->setNMI( true );
	}
	else if ( testFlag( m_control, NmiEnable )
		&& !testFlag( value, NmiEnable )
		&& ( m_scanline == VBLANK_SCANLINE )
		&& ( m_cycle <= 2 ) )
	{
		// supress NMI near vblank
		m_cpu->setNMI( false );
	}

	m_control = value;

	// set name tables bits
	m_tempVRAMAddress = ( m_tempVRAMAddress & 0x73ff )
		| ( ( value & 0x03 ) << 10 );
}

void Ppu::writeToScroll( Byte value )
{
	if ( !m_writeToggle )
	{
		// first write
		m_fineXScroll = value & 0x07;

		// set coarse x
		m_tempVRAMAddress = ( m_tempVRAMAddress & 0x7fe0 )
			| ( value >> 3 );
	}
	else
	{
		// second write
		int fineY = value & 0x07;
		int coarseY = value >> 3;
		m_tempVRAMAddress = ( m_tempVRAMAddress & 0x0c1f )
			| ( fineY << 12 )
			| ( coarseY << 5 );
	}
	m_writeToggle = !m_writeToggle;
}

void Ppu::writeToAddress( Byte value )
{
	if ( !m_writeToggle )
	{
		// first write
		// set address high
		m_tempVRAMAddress = ( m_tempVRAMAddress & 0x00ff )
			| ( ( value & 0x3f ) << 8 );
	}
	else
	{
		// second write
		// set address low
		m_tempVRAMAddress = ( m_tempVRAMAddress & 0x7f00 ) | value;
		m_vramAddress = m_tempVRAMAddress;
	}
	m_writeToggle = !m_writeToggle;
}

Byte Ppu::read( Word address )
{
	switch( address )
	{
		case CHR_START ... CHR_END:
			dbAssert( m_cartridge );
			return m_cartridge->readCHR( address );

		case NAMETABLE_START ... NAMETABLE_END:
			return m_nametable[ nametableMirror( address ) ];

		case PALETTE_START ... PALETTE_END:
		{
			if ( ( address & 0x0013 ) == 0x0010 )
				address &= ~0x0010;

			return m_palette[ address ]
				& ( testFlag( m_mask, GreyScale ) ? 0x30 : 0xff );
		}

		default:
			dbBreakMessage( "reading outside PPU memory");
			return 0;
	}
}

void Ppu::write( Word address, Byte value )
{
	switch( address )
	{
		case CHR_START ... CHR_END:
			dbAssert( m_cartridge );
			m_cartridge->writeCHR( address, value );
			break;

		case NAMETABLE_START ... NAMETABLE_END:
		{
			m_nametable[ nametableMirror( address ) ] = value;
			break;
		}

		case PALETTE_START ... PALETTE_END:
		{
			if ( ( address & 0x13 ) == 0x10 )
				address &= ~0x10;

			m_palette[ address ] = value;
			break;
		}

		default:
			dbBreakMessage( "writing outside PPU memory" );
			break;
	}
}

void Ppu::setVBlank()
{
	if ( m_supressVBlank )
	{
		m_supressVBlank = false;
		return;
	}

	setStatusFlag( VBlank );
	if ( testFlag( m_control, NmiEnable ) )
		m_cpu->setNMI();
}

void Ppu::clearOAM()
{
	auto it = m_secondaryOAM.data();
	auto end = it + getSecondaryOamSize() * OBJECT_SIZE;
	for( ; it != end; ++it )
	{
		*it = 0xff;
	}
}

void Ppu::tick()
{
	m_cycle = ( m_cycle + 1 ) % NUM_CYCLES;

	if ( m_cycle == 0 )
	{
		m_scanline = ( m_scanline + 1 ) % NUM_SCANLINES;
		if ( m_scanline == 0 )
		{
			m_oddFrame = !m_oddFrame;
			++m_frame;

			if ( m_oddFrame && renderingEnabled() )
				m_cycle = 1;
		}
	}

	constexpr auto LastVisibleScanline = ScreenHeight - 1;
	switch( m_scanline )
	{
		case 0 ... LastVisibleScanline:
			scanlineCycle<Scanline::Visible>();
			break;

		case POSTRENDER_SCANLINE:
			scanlineCycle<Scanline::PostRender>();
			break;

		case VBLANK_SCANLINE:
			scanlineCycle<Scanline::VBlankLine>();
			break;

		case 242 ... 260:
			break;

		case PRERENDER_SCANLINE:
			scanlineCycle<Scanline::PreRender>();
			break;

		default:
			dbBreak();
			break;
	}
}

template <Ppu::Scanline s>
void Ppu::scanlineCycle()
{
	if ( ( s == Scanline::VBlankLine ) && ( m_cycle == 1 ) )
	{
		setVBlank();
	}
	else if ( ( s == Scanline::PostRender ) && ( m_cycle == 0 ) )
	{
		m_canDraw = true;
	}
	else if constexpr ( ( s == Scanline::Visible ) || ( s == Scanline::PreRender ) )
	{
		// sprites:
		switch( m_cycle )
		{
			case 1:
			{
				clearOAM();
				if constexpr ( s == Scanline::PreRender )
				{
					clearStatusFlag( SpriteOverflow );
					clearStatusFlag( SpriteZeroHit );
					clearVBlank();
				}
				break;
			}

			case 257:
				loadSpritesOnScanline();
				break;

			case 321:
				loadSpriteRegisters();
				break;
		}

		// background:
		switch( m_cycle )
		{
			case 2 ... 255:
			case 322 ... 337:
			{
				renderPixel();
				switch( m_cycle % 8 )
				{
					// nametable:
					case 1:
					{
						m_renderAddress = getNametableAddress();
						loadShiftRegisters();
						break;
					}

					case 2:
						m_nametableLatch = read( m_renderAddress );
						break;

					// attribute:
					case 3:
						m_renderAddress = getAttributeAddress();
						break;

					case 4:
					{
						m_attributeLatch = read( m_renderAddress );
						switch( m_vramAddress & 0x42 )
						{
							case 0x02:
								m_attributeLatch >>= 2;
								break;

							case 0x40:
								m_attributeLatch >>= 4;
								break;

							case 0x42:
								m_attributeLatch >>= 6;
								break;
						}
						break;
					}

					// background
					case 5:
						m_renderAddress = getBackgroundAddress();
						break;

					case 6:
						m_bgLatchLow = read( m_renderAddress );
						break;

					case 7:
						m_renderAddress += 8;
						break;

					case 0:
					{
						m_bgLatchHigh = read( m_renderAddress );
						incrementXComponent();
						break;
					}
				}
				break;
			}

			case 256:
			{
				renderPixel();
				m_bgLatchHigh = read( m_renderAddress );
				incrementYComponent();
				break;
			}

			case 257:
			{
				renderPixel();
				loadShiftRegisters();
				updateVRAMX();
				break;
			}

			case 280 ... 304:
			{
				if constexpr ( s == Scanline::PreRender )
					updateVRAMY();
				break;
			}

			// no shift loading:
			case 1:
			case 321:
			case 339:
				m_renderAddress = getNametableAddress();
				break;

			// nametable fetch instead of attribute
			case 338:
			case 340:
				m_nametableLatch = read( m_renderAddress );
				break;
		}

		// signal scanline to MMC3 cartridge
		if ( renderingEnabled()
			&& ( ( m_control & 0x10 )
				? ( m_cycle == 324 || m_cycle == 4 )
				: ( m_cycle == 260 ) ) )
		{
			dbAssert( m_cartridge );
			m_cartridge->signalScanline();
		}
	}
}

void Ppu::incrementXComponent()
{
	if ( !renderingEnabled() )
		return;

	if ( ( m_vramAddress & 0x001f ) == 0x001f ) // if coarse x == 31
	{
		// set coarse x to 0
		// switch horizontal nametable
		m_vramAddress ^= 0x041f;
	}
	else
	{
		++m_vramAddress;
	}
}

void Ppu::incrementYComponent()
{
	if ( !renderingEnabled() )
		return;

	if ( ( m_vramAddress & 0x7000 ) != 0x7000 ) // if fine Y != 7
	{
		// increment fine Y
		m_vramAddress += 0x1000;
	}
	else
	{
		m_vramAddress &= ~0x7000; // set fine Y to 0

		int y = ( m_vramAddress >> 5 ) & 0x1f; // let y = coarse Y
		switch( y )
		{
			case 29:
			{
				y = 0;
				m_vramAddress ^= 0x0800; // switch vertical nametable
				break;
			}

			case 31:
				y = 0;
				break;

			default:
				++y;
				break;
		}

		// set coarse Y
		m_vramAddress = ( m_vramAddress & ~0x03e0 ) | ( y << 5 );
	}
}

void Ppu::updateVRAMX()
{
	constexpr Word XComponent = 0x041f;

	if ( renderingEnabled() )
	{
		m_vramAddress = ( m_vramAddress & ~XComponent )
			| ( m_tempVRAMAddress & XComponent );
	}
}

void Ppu::updateVRAMY()
{
	constexpr Word YComponent = 0x7be0;

	if ( renderingEnabled() )
	{
		m_vramAddress = ( m_vramAddress & ~YComponent )
			| ( m_tempVRAMAddress & YComponent );
	}
}

void Ppu::incrementVRAMAddress()
{
	if ( ( ( m_scanline < (int)ScreenHeight ) || ( m_scanline == PRERENDER_SCANLINE ) )
		&& renderingEnabled() )
	{
		incrementXComponent();
		incrementYComponent();
	}
	else
	{
		m_vramAddress += testFlag( m_control, IncrementMode ) ? 32 : 1;
	}
}

Word Ppu::nametableMirror( Word address )
{
	dbAssert( m_cartridge );
	switch( m_cartridge->nameTableMirroring() )
	{
		case Cartridge::NameTableMirroring::Horizontal:
			return ( ( address / 2 ) & 0x400 )
				+ ( address % 0x400 );

		case Cartridge::NameTableMirroring::Vertical:
			return address % 0x800;
	}

	dbBreak();
	return 0;
}

void Ppu::loadShiftRegisters()
{
	m_bgShiftLow = ( m_bgShiftLow & 0xff00 ) | m_bgLatchLow;
	m_bgShiftHigh = ( m_bgShiftHigh & 0xff00 ) | m_bgLatchHigh;

	m_attributeLatchLow = m_attributeLatch & 0x01;
	m_attributeLatchHigh = m_attributeLatch & 0x02;
}

void Ppu::loadSpritesOnScanline()
{
	m_spriteZeroNextScanline = false;

	int scanlineY = ( m_scanline == PRERENDER_SCANLINE )
		? -1
		: m_scanline;

	auto spriteHeight = getSpriteHeight();

	size_t destIndex = 0;
	size_t secondaryOamSize = getSecondaryOamSize();

	for( size_t i = 0; i < PRIMARY_OAM_SIZE; ++i )
	{
		Byte* srcObject = m_primaryOAM.data() + i * OBJECT_SIZE;

		int y = scanlineY - srcObject[ ObjectVariable::YPos ];

		if ( ( y < 0 ) || ( y >= spriteHeight ) )
		{
			continue;
		}

		if ( i == 0 )
			m_spriteZeroNextScanline = true;

		if ( destIndex == SECONDARY_OAM_SIZE )
		{
			if ( renderingEnabled() )
				setStatusFlag( SpriteOverflow );

			if ( m_spriteFlickering )
				break;
		}

		if ( destIndex < secondaryOamSize )
		{
			// copy object from primary OAM to secondary OAM
			Byte* destObject = m_secondaryOAM.data() + destIndex * OBJECT_SIZE;
			for( size_t a = 0; a < OBJECT_SIZE; ++a )
			{
				destObject[ a ] = srcObject[ a ];
			}
			++destIndex;
		}
	}
}

void Ppu::loadSpriteRegisters()
{
	m_spriteZeroThisScanline = m_spriteZeroNextScanline;

	const auto spriteHeight = getSpriteHeight();
	const bool tallSprites = ( spriteHeight == 16 );

	const auto size = getSecondaryOamSize();
	for( size_t i = 0; i < size; ++i )
	{
		Byte* object = m_secondaryOAM.data() + i * OBJECT_SIZE;
		const Byte tile = object[ ObjectVariable::TileIndex ];

		Word address = 0;

		if ( tallSprites )
		{
			address = ( ( tile & 1 ) * 0x1000 )
				+ ( ( tile & ~1 ) * 16 );
		}
		else // sprite height == 8
		{
			address = ( testFlag( m_control, SpriteTileSelect ) * 0x1000 )
				+ ( tile * 16 );
		}

		size_t spriteY = ( m_scanline - object[ ObjectVariable::YPos ] )
			% spriteHeight;

		if ( testFlag( object[ ObjectVariable::Attributes ], FlipVertically ) )
		{
			spriteY ^= spriteHeight - 1;
		}

		// select seconds tile if on 8x16
		address += spriteY + ( spriteY & 0x08 );

		// load registers
		m_spriteShiftLow[ i ] = read( address );
		m_spriteShiftHigh[ i ] = read( address + 8 );
		m_spriteXCounter[ i ] = object[ ObjectVariable::XPos ];
		m_spriteAttributeLatch[ i ] = object[ ObjectVariable::Attributes ];
	}
}

void Ppu::renderPixel()
{
	renderPixelInternal();

	m_bgShiftLow <<= 1;
	m_bgShiftHigh <<= 1;

	m_attributeShiftLow = ( m_attributeShiftLow << 1 ) | m_attributeLatchLow;
	m_attributeShiftHigh = ( m_attributeShiftHigh << 1 ) | m_attributeLatchHigh;
}

void Ppu::renderPixelInternal()
{
	if ( !renderingEnabled() )
		return;

	int x = m_cycle - 2;
	if ( m_scanline >= (int)ScreenHeight || x < 0 || x >= (int)ScreenWidth )
		return;

	Byte palette = 0;
	Byte objectPalette = 0;
	bool objectPriority = false;

	const bool showBackground = testFlag( m_mask, ShowBackground );
	if ( showBackground
		&& ( testFlag( m_mask, ShowBackgroundLeft8 ) || ( x >= 8 ) ) )
	{
		size_t bgBit = 15 - m_fineXScroll;
		palette = ( getBit( m_bgShiftHigh, bgBit ) << 1 )
			| getBit( m_bgShiftLow, bgBit );

		if ( palette != 0 )
		{
			palette |= ( getBit( m_attributeShiftHigh, 7 - m_fineXScroll ) << 3 )
				| ( getBit( m_attributeShiftLow, 7 - m_fineXScroll ) << 2 );
		}
	}

	const bool showSprites = testFlag( m_mask, ShowSprite );
	if ( showSprites
		&& ( testFlag( m_mask, ShowSpriteLeft8 ) || ( x >= 8 ) )
		&& ( x < 255 ) )
	{
		auto size = getSecondaryOamSize();
		for( size_t i = size-1; i --> 0; )
		{
			Byte attributes = m_spriteAttributeLatch[ i ];

			int spriteX = x - m_spriteXCounter[ i ];
			if ( spriteX < 0 || spriteX >= 8 )
				continue;

			if ( testFlag( attributes, FlipHorizontally ) )
			{
				spriteX ^= 0x07;
			}

			Byte spritePalette = ( getBit( m_spriteShiftHigh[ i ], 7 - spriteX ) << 1 )
				| getBit( m_spriteShiftLow[ i ], 7 - spriteX );

			// continue if transparent
			if ( spritePalette == 0 )
				continue;

			if ( m_spriteZeroThisScanline
				&& ( i == 0 )
				&& showBackground
				&& ( palette != 0 ) )
			{
				m_spriteZeroThisScanline = false;
				setStatusFlag( SpriteZeroHit );
			}

			spritePalette |= ( attributes & ObjectAttribute::SpritePalette ) << 2;
			objectPalette = spritePalette + 16;
			objectPriority = testFlag( attributes, ObjectAttribute::Priority );
		}
	}

	if ( ( objectPalette != 0 ) && ( palette == 0 || !objectPriority ) )
	{
		palette = objectPalette;
	}

	size_t paletteIndex = read( PALETTE_START + palette );
	m_pixels[ m_scanline * ScreenWidth + x ] = s_nesColourPalette[ paletteIndex ];
}

#define writeBytes( var ) out.write( (const char*)&var, sizeof( var ) );
#define readBytes( var ) in.read( (char*)&var, sizeof( var ) );

void Ppu::saveState( std::ostream& out )
{
	writeBytes( m_nametable )
	writeBytes( m_palette )
	writeBytes( m_primaryOAM )
	writeBytes( m_secondaryOAM )
	writeBytes( m_spriteShiftLow )
	writeBytes( m_spriteShiftHigh )
	writeBytes( m_spriteXCounter )
	writeBytes( m_spriteAttributeLatch );

	writeBytes( m_canDraw )
	writeBytes( m_openBus )
	writeBytes( m_readBuffer )
	writeBytes( m_renderAddress )
	writeBytes( m_writeToggle )
	writeBytes( m_control )
	writeBytes( m_mask )
	writeBytes( m_status )
	writeBytes( m_oamAddress )
	writeBytes( m_vramAddress )
	writeBytes( m_tempVRAMAddress )
	writeBytes( m_fineXScroll )
	writeBytes( m_supressVBlank )
	writeBytes( m_bgLatchLow )
	writeBytes( m_bgLatchHigh )
	writeBytes( m_bgShiftLow )
	writeBytes( m_bgShiftHigh )
	writeBytes( m_attributeLatch )
	writeBytes( m_attributeLatchLow )
	writeBytes( m_attributeLatchHigh )
	writeBytes( m_attributeShiftLow )
	writeBytes( m_attributeShiftHigh )
	writeBytes( m_nametableLatch )
	writeBytes( m_spriteZeroNextScanline )
	writeBytes( m_spriteZeroThisScanline )
	writeBytes( m_spriteZeroHit )
	writeBytes( m_cycle )
	writeBytes( m_scanline )
	writeBytes( m_oddFrame )
}

void Ppu::loadState( std::istream& in )
{
	readBytes( m_nametable )
	readBytes( m_palette )
	readBytes( m_primaryOAM )
	readBytes( m_secondaryOAM )
	readBytes( m_spriteShiftLow )
	readBytes( m_spriteShiftHigh )
	readBytes( m_spriteXCounter )
	readBytes( m_spriteAttributeLatch );
	
	readBytes( m_canDraw )
	readBytes( m_openBus )
	readBytes( m_readBuffer )
	readBytes( m_renderAddress )
	readBytes( m_writeToggle )
	readBytes( m_control )
	readBytes( m_mask )
	readBytes( m_status )
	readBytes( m_oamAddress )
	readBytes( m_vramAddress )
	readBytes( m_tempVRAMAddress )
	readBytes( m_fineXScroll )
	readBytes( m_supressVBlank )
	readBytes( m_bgLatchLow )
	readBytes( m_bgLatchHigh )
	readBytes( m_bgShiftLow )
	readBytes( m_bgShiftHigh )
	readBytes( m_attributeLatch )
	readBytes( m_attributeLatchLow )
	readBytes( m_attributeLatchHigh )
	readBytes( m_attributeShiftLow )
	readBytes( m_attributeShiftHigh )
	readBytes( m_nametableLatch )
	readBytes( m_spriteZeroNextScanline )
	readBytes( m_spriteZeroThisScanline )
	readBytes( m_spriteZeroHit )
	readBytes( m_cycle )
	readBytes( m_scanline )
	readBytes( m_oddFrame )
}

#undef writeBytes
#undef readBytes