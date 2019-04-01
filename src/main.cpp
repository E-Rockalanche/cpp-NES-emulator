#include <iostream>
#include <iomanip>
#include <string>
#include <ctime>
#include <cstdlib>
#include <fstream>

#include "SDL2/SDL.h"

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
#include "gui.hpp"
#include "keyboard.hpp"
#include "globals.hpp"
#include "api.hpp"
#include "assert.hpp"

// GUI
Gui::HBox top_gui({ 0, 0, window_width, GUI_BAR_HEIGHT });
Gui::Element* active_menu = &top_gui;

// SDL
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

// NES
Joypad joypad[4];
Zapper zapper;
bool paused = true;
bool in_menu = false;
bool muted = false;

const int SCREEN_BPP = 24; // bits per pixel
Pixel screen[SCREEN_WIDTH * SCREEN_HEIGHT];

// paths
std::string rom_filename;
std::string save_filename;
std::string save_folder;
std::string rom_folder;
std::string screenshot_folder;

// window size
int window_width;
int window_height;
bool fullscreen;
float render_scale;
SDL_Rect render_area = { 0, GUI_BAR_HEIGHT, 256, 240 };

// frame timing
const unsigned int TARGET_FPS = 60;
const double TIME_PER_FRAME = 1000.0 / TARGET_FPS;
int last_time = 0;
int last_render_time = 0;
double last_wait_time = 0;

// frame rate
int total_frames = 0;
float fps = 0;
float total_fps = 0;
float real_fps = 0;
float total_real_fps = 0;
#define ave_fps (total_fps / total_frames)
#define ave_real_fps (total_real_fps / total_frames)

std::string stripFilename(std::string path) {
	bool found_ext = false;
	int start = path.size() - 1;
	int end = path.size();
	while(start > 0 && path[start-1] != '/' && path[start-1] != '\\') {
		if (!found_ext && path[start] == '.') {
			end = start;
			found_ext = true;
		}
		start--;
	}
	return std::string(path, start, end - start);
}

void reset() {
	if (cartridge != NULL) {
		clearScreen();
		CPU::reset();
		PPU::reset();
		APU::reset();
		paused = false;
	} else {
		paused = true;
	}
}

void power() {
	if (cartridge != NULL) {
		clearScreen();
		CPU::power();
		PPU::power();
		APU::reset();
		paused = false;
	} else {
		paused = true;
	}
}

bool loadFile(std::string filename) {
	if (cartridge) delete cartridge;
	cartridge = Cartridge::loadFile(filename);
	if (!cartridge) {
		dout("could not load " << filename);
		return false;
	} else {
		rom_filename = stripFilename(filename);
		save_filename = save_folder + rom_filename + ".sav";
		if (cartridge->hasSRAM()) {
			cartridge->loadSave(save_filename);
		}
		return true;
	}
}

bool loadSave(std::string filename) {
	if (cartridge && cartridge->hasSRAM()) {
		return cartridge->loadSave(filename);
	} else {
		return false;
	}
}

void saveGame() {
	if (cartridge && cartridge->hasSRAM()) {
		cartridge->saveGame(save_filename);
	}
}

// hotkeys
void quit() { exit(0); }

void toggleFullscreen() {
	fullscreen = !fullscreen;
	int flags = fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;
	SDL_SetWindowFullscreen(window, flags);
}

void togglePaused() {
	paused = !paused || (cartridge == NULL);
}

void toggleMute() {
	muted = !muted;
	APU::mute(muted);
}

void selectRom() {
	std::string filename = API::getFilename("Select a ROM", "NES ROM\0*.nes\0Any file\0*.*\0");
	if ((filename.size() > 0) && loadFile(filename)) {
		power();
	}
}

void closeFile() {
	saveGame();
	delete cartridge;
	cartridge = NULL;
	paused = true;
	rom_filename = "";
	save_filename = "";
	clearScreen();
}

typedef void(*Callback)(void);
struct Hotkey {
	enum Type {
		QUIT,
		FULLSCREEN,
		PAUSE,
		MUTE,

		NUM_HOTKEYS
	};
	SDL_Keycode key;
	Callback callback;
};
Hotkey hotkeys[Hotkey::NUM_HOTKEYS] = {
	{ SDLK_ESCAPE, quit },
	{ SDLK_F11, toggleFullscreen},
	{ SDLK_p, togglePaused},
	{ SDLK_m, toggleMute}
};

void pressHotkey(SDL_Keycode key) {
	for(int i = 0; i < Hotkey::NUM_HOTKEYS; i++) {
		if (key == hotkeys[i].key) {
			(*hotkeys[i].callback)();
		}
	}
}

// guaranteed close program callback
ProgramEnd pe([](void){
	saveGame();
	std::cout << "Goodbye!\n";
});

void step() {
	if (CPU::halted()) {
		std::cout << "HALTED\n";
	} else {
		CPU::execute();
	}
}

int readAddress() {
	std::string raw;
	std::cin >> raw;
	return std::stoi(raw, NULL, 16);
}

