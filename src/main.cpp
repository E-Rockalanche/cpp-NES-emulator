// standard library
#include <iostream>
#include <iomanip>
#include <string>
#include <ctime>
#include <cstdlib>
#include <fstream>

// nes
#include "common.hpp"
#include "main.hpp"
#include "debugging.hpp"
#include "cpu.hpp"
#include "ppu.hpp"
#include "apu.hpp"
#include "cartridge.hpp"
#include "joypad.hpp"
#include "zapper.hpp"
#include "file_path.hpp"
#include "program_end.hpp"
#include "screen.hpp"
#include "keyboard.hpp"

// config
#include "json.hpp"
using nlohmann::json;

// graphics
#include "SDL2/SDL.h"

#include "assert.hpp"

// SDL
SDL_Window* sdl_window = NULL;
SDL_Renderer* sdl_renderer = NULL;

// NES
Joypad joypad[4];
Zapper zapper;
bool paused = true;

const int SCREEN_BPP = 24; // bits per pixel
Pixel screen[SCREEN_WIDTH * SCREEN_HEIGHT];

// paths
std::string file_name = "";
std::string save_path = "./";
std::string rom_path = "./";
std::string screenshot_path = "./";

// window size
int window_width = SCREEN_WIDTH;
int window_height = SCREEN_HEIGHT;
bool fullscreen = false;
float render_scale = 1;
int render_width = SCREEN_WIDTH;
int render_height = SCREEN_HEIGHT;

// frame timing
const unsigned int TARGET_FPS = 60;
const double TIME_PER_FRAME = 1000.0 / TARGET_FPS;
int last_time = 0;
int last_render_time = 0;
double last_wait_time = 0;

// frame rate
int total_frames = 0;
float fps = 0;
float real_fps = 0;
float total_fps = 0;
float total_real_fps = 0;
#define ave_fps (total_fps / total_frames)
#define ave_real_fps (total_real_fps / total_frames)

// config
json config;

// hotkeys
void quit() { exit(0); }

void toggleFullscreen() {
	fullscreen = !fullscreen;
	SDL_SetWindowFullscreen(sdl_window, fullscreen);
}

void togglePaused() {
	paused = !paused || (cartridge == NULL);
}

typedef void(*Callback)(void);
struct Hotkey {
	enum Type {
		QUIT,
		FULLSCREEN,
		PAUSE,

		NUM_HOTKEYS
	};
	SDL_Keycode key;
	Callback callback;
};
Hotkey hotkeys[Hotkey::NUM_HOTKEYS] = {
	{ SDLK_ESCAPE, quit },
	{ SDLK_F11, toggleFullscreen},
	{ SDLK_p, togglePaused}
};

void pressHotkey(SDL_Keycode key) {
	for(int i = 0; i < Hotkey::NUM_HOTKEYS; i++) {
		if (key == hotkeys[i].key) {
			(*hotkeys[i].callback)();
		}
	}
}

Sound_Queue* sound_queue = NULL;
void newSamples(const blip_sample_t* samples, size_t count)
{
    sound_queue->write(samples, count);
}

const char* CONFIG_FILE = "config.json";
void loadConfig() {
	dout("load config");

	config = R"(
	{
		"general": {
			"fullscreen": false,
			"scale": 2.0,
			"sprite_flickering": false
		},
		"hotkeys": {
			"quit": "Q",
			"fullscreen": "F11",
			"pause": "P"
		},
		"controls": [{
			"A": "X",
			"B": "Z",
			"select": "RIGHTSHIFT",
			"start": "RETURN",
			"up": "UP",
			"down": "DOWN",
			"left": "LEFT",
			"right": "RIGHT"
		}, {
			"A": "X",
			"B": "Z",
			"select": "RIGHTSHIFT",
			"start": "RETURN",
			"up": "UP",
			"down": "DOWN",
			"left": "LEFT",
			"right": "RIGHT"
		}, {
			"A": "X",
			"B": "Z",
			"select": "RIGHTSHIFT",
			"start": "RETURN",
			"up": "UP",
			"down": "DOWN",
			"left": "LEFT",
			"right": "RIGHT"
		}, {
			"A": "X",
			"B": "Z",
			"select": "RIGHTSHIFT",
			"start": "RETURN",
			"up": "UP",
			"down": "DOWN",
			"left": "LEFT",
			"right": "RIGHT"
		}]
	})"_json;

	std::ifstream fin(CONFIG_FILE);
	if (fin.is_open()) {
		// load config
		json config_patch;
		fin >> config_patch;
		fin.close();
		config.merge_patch(config_patch);
	} else {
		// save default config
		std::ofstream fout(CONFIG_FILE);
		assert((fout.is_open()), "Cannot save default config");
		fout << std::setw(4) << config;
		fout.close();
	}

	// general
	const json& general = config["general"];
	PPU::sprite_flickering = general["sprite_flickering"].get<bool>();
	fullscreen = general["fullscreen"].get<bool>();
	render_scale = general["scale"].get<float>();
	window_width = render_scale * SCREEN_WIDTH;
	window_height = render_scale * SCREEN_HEIGHT;

	//joypad 1
	for(int j = 0; j < 4; j++) {
		const json& keys = config["controls"][j];
		for(int b = 0; b < Joypad::NUM_BUTTONS; b++) {
			const char* button_name = Joypad::getButtonName((Joypad::Button)b);
			std::string key_name = keys[button_name].get<std::string>();
			SDL_Keycode keycode = getKeycode(key_name);
			joypad[j].mapButton((Joypad::Button)b, keycode);
		}
	}
}

