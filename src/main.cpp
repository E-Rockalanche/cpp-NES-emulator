
#include "main.hpp"

#include "debug.hpp"
#include "filesystem.hpp"
#include "common.hpp"
#include "nes.hpp"
#include "cartridge.hpp"
#include "joypad.hpp"
#include "zapper.hpp"
#include "program_end.hpp"
#include "config.hpp"
#include "keyboard.hpp"
#include "globals.hpp"
#include "api.hpp"
#include "movie.hpp"
#include "hotkeys.hpp"
#include "menu_elements.hpp"
#include "menu_bar.hpp"
#include "message.hpp"

#include <iostream>
#include <iomanip>
#include <string>
#include <ctime>
#include <cstdlib>
#include <fstream>
#include <sstream>

#include "SDL2/SDL.h"

// SDL
SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
SDL_Texture* nes_texture = nullptr;

constexpr size_t ScreenWidth = nes::Ppu::ScreenWidth;
constexpr size_t ScreenHeight = nes::Ppu::ScreenHeight;

// NES
nes::Nes s_nes;
nes::Zapper zapper;
nes::Joypad joypad[ 4 ];
bool paused = false;
bool step_frame = false;
bool in_menu = false;
bool muted = false;

// paths
fs::path rom_filename;
fs::path save_filename;
fs::path rom_folder = std::string( "roms" );
fs::path save_folder = std::string( "saves" );
fs::path screenshot_folder = std::string( "screenshots" );
fs::path movie_folder = std::string( "movies" );
fs::path savestate_folder = std::string( "savestates" );

std::string rom_ext = ".nes";
std::string save_ext = ".sav";
std::string movie_ext = ".nesmov";
std::string savestate_ext = ".state";

// window size
bool fullscreen = false;
float render_scale = 1;
SDL_Rect render_area =
{
	0,
	0,
	ScreenWidth - DefaultCrop * 2,
	ScreenHeight - DefaultCrop * 2
};
SDL_Rect crop_area =
{
	DefaultCrop,
	DefaultCrop,
	ScreenWidth - DefaultCrop,
	ScreenHeight - DefaultCrop
};
int window_width = ScreenWidth - DefaultCrop;
int window_height = ScreenHeight - DefaultCrop;

// frame timing
const unsigned int TARGET_FPS = 60;
const double TIME_PER_FRAME = 1000.0 / TARGET_FPS;
int last_time = 0;
int last_render_time = 0;
double last_wait_time = 0;

// frame rate
int frame_number = 0;
float fps = 0;
float total_fps = 0;
float real_fps = 0;
float total_real_fps = 0;
#define ave_fps ( total_fps / frame_number )
#define ave_real_fps ( total_real_fps / frame_number )

#define FPS_COUNT 15
float fps_count[FPS_COUNT];
std::string fps_text = "fps: 0";

void addFPS( float fps )
{
	for ( int i = 0; i < FPS_COUNT - 1; i++ )
	{
		fps_count[i] = fps_count[i + 1];
	}
	fps_count[FPS_COUNT - 1] = fps;
}

float currentFPS()
{
	float sum = 0;
	for ( int i = 0; i < FPS_COUNT; i++ )
	{
		sum += fps_count[i];
	}
	return sum / FPS_COUNT;
}

void resetFrameNumber()
{
	frame_number = 0;
	total_fps = 0;
	total_real_fps = 0;
	fps = 0;
	real_fps = 0;
}

bool loadFile( std::string filename )
{
	if ( cartridge )
		delete cartridge;
	
	cartridge = nes::Cartridge::loadFile( filename );
	if ( !cartridge )
	{
		return false;
	}
	else
	{
		rom_filename = filename;
		if ( cartridge->hasSRAM() )
		{
			save_filename = save_folder
							/ rom_filename.filename().replace_extension( save_ext );
			cartridge->loadSave( save_filename );
		}
		else
		{
			save_filename = "";
		}
		s_nes.setCartridge( cartridge );
		power();
		return true;
	}
}

bool loadSave( std::string filename )
{
	if ( cartridge && cartridge->hasSRAM() )
	{
		if ( cartridge->loadSave( filename ) )
		{
			save_filename = filename;
			return true;
		}
	}
	return false;
}

