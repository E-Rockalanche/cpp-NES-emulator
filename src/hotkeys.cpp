
#include "hotkeys.hpp"

#include "common.hpp"
#include "globals.hpp"
#include "movie.hpp"
#include "api.hpp"
#include "cartridge.hpp"
#include "main.hpp"
#include "menu_bar.hpp"
#include "message.hpp"

#include <fstream>

#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"

void quit()
{
	exit( 0 );
}

void reset()
{
	s_nes.reset();
	resetFrameNumber();
}

void power()
{
	s_nes.power();
	resetFrameNumber();
}

void toggleSpriteFlickering()
{
	bool flicker = !s_nes.getSpriteFlickering();
	s_nes.setSpriteFlickering( flicker );
	sprite_flicker_button.check( flicker );
}

// hiding menu before and show after fullscreen toggle prevents window size changes on Windows
void setFullscreen( bool on )
{
	fullscreen = on;
	int flags = fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;

	if ( fullscreen )
	{
		menu_bar.hide();
		fullscreen_button.check();
	}

	SDL_SetWindowFullscreen( window, flags );

	if ( !fullscreen )
	{
		menu_bar.show();
		fullscreen_button.uncheck();
	}
}
void toggleFullscreen()
{
	setFullscreen( !fullscreen );
}

void setResolutionScale( float scale )
{
	setFullscreen( false );
	resizeWindow( crop_area.w * scale, crop_area.h * scale );
}
void setResolutionScale1()
{
	setResolutionScale( 1 );
}
void setResolutionScale2()
{
	setResolutionScale( 2 );
}
void setResolutionScale3()
{
	setResolutionScale( 3 );
}

void setPaused( bool pause )
{
	paused = pause;
	pause_button.check( paused );
}
void togglePaused()
{
	setPaused( !paused );
}

void stepFrame()
{
	step_frame = true;
}

void setMute( bool mute )
{
	muted = mute;
	s_nes.setMute( mute );
	mute_button.check( mute );
}
void toggleMute()
{
	setMute( !muted );
}

void selectRom()
{
	API::FileDialog dialog( "Select ROM" );
	dialog.setFilter( "NES ROM\0*.nes\0Any file\0*.*\0" );
	dialog.setDirectory( rom_folder.c_str() );
	std::string filename = dialog.getOpenFileName();

	if ( filename.size() > 0 )
	{
		loadFile( filename );
	}
}

void closeFile()
{
	saveGame();
	s_nes.setCartridge( nullptr );
	rom_filename = "";
	save_filename = "";
	Movie::clear();
}

void toggleRecording()
{
	switch ( Movie::getState() )
	{
		case Movie::PLAYING:
			Movie::stopPlayback();
			power();
			Movie::startRecording();
			break;

		case Movie::NONE:
			power();
			Movie::startRecording();
			break;

		case Movie::RECORDING:
			Movie::stopRecording();
			break;

		default:
			break;
	}
}

void togglePlayback()
{
	switch ( Movie::getState() )
	{
		case Movie::RECORDING:
			Movie::stopRecording();
			power();
			Movie::startPlayback();
			break;

		case Movie::NONE:
			power();
			Movie::startPlayback();
			break;

		case Movie::PLAYING:
			Movie::stopPlayback();
			break;

		default:
			break;
	}
}

void saveMovie()
{
	API::FileDialog dialog( "Save Movie" );
	dialog.setFilter( "NES Movie\0*.nesmov\0" );
	dialog.setDirectory( movie_folder.c_str() );

	fs::path default_name = movie_folder
		/ fs::path( rom_filename ).filename().replace_extension( movie_ext );
	dialog.setFilename( default_name.c_str() );

	std::string filename = dialog.getSaveFileName();

	if ( !filename.empty() )
	{
		filename += movie_ext;
		if ( !Movie::save( filename ) )
		{
			showError( "Error", "Failed to save movie to " + filename );
		}
	}
}

void loadMovie()
{
	API::FileDialog dialog( "Open Movie" );
	dialog.setFilter( "NES Movie\0*.nesmov\0" );
	dialog.setDirectory( movie_folder.c_str() );
	std::string filename = dialog.getOpenFileName();

	if ( Movie::load( filename ) )
	{
		power();
		Movie::startPlayback();
	}
	else
	{
		showError( "Error", "Failed to load movie from " + filename );
	}
}

void takeScreenshot()
{
	SDL_Surface* surface = SDL_CreateRGBSurface( 0, nes::Ppu::ScreenWidth, nes::Ppu::ScreenHeight,
						   24, Pixel::r_mask, Pixel::g_mask, Pixel::b_mask, Pixel::a_mask );
	surface->pixels = const_cast<Pixel*>( s_nes.getPixelBuffer() );

	int timestamp = std::time( NULL );
	std::string name = "screenshot_" + std::to_string( timestamp ) + ".png";
	fs::path filename = screenshot_folder / name;
	IMG_SavePNG( surface, filename.c_str() );
}

void saveState( const std::string& filename )
{
	if ( s_nes.cartridge != nullptr )
	{
		std::ofstream fout( filename.c_str(), std::ios::binary );
		if ( fout.is_open() )
		{
			uint32_t checksum = s_nes.cartridge->getChecksum();
			writeBinary( fout, checksum );
			s_nes.saveState( fout );
			fout.close();
		}
	}
}

void saveState()
{
	fs::path filename = savestate_folder
						/ rom_filename.filename().replace_extension( savestate_ext );
	saveState( filename );
}

void loadState( const std::string& filename )
{
	if ( s_nes.cartridge != nullptr )
	{
		std::ifstream fin( filename.c_str(), std::ios::binary );
		if ( fin.is_open() )
		{
			unsigned int checksum = 0;
			readBinary( fin, checksum );
			bool load = true;
			if ( s_nes.cartridge->getChecksum() != checksum )
			{
				load = askYesNo( "Warning",
								 "This save state appears to be for a different game. Load it anyway?",
								 WARNING_ICON );
			}
			if ( load )
			{
				s_nes.loadState( fin );
			}
			fin.close();
		}
	}
}

void loadState()
{
	fs::path filename = savestate_folder
						/ rom_filename.filename().replace_extension( savestate_ext );
	loadState( filename );
}

std::vector<Hotkey> hotkeys =
{
	{ SDLK_ESCAPE, quit },
	{ SDLK_F9, takeScreenshot},
	{ SDLK_F11, toggleFullscreen},
	{ SDLK_m, toggleMute},
	{ SDLK_p, togglePaused},
	{ SDLK_s, stepFrame},
	{ SDLK_r, toggleRecording},
	{ SDLK_a, togglePlayback},
	{ SDLK_F5, saveState},
	{ SDLK_F6, loadState}
};

void pressHotkey( SDL_Keycode key )
{
	for ( int i = 0; i < ( int )hotkeys.size(); i++ )
	{
		if ( key == hotkeys[i].key )
		{
			( *hotkeys[i].callback )();
		}
	}
}