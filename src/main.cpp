
#include "api.hpp"
#include "Cartridge.hpp"
#include "config.hpp"
#include "common.hpp"
#include "debug.hpp"
#include "filesystem.hpp"
#include "globals.hpp"
#include "hotkeys.hpp"
#include "joypad.hpp"
#include "keyboard.hpp"
#include "menu_bar.hpp"
#include "menu_elements.hpp"
#include "message.hpp"
#include "movie.hpp"
#include "nes.hpp"
#include "program_end.hpp"
#include "rom_loader.hpp"
#include "zapper.hpp"
#include "RGBTexture.hpp"

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include <SDL2/SDL.h>
#include <glad/glad.h>

#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

const double targetFPS = 60.0;
const double targetMPF = 1000.0 / targetFPS;

// SDL
SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
SDL_Texture* nes_texture = nullptr;

constexpr size_t ScreenWidth = nes::Ppu::ScreenWidth;
constexpr size_t ScreenHeight = nes::Ppu::ScreenHeight;

// NES
nes::Nes s_nes;
nes::Zapper zapper( s_nes.getPixelBuffer() );
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

Rect render_area = {
	0,
	0,
	ScreenWidth - DefaultCrop * 2,
	ScreenHeight - DefaultCrop * 2
};

Rect crop_area = {
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
float real_fps = 0;
float total_fps = 0;
float total_real_fps = 0;
#define ave_fps ( total_fps / frame_number )
#define ave_real_fps ( total_real_fps / frame_number )

#define FPS_COUNT 15
float fps_count[FPS_COUNT];
std::string fps_text = "fps: 0";

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
	auto cartridge = nes::Rom::load( filename.c_str() );

	if ( !cartridge )
		return false;

	rom_filename = filename;
	if ( cartridge->hasSRAM() )
	{
		save_filename = save_folder / rom_filename.filename().replace_extension( save_ext );
		cartridge->loadSave( save_filename.c_str() );
	}
	else
	{
		save_filename = "";
	}

	s_nes.setCartridge( std::move( cartridge ) );
	power();
	Movie::clear();

	return true;
}

bool loadSave( std::string filename )
{
	auto cartridge = s_nes.cartridge.get();
	if ( cartridge && cartridge->hasSRAM() )
	{
		if ( cartridge->loadSave( filename.c_str() ) )
		{
			save_filename = filename;
			return true;
		}
	}
	return false;
}

void saveGame()
{
	auto cartridge = s_nes.cartridge.get();
	if ( cartridge && cartridge->hasSRAM() )
	{
		cartridge->saveGame( save_filename.c_str() );
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
	bool pressed = ( event.key.state == SDL_PRESSED );
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

			case SDLK_d:
			{
				std::string str;
				std::cin >> str;
				nes::Word addr = std::stoi( str, nullptr, 16 );
				s_nes.cpu.dump( addr );
			}
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
			auto cartridge = s_nes.cartridge.get();
			cartridge->loadSave( filename.c_str() );
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
		ImGui_ImplSDL2_ProcessEvent( &event );

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

	dbAssertMessage( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER ) == 0, "failed to initialize SDL" );

    const char* glslVersion = "#version 130";
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_FLAGS, 0 );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 0 );

    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
    SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 24 );
    SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, 8 );

	loadConfig();

	// create window
	int windowFlags = SDL_WINDOW_OPENGL
		| SDL_WINDOW_RESIZABLE
		| ( fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0 );
	window = SDL_CreateWindow( "NES emulator",
							   SDL_WINDOWPOS_UNDEFINED,
							   SDL_WINDOWPOS_UNDEFINED,
							   window_width, window_height,
							   windowFlags );
	dbAssertMessage( window, "failed to create window" );

	SDL_GLContext openglContext = SDL_GL_CreateContext( window );
    SDL_GL_MakeCurrent( window, openglContext );
    // SDL_GL_SetSwapInterval( 1 );

    if ( !gladLoadGL() )
    {
    	dbLogError( "failed to initialize opengl loader" );
    	return 1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable;

    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForOpenGL( window, openglContext );
    ImGui_ImplOpenGL3_Init( glslVersion );

	constructMenu();

	RGBTexture nesTexture( ScreenWidth, ScreenHeight, s_nes.getPixelBuffer() );

	// initialize NES
	nes::Cpu::initialize();
	s_nes.setController( &joypad[ 0 ], 0 );
	s_nes.setController( &zapper, 1 );

	// load ROM from command line
	if ( argc > 1 )
	{
		loadFile( argv[1] );
	}

	Movie::clear();

	resizeWindow( window_width, window_height );

	// run emulator
	while ( true )
	{
		auto frameStart = SDL_GetTicks();
		pollEvents();

		if ( ( !paused || step_frame ) && ( s_nes.cartridge != nullptr ) && !s_nes.halted() )
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
				paused = true;
			}
		}
		step_frame = false;

		glClearColor( 0, 0, 0, 1 );
		glClear( GL_COLOR_BUFFER_BIT );

		nesTexture.updateData( s_nes.getPixelBuffer() );
		nesTexture.render( render_area, crop_area );

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame( window );
        ImGui::NewFrame();

		ImGui::Begin( "Perf" );
		ImGui::Text( "fps: %f", fps );
		ImGui::Text( "real fps: %f", real_fps );
		ImGui::End();
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData( ImGui::GetDrawData() );

		SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
		SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		SDL_GL_MakeCurrent( backup_current_window, backup_current_context );

		// present screen
		// SDL_RenderPresent( renderer );
        SDL_GL_SwapWindow( window );

     	auto elapsed = SDL_GetTicks() - frameStart;

		if ( !paused )
		{
			real_fps = 1000.0 / elapsed;
			total_real_fps += real_fps;
		}

		if ( elapsed < targetMPF )
		{
			SDL_Delay( targetMPF - elapsed );
		}

		if ( !paused )
		{
			auto elapsed = SDL_GetTicks() - frameStart;
			fps = 1000.0 / elapsed;
			total_fps += fps;

			frame_number++;
		}
	}

	nesTexture.~RGBTexture();

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext( openglContext );
    SDL_DestroyWindow( window );
    SDL_Quit();

	return 0;
}
