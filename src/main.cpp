#include <iostream>
#include <iomanip>
#include <string>
#include <ctime>
#include <cstdlib>
#include <fstream>
#include <sstream>

#include "SDL2/SDL.h"

#include "filesystem.hpp"

#include "common.hpp"
#include "main.hpp"
#include "debugging.hpp"
#include "cpu.hpp"
#include "ppu.hpp"
#include "apu.hpp"
#include "cartridge.hpp"
#include "joypad.hpp"
#include "zapper.hpp"
#include "program_end.hpp"
#include "screen.hpp"
#include "config.hpp"
#include "keyboard.hpp"
#include "globals.hpp"
#include "api.hpp"
#include "movie.hpp"
#include "hotkeys.hpp"
#include "menu_elements.hpp"
#include "menu_bar.hpp"
#include "message.hpp"
#include "assert.hpp"

// SDL
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_Texture* nes_texture = NULL;

// NES
Zapper zapper;
bool paused = false;
bool step_frame = false;
bool in_menu = false;
bool muted = false;
Pixel screen[SCREEN_WIDTH * SCREEN_HEIGHT];

// paths
fs::path rom_filename;
fs::path save_filename;
fs::path rom_folder = std::string("roms");
fs::path save_folder = std::string("saves");
fs::path screenshot_folder = std::string("screenshots");
fs::path movie_folder = std::string("movies");
fs::path savestate_folder = std::string("savestates");

std::string rom_ext = ".nes";
std::string save_ext = ".sav";
std::string movie_ext = ".nesmov";
std::string savestate_ext = ".state";

// window size
bool fullscreen = false;
float render_scale = 1;
SDL_Rect render_area = {
	0,
	0,
	SCREEN_WIDTH - DEFAULT_CROP,
	SCREEN_HEIGHT - DEFAULT_CROP
};
SDL_Rect crop_area = {
	DEFAULT_CROP,
	DEFAULT_CROP,
	SCREEN_WIDTH - DEFAULT_CROP,
	SCREEN_HEIGHT - DEFAULT_CROP
};
int window_width = SCREEN_WIDTH - DEFAULT_CROP;
int window_height = SCREEN_HEIGHT - DEFAULT_CROP;

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
#define ave_fps (total_fps / frame_number)
#define ave_real_fps (total_real_fps / frame_number)

#define FPS_COUNT 15
float fps_count[FPS_COUNT];
std::string fps_text = "fps: 0";

void addFPS(float fps) {
	for(int i = 0; i < FPS_COUNT-1; i++) {
		fps_count[i] = fps_count[i+1];
	}
	fps_count[FPS_COUNT-1] = fps;
}

float currentFPS() {
	float sum = 0;
	for(int i = 0; i < FPS_COUNT; i++) {
		sum += fps_count[i];
	}
	return sum / FPS_COUNT;
}

void resetFrameNumber() {
	frame_number = 0;
	total_fps = 0;
	total_real_fps = 0;
	fps = 0;
	real_fps = 0;
}

bool loadFile(std::string filename) {
	if (cartridge) delete cartridge;
	cartridge = Cartridge::loadFile(filename);
	if (!cartridge) {
		dout("could not load " << filename);
		return false;
	} else {
		rom_filename = filename;
		if (cartridge->hasSRAM()) {
			save_filename = save_folder
				/ rom_filename.filename().replace_extension(save_ext);
			cartridge->loadSave(save_filename);
		} else {
			save_filename = "";
		}
		power();
		return true;
	}
}

bool loadSave(std::string filename) {
	if (cartridge && cartridge->hasSRAM()) {
		if (cartridge->loadSave(filename)) {
			save_filename = filename;
			return true;
		}
	}
	return false;
}

void saveGame() {
	if (cartridge && cartridge->hasSRAM()) {
		cartridge->saveGame(save_filename);
	}
}

// resize window and render area
void resizeRenderArea(bool round_scale) {
	SDL_GetWindowSize(window, &window_width, &window_height);

	// dout("window size: " << window_width << ", " << window_height);

	// set viewport to entire window
	SDL_RenderSetViewport(renderer, NULL);

	float x_scale = (float)window_width / crop_area.w;
	float y_scale = (float)window_height / crop_area.h;
	render_scale = MIN(x_scale, y_scale);

	if (round_scale) {
		// round down
		render_scale = MAX((int)render_scale, 1);
	}

	render_area.w = crop_area.w * render_scale;
	render_area.h = crop_area.h * render_scale;
	render_area.x = (window_width - render_area.w) / 2;
	render_area.y = (window_height - render_area.h) / 2;
}

void resizeWindow(int width, int height) {
	SDL_SetWindowSize(window, width, height);
	resizeRenderArea();
}

void cropScreen(int dx, int dy) {
	crop_area.x = CLAMP(crop_area.x + dx, 0, MAX_CROP);
	crop_area.y = CLAMP(crop_area.y + dy, 0, MAX_CROP);
	crop_area.w = SCREEN_WIDTH - crop_area.x * 2;
	crop_area.h = SCREEN_HEIGHT - crop_area.y * 2;
	resizeWindow(crop_area.w * render_scale, crop_area.h * render_scale);
}

// guaranteed close program callback
ProgramEnd pe([]{
	saveGame();
	std::cout << "Goodbye!\n";
});