void saveGame()
{
	if ( cartridge && cartridge->hasSRAM() )
	{
		cartridge->saveGame( save_filename );
	}
}

// resize window and render area
void resizeRenderArea( bool round_scale )
{
	SDL_GetWindowSize( window, &window_width, &window_height );

	// set viewport to entire window
	SDL_RenderSetViewport( renderer, NULL );

	float x_scale = ( float )window_width / crop_area.w;
	float y_scale = ( float )window_height / crop_area.h;
	render_scale = MIN( x_scale, y_scale );

	if ( round_scale )
	{
		// round down
		render_scale = MAX( ( int )render_scale, 1 );
	}

	render_area.w = crop_area.w * render_scale;
	render_area.h = crop_area.h * render_scale;
	render_area.x = ( window_width - render_area.w ) / 2;
	render_area.y = ( window_height - render_area.h ) / 2;
}

void resizeWindow( int width, int height )
{
	SDL_SetWindowSize( window, width, height );
	resizeRenderArea();
}

void cropScreen( int dx, int dy )
{
	crop_area.x = CLAMP( crop_area.x + dx, 0, MaxCrop );
	crop_area.y = CLAMP( crop_area.y + dy, 0, MaxCrop );
	crop_area.w = ScreenWidth - crop_area.x * 2;
	crop_area.h = ScreenHeight - crop_area.y * 2;
	resizeWindow( crop_area.w * render_scale, crop_area.h * render_scale );
}

// guaranteed close program callback
ProgramEnd pe( []
{
	saveGame();
	std::cout << "Goodbye!\n";
} );

void keyboardEvent( const SDL_Event& event )
{
	SDL_Keycode key = event.key.keysym.sym;
	bool pressed = event.key.state == SDL_PRESSED;
	if ( pressed )
	{
		switch ( key )
		{
			case SDLK_f:
				dbLog( "fps: %f", ave_fps );
				dbLog( "real fps: %f", ave_real_fps );
				break;

			case SDLK_KP_4:
				cropScreen( +1, 0 );
				break;
			case SDLK_KP_6:
				cropScreen( -1, 0 );
				break;
			case SDLK_KP_8:
				cropScreen( 0, -1 );
				break;
			case SDLK_KP_2:
				cropScreen( 0, +1 );
				break;
		}

		pressHotkey( key );
	}

	if ( !Movie::isPlaying() )
	{
		// get joypad input
		for ( int i = 0; i < 4; i++ )
		{
			nes::Joypad::Button button = joypad[i].setKeyState( key, pressed );

			// record button press
			if ( Movie::isRecording() && ( button != nes::Joypad::NONE ) )
			{
				Movie::recordButtonState( frame_number, i, button, pressed );
			}
		}
	}
}

void windowEvent( const SDL_Event& event )
{
	switch ( event.window.event )
	{
		case SDL_WINDOWEVENT_SIZE_CHANGED:
		case SDL_WINDOWEVENT_MAXIMIZED:
		case SDL_WINDOWEVENT_RESTORED:
			resizeRenderArea();
			break;

		case SDL_WINDOWEVENT_CLOSE:
			exit( 0 );
			break;
	}
}

void mouseMotionEvent( const SDL_Event& event )
{
	int tv_x = ( event.motion.x - render_area.x ) / render_scale;
	int tv_y = ( event.motion.y - render_area.y ) / render_scale;
	zapper.aim( tv_x, tv_y );
}

void mouseButtonEvent( const SDL_Event& event )
{
	if ( ( event.button.state == SDL_PRESSED )
		 && ( event.button.button == SDL_BUTTON_LEFT ) )
	{
		zapper.pull();
	}
}

void dropEvent( const SDL_Event& event )
{
	if ( event.type == SDL_DROPFILE )
	{
		fs::path filename = std::string( event.drop.file );
		fs::path extension = filename.extension();
		if ( extension == rom_ext )
		{
			loadFile( filename );
		}
		else if ( extension == save_ext )
		{
			cartridge->loadSave( filename );
		}
		else if ( extension == movie_ext )
		{
			Movie::load( filename );
		}
		else if ( extension == savestate_ext )
		{
			loadState( filename );
		}
		else
		{
			showError( "Error", "Cannot open " + filename.native() );
		}
		SDL_free( event.drop.file );
	}
}