bool loadFile(std::string filename) {
	if (cartridge) delete cartridge;
	cartridge = Cartridge::loadFile(filename);
	if (!cartridge) {
		dout("could not load " << filename);
		return false;
	} else {
		if (cartridge->hasSRAM()) {
			file_name = getFilename(filename);
			cartridge->loadSave(save_path + file_name + ".sav");
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

// guaranteed close program callback
void saveGame() {
	std::cout << "Goodbye!\n";

	if (cartridge && cartridge->hasSRAM()) {
		cartridge->saveGame(save_path + file_name + ".sav");
	}
}
ProgramEnd pe(saveGame);

void reset() {
	if (cartridge != NULL) {
		CPU::reset();
		PPU::reset();
		paused = false;
	} else {
		paused = true;
	}
}

void power() {
	if (cartridge != NULL) {
		CPU::power();
		PPU::power();
		paused = false;
	} else {
		paused = true;
	}
}

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

void resizeRender() {
	dout("resize render");

	SDL_GetWindowSize(sdl_window, &window_width, &window_height);

	// calculate largest screen scale in current window size
	float x_scale = (float)window_width / SCREEN_WIDTH;
	float y_scale = (float)window_height / SCREEN_HEIGHT;
	float scale = MIN(x_scale, y_scale);

	// set new screen size
	render_width = scale * SCREEN_WIDTH;
	render_height = scale * SCREEN_HEIGHT;

	SDL_RenderSetLogicalSize(sdl_renderer, render_width, render_height);
}

void resizeWindow(int width, int height) {
	dout("resize window");

	SDL_SetWindowSize(sdl_window, width, height);
	resizeRender();
}

void keyboardEvent(const SDL_Event& event) {
	SDL_Keycode key = event.key.keysym.sym;
	if (event.key.state == SDL_PRESSED) {
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
			resizeRender();
			break;

		case SDL_WINDOWEVENT_CLOSE:
			exit(0);
			break;
	}
}

void mouseMotionEvent(const SDL_Event& event) {
	int tv_x = event.motion.x / render_scale;
	int tv_y = event.motion.y / render_scale;

	zapper.aim(tv_x, tv_y);
}

void mouseButtonEvent(const SDL_Event& event) {
	if (event.button.state == SDL_PRESSED) {
		if (event.button.button == SDL_BUTTON_LEFT) {
			zapper.pull();
		}
	}
}

void pollEvents() {
	SDL_Event event;
	while(SDL_PollEvent(&event)) {
		switch(event.type) {
			case SDL_KEYDOWN:
			case SDL_KEYUP:
				keyboardEvent(event);
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

	assert(SDL_Init(SDL_INIT_VIDEO) == 0, "failed to initialize SDL");

	loadConfig();

	// create window
	int window_flags = SDL_WINDOW_OPENGL | (fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
	sdl_window = SDL_CreateWindow("NES emulator",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		window_width, window_height,
		window_flags);
	assert(sdl_window != NULL, "failed to create screen");
	SDL_SetWindowResizable(sdl_window, SDL_bool(true));

	// create renderer
	sdl_renderer = SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_PRESENTVSYNC);
	assert(sdl_renderer != NULL, "failed to create renderer");
	resizeRender();

	// create texture
	SDL_Texture* sdl_texture = SDL_CreateTexture(sdl_renderer,
		(sizeof(Pixel) == 32) ? SDL_PIXELFORMAT_RGBA32 : SDL_PIXELFORMAT_RGB24,
		SDL_TEXTUREACCESS_STREAMING,
		SCREEN_WIDTH, SCREEN_HEIGHT);
	assert(sdl_texture != NULL, "failed to create texture");

	// initialize NES
	controller_ports[0] = &joypad[0];
	controller_ports[1] = &zapper;
	CPU::init();
	APU::init();
    sound_queue = new Sound_Queue;
    sound_queue->init(96000);

	// A, B, select, start, up, down, left, right
	joypad[0].mapButtons((const int[8]){ SDLK_x, SDLK_z, SDLK_RSHIFT, SDLK_RETURN,
		SDLK_UP, SDLK_DOWN, SDLK_LEFT, SDLK_RIGHT });

	// load ROM from command line
	if ((argc > 1) && loadFile(argv[1])) {
		power();
	}

	// run emulator
	while(true) {
		pollEvents();

		if (!paused) {
			CPU::runFrame();
			zapper.update();
			total_frames++;
		}

		SDL_UpdateTexture(sdl_texture, NULL, screen, SCREEN_WIDTH * sizeof (Pixel));
		SDL_RenderClear(sdl_renderer);
		SDL_RenderCopy(sdl_renderer, sdl_texture, NULL, NULL);
		SDL_RenderPresent(sdl_renderer);
	}

	return 0;
}