void resizeRenderArea(bool round_scale = false) {
	SDL_GetWindowSize(window, &window_width, &window_height);
	top_gui.setSize(window_width, GUI_BAR_HEIGHT);

	// set viewport to fill window
	SDL_RenderSetViewport(renderer, NULL);

	int gui_height = fullscreen ? 0 : GUI_BAR_HEIGHT;

	int allowed_height = window_height - gui_height;
	float x_scale = (float)allowed_height / SCREEN_HEIGHT;
	float y_scale = (float)window_width / SCREEN_WIDTH;
	render_scale = MIN(x_scale, y_scale);

	if (round_scale) {
		// round down
		render_scale = (int)render_scale;
	}

	render_area.w = SCREEN_WIDTH * render_scale;
	render_area.h = SCREEN_HEIGHT * render_scale;
	render_area.x = (window_width - render_area.w) / 2;
	render_area.y = gui_height + (allowed_height - render_area.h) / 2;
}

void resizeWindow(int width, int height) {
	SDL_SetWindowSize(window, width, height + GUI_BAR_HEIGHT);
	resizeRenderArea();
}

void keyboardEvent(const SDL_Event& event) {
	SDL_Keycode key = event.key.keysym.sym;
	if (event.key.state == SDL_PRESSED) {
		if (key == SDLK_f) {
			dout("fps: " << ave_fps);
			dout("real fps: " << ave_real_fps);
		}

		pressHotkey(key);
		for(int i = 0; i < 4; i++) joypad[i].pressKey(key);
	} else {
		for(int i = 0; i < 4; i++) joypad[i].releaseKey(key);
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
	active_menu->mouseMotion(event.motion.x, event.motion.y);

	int tv_x = (event.motion.x - render_area.x) / render_scale;
	int tv_y = (event.motion.y - render_area.y) / render_scale;
	zapper.aim(tv_x, tv_y);
}

void mouseButtonEvent(const SDL_Event& event) {
	if (event.button.state == SDL_PRESSED) {
		if (event.button.button == SDL_BUTTON_LEFT) {
			bool clicked_menu = false;
			if (active_menu) {
				clicked_menu = active_menu->click(event.button.x, event.button.y);
			}
			if (!clicked_menu) {
				int gui_height = fullscreen ? 0 : GUI_BAR_HEIGHT;
				if (event.button.y > gui_height) {
					zapper.pull();
				}
			}
		}
	} else if (event.button.state == SDL_RELEASED) {
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

			default: break;
		}
	}
}

int main(int argc, char* argv[]) {
	srand(time(NULL));

	assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) == 0, "failed to initialize SDL");
	Gui::init();

	loadConfig();

	// create window
	int window_flags = SDL_WINDOW_OPENGL | (fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
	window = SDL_CreateWindow("NES emulator",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		window_width, window_height + GUI_BAR_HEIGHT,
		window_flags);
	assert(window != NULL, "failed to create screen");
	SDL_SetWindowResizable(window, SDL_bool(true));

	// create renderer
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
	assert(renderer != NULL, "failed to create renderer");
	resizeRenderArea();

	// create texture
	SDL_Texture* nes_texture = SDL_CreateTexture(renderer,
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
		if (loadFile(argv[1])) {
			power();
		}
	}

	// A, B, select, start, up, down, left, right
	joypad[0].mapButtons((const int[8]){ SDLK_x, SDLK_z, SDLK_RSHIFT, SDLK_RETURN,
		SDLK_UP, SDLK_DOWN, SDLK_LEFT, SDLK_RIGHT });

	// build gui bar
	const SDL_Rect gui_button_rect = { 0, 0, 64, GUI_BAR_HEIGHT };
	Gui::DropDown file_dropdown = Gui::DropDown(gui_button_rect, "File");
	Gui::DropDown view_dropdown = Gui::DropDown(gui_button_rect, "View");
	Gui::DropDown options_dropdown = Gui::DropDown(gui_button_rect, "Options");
	top_gui.addElement(file_dropdown);
	top_gui.addElement(view_dropdown);
	top_gui.addElement(options_dropdown);

	Gui::Button load_rom_button = Gui::Button(gui_button_rect, "Load", selectRom);
	Gui::Button close_rom_button = Gui::Button(gui_button_rect, "Close", closeFile);
	file_dropdown.addElement(load_rom_button);
	file_dropdown.addElement(close_rom_button);

	Gui::Button fullscreen_button = Gui::Button(gui_button_rect, "Fullscreen", toggleFullscreen);
	view_dropdown.addElement(fullscreen_button);

	Gui::Button pause_button = Gui::Button(gui_button_rect, "Pause", togglePaused);
	Gui::Button mute_button = Gui::Button(gui_button_rect, "Mute", toggleMute);
	options_dropdown.addElement(pause_button);
	options_dropdown.addElement(mute_button);


	// run emulator
	int last_time = SDL_GetTicks();
	while(true) {
		pollEvents();

		if (!paused) {

			CPU::runFrame();
			zapper.update();
			total_frames++;
			double elapsed = SDL_GetTicks() - last_time;
			total_real_fps += 1000.0/elapsed;
		}

		// clear the screen
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // black
		SDL_RenderClear(renderer);

		// render nes & gui
		SDL_UpdateTexture(nes_texture, NULL, screen, SCREEN_WIDTH * sizeof (Pixel));
		SDL_RenderCopy(renderer, nes_texture, NULL, &render_area);
		if (!fullscreen || paused) {
			top_gui.render();
		}

		// preset screen
		SDL_RenderPresent(renderer);

		int now = SDL_GetTicks();
		if (!paused) {
			double elapsed = now - last_time;
			total_fps += 1000.0/elapsed;
		}
		last_time = now;
	}

	return 0;
}