void pollEvents()
{
	SDL_Event event;
	while ( SDL_PollEvent( &event ) )
	{
		switch ( event.type )
		{
			case SDL_KEYDOWN:
			case SDL_KEYUP:
				if ( !event.key.repeat )
				{
					keyboardEvent( event );
				}
				break;

			case SDL_MOUSEMOTION:
				mouseMotionEvent( event );
				break;

			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				mouseButtonEvent( event );
				break;

			case SDL_QUIT:
				exit( 0 );
				break;

			case SDL_WINDOWEVENT:
				windowEvent( event );
				break;

			case SDL_DROPFILE:
				dropEvent( event );
				break;

			case SDL_MENU_EVENT:
				Menu::handleMenuEvent( event );
				break;

			default:
				break;
		}
	}
}

int main( int argc, char** argv )
{
	srand( time( NULL ) );

	nes::Cpu::initialize();

	dbAssertMessage( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_AUDIO ) == 0, "failed to initialize SDL" );

	loadConfig();

	// create window
	int window_flags = SDL_WINDOW_OPENGL | ( fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0 );
	window = SDL_CreateWindow( "NES emulator",
							   SDL_WINDOWPOS_UNDEFINED,
							   SDL_WINDOWPOS_UNDEFINED,
							   window_width, window_height,
							   window_flags );
	dbAssertMessage( window != NULL, "failed to create screen" );
	SDL_SetWindowResizable( window, SDL_bool( true ) );

	// create renderer
	renderer = SDL_CreateRenderer( window, -1, SDL_RENDERER_PRESENTVSYNC );
	dbAssertMessage( renderer != NULL, "failed to create renderer" );

	// create texture
	nes_texture = SDL_CreateTexture( renderer,
									 ( sizeof( Pixel ) == 32 ) ? SDL_PIXELFORMAT_RGBA32 : SDL_PIXELFORMAT_RGB24,
									 SDL_TEXTUREACCESS_STREAMING,
									 ScreenWidth, ScreenHeight );
	dbAssertMessage( nes_texture != NULL, "failed to create texture" );

	// initialize NES
	s_nes.setController( &joypad[ 0 ], 0 );
	s_nes.setController( &zapper, 1 );

	// load ROM from command line
	if ( argc > 1 )
	{
		loadFile( argv[1] );
	}

	Movie::clear();

	constructMenu();

	resizeWindow( window_width, window_height );

	// run emulator
	int last_time = SDL_GetTicks();
	while ( true )
	{
		pollEvents();

		if ( ( !paused || step_frame ) && ( cartridge != NULL ) && !s_nes.halted() )
		{
			if ( Movie::isPlaying() )
			{
				Movie::updateInput( frame_number );
			}
			s_nes.runFrame();
			zapper.update();

			if ( s_nes.halted() )
			{
				std::stringstream ss;
				ss << "The CPU encountered an illegal instruction at address " << std::hex << ( s_nes.cpu.getProgramCounter() - 1 );
				showError( "Error", ss.str() );
				s_nes.dump();
			}
			else
			{
				frame_number++;
				double elapsed = SDL_GetTicks() - last_time;
				total_real_fps += 1000.0 / elapsed;
			}
		}
		step_frame = false;

		// clear the screen
		SDL_SetRenderDrawColor( renderer, 0, 0, 0, 255 ); // black
		SDL_RenderClear( renderer );

		// render nes & gui
		SDL_UpdateTexture( nes_texture, nullptr, s_nes.getPixelBuffer(), ScreenWidth * sizeof ( Pixel ) );
		SDL_RenderCopy( renderer, nes_texture, &crop_area, &render_area );

		// preset screen
		SDL_RenderPresent( renderer );

		int now = SDL_GetTicks();
		if ( !paused )
		{
			double elapsed = now - last_time;
			double current_fps = 1000.0 / elapsed;
			total_fps += current_fps;
			addFPS( current_fps );

			std::stringstream stream;
			stream << std::fixed << std::setprecision( 1 ) << currentFPS();
			fps_text = "fps: " + stream.str();
		}
		last_time = now;
	}

	return 0;
}