void keyboardEvent(const SDL_Event& event) {
	SDL_Keycode key = event.key.keysym.sym;
	bool pressed = event.key.state == SDL_PRESSED;
	if (pressed) {
		switch(key) {
			case SDLK_f:
				dout("fps: " << ave_fps);
				dout("real fps: " << ave_real_fps);
				break;

			case SDLK_KP_4: cropScreen(+1, 0); break;
			case SDLK_KP_6: cropScreen(-1, 0); break;
			case SDLK_KP_8: cropScreen(0, -1); break;
			case SDLK_KP_2: cropScreen(0, +1); break;
		}

		pressHotkey(key);
	}

	if (!Movie::isPlaying()) {
		// get joypad input
		for(int i = 0; i < 4; i++) {
			Joypad::Button button = joypad[i].setKeyState(key, pressed);

			// record button press
			if (Movie::isRecording() && (button != Joypad::NONE)) {
				Movie::recordButtonState(frame_number, i, button, pressed);
			}
		}
	}
}

void windowEvent(const SDL_Event& event) {
	switch(event.window.event) {
		case SDL_WINDOWEVENT_SIZE_CHANGED:
		case SDL_WINDOWEVENT_MAXIMIZED:
		case SDL_WINDOWEVENT_RESTORED:
			resizeRenderArea();
			break;

		case SDL_WINDOWEVENT_CLOSE:
			exit(0);
			break;
	}
}

void mouseMotionEvent(const SDL_Event& event) {
	int tv_x = (event.motion.x - render_area.x) / render_scale;
	int tv_y = (event.motion.y - render_area.y) / render_scale;
	zapper.aim(tv_x, tv_y);
}

void mouseButtonEvent(const SDL_Event& event) {
	if ((event.button.state == SDL_PRESSED)
	&& (event.button.button == SDL_BUTTON_LEFT)) {
		zapper.pull();
	}
}

void dropEvent(const SDL_Event& event) {
	if (event.type == SDL_DROPFILE) {
		fs::path filename = std::string(event.drop.file);
		fs::path extension = filename.extension();
		if (extension == rom_ext) {
			loadFile(filename);
		} else if (extension == save_ext) {
			cartridge->loadSave(filename);
		} else if (extension == movie_ext) {
			Movie::load(filename);
		} else if (extension == savestate_ext) {
			loadState(filename);
		} else {
			dout("cannot open " << filename.native());
		}
		SDL_free(event.drop.file);
	}
}

void pollEvents() {
	SDL_Event event;
	while(SDL_PollEvent(&event)) {
		switch(event.type) {
			case SDL_KEYDOWN:
			case SDL_KEYUP:
				if (!event.key.repeat) {
					keyboardEvent(event);
				}
				break;

			case SDL_MOUSEMOTION:
				mouseMotionEvent(event);
				break;

			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				mouseButtonEvent(event);
				break;

			case SDL_QUIT:
				exit(0);
				break;

			case SDL_WINDOWEVENT:
				windowEvent(event);
				break;

		    case SDL_DROPFILE:
		    	dropEvent(event);
		    	break;

		    case SDL_MENU_EVENT:
		    	Menu::handleMenuEvent(event);
		        break;

			default: break;
		}
	}
}

int main(int argc, char** argv) {
	srand(time(NULL));

	assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) == 0, "failed to initialize SDL");

	loadConfig();

	// create window
	int window_flags = SDL_WINDOW_OPENGL | (fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
	window = SDL_CreateWindow("NES emulator",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		window_width, window_height,
		window_flags);
	assert(window != NULL, "failed to create screen");
	SDL_SetWindowResizable(window, SDL_bool(true));

	// create renderer
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
	assert(renderer != NULL, "failed to create renderer");

	// create texture
	nes_texture = SDL_CreateTexture(renderer,
		(sizeof(Pixel) == 32) ? SDL_PIXELFORMAT_RGBA32 : SDL_PIXELFORMAT_RGB24,
		SDL_TEXTUREACCESS_STREAMING,
		SCREEN_WIDTH, SCREEN_HEIGHT);
	assert(nes_texture != NULL, "failed to create texture");

	// initialize NES
	controller_ports[0] = &joypad[0];
	controller_ports[1] = &zapper;
	CPU::init();
	APU::init();

	// load ROM from command line
	if (argc > 1) {
		loadFile(argv[1]);
	}

	Movie::clear();

	constructMenu();

	resizeWindow(window_width, window_height);

	// run emulator
	int last_time = SDL_GetTicks();
	while(true) {
		pollEvents();

		if ((!paused || step_frame) && (cartridge != NULL) && !CPU::halted()) {
			if (Movie::isPlaying()) {
				Movie::updateInput(frame_number);
			}
			CPU::runFrame();
			zapper.update();

			if (CPU::halted()) {
				showError("Error", "The CPU encountered an illegal instruction");
			} else {
				frame_number++;
				double elapsed = SDL_GetTicks() - last_time;
				total_real_fps += 1000.0/elapsed;
			}
		}
		step_frame = false;

		// clear the screen
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // black
		SDL_RenderClear(renderer);

		// render nes & gui
		SDL_UpdateTexture(nes_texture, NULL, screen, SCREEN_WIDTH * sizeof (Pixel));
		SDL_RenderCopy(renderer, nes_texture, &crop_area, &render_area);

		// preset screen
		SDL_RenderPresent(renderer);

		int now = SDL_GetTicks();
		if (!paused) {
			double elapsed = now - last_time;
			double current_fps = 1000.0/elapsed;
			total_fps += current_fps;
			addFPS(current_fps);

			std::stringstream stream;
			stream << std::fixed << std::setprecision(1) << currentFPS();
			fps_text = "fps: " + stream.str();
		}
		last_time = now;
	}

	return 0;
}
