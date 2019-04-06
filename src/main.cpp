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
#include "gui.hpp"
#include "keyboard.hpp"
#include "globals.hpp"
#include "api.hpp"
#include "movie.hpp"
#include "assert.hpp"

#define GUI_HEIGHT 24

// GUI
Gui::HBox top_gui({ 0, 0, window_width, GUI_HEIGHT });
Gui::Element* active_menu = &top_gui;

// SDL
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

// NES
Zapper zapper;
bool paused = false;
bool step_frame = false;
bool in_menu = false;
bool muted = false;

const int SCREEN_BPP = 24; // bits per pixel
Pixel screen[SCREEN_WIDTH * SCREEN_HEIGHT];

// paths
fs::path rom_filename;
fs::path save_filename;
fs::path save_folder;
fs::path rom_folder;
fs::path screenshot_folder;
fs::path movie_folder;
std::string save_ext;
std::string movie_ext;

// window size
int window_width;
int window_height;
bool fullscreen;
float render_scale;
SDL_Rect render_area = { 0, GUI_HEIGHT, 256, 240 };

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

void reset() {
	if (cartridge != NULL) {
		clearScreen();
		CPU::reset();
		PPU::reset();
		APU::reset();
		resetFrameNumber();
	}
}

void power() {
	if (cartridge != NULL) {
		clearScreen();
		CPU::power();
		PPU::power();
		APU::reset();
		resetFrameNumber();
	}
}

bool loadFile(std::string filename) {
	if (cartridge) delete cartridge;
	cartridge = Cartridge::loadFile(filename);
	if (!cartridge) {
		dout("could not load " << filename);
		return false;
	} else {
		rom_filename = filename;
		save_filename = save_folder / rom_filename.filename().replace_extension(save_ext);
		if (cartridge->hasSRAM()) {
			cartridge->loadSave(save_filename.string());
		}
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
		cartridge->saveGame(save_filename.string());
	}
}

// resize window and render area
void resizeRenderArea(bool round_scale = false) {
	SDL_GetWindowSize(window, &window_width, &window_height);
	top_gui.setSize(window_width, GUI_HEIGHT);

	// set viewport to fill window
	SDL_RenderSetViewport(renderer, NULL);

	int gui_height = fullscreen ? 0 : GUI_HEIGHT;

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
	SDL_SetWindowSize(window, width, height + GUI_HEIGHT);
	resizeRenderArea();
}

// hotkeys and button callbacks
void quit() { exit(0); }

void setFullscreen(bool on = true) {
	fullscreen = on;
	int flags = fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;
	SDL_SetWindowFullscreen(window, flags);
}
void toggleFullscreen() {
	setFullscreen(!fullscreen);
}

void setResolutionScale(float scale) {
	setFullscreen(false);
	resizeWindow(SCREEN_WIDTH * scale, SCREEN_HEIGHT * scale);
}
void setResolutionScale1() {
	setResolutionScale(1);
}
void setResolutionScale2() {
	setResolutionScale(2);
}
void setResolutionScale3() {
	setResolutionScale(3);
}

void setPaused(bool pause = true) {
	paused = pause;
}
void togglePaused() {
	setPaused(!paused);
}

void stepFrame() {
	step_frame = true;
}

void setMute(bool mute = true) {
	muted = mute;
	APU::mute(muted);
}
void toggleMute() {
	setMute(!muted);
}

void selectRom() {
	API::FileDialog dialog("Select ROM");
	dialog.setFilter("NES ROM\0*.nes\0Any file\0*.*\0");
	dialog.setDirectory(rom_folder.c_str());
	std::string filename = dialog.getOpenFileName();

	if ((filename.size() > 0) && loadFile(filename)) {
		power();
	}
}

void closeFile() {
	saveGame();
	delete cartridge;
	cartridge = NULL;
	rom_filename = "";
	save_filename = "";
	clearScreen();
}

void toggleRecording() {
	switch(Movie::getState()) {
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

		default: break;
	}
}

void togglePlayback() {
	switch(Movie::getState()) {
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

		default: break;
	}
}

void saveMovie() {
	API::FileDialog dialog("Save Movie");
	dialog.setFilter("NES Movie\0*.nesmov\0");
	dialog.setDirectory(movie_folder.c_str());
	std::string filename = dialog.getSaveFileName() + movie_ext;

	if (!Movie::save(filename)) {
		dout("failed to save movie");
	}
}

void loadMovie() {
	API::FileDialog dialog("Open Movie");
	dialog.setFilter("NES Movie\0*.nesmov\0");
	dialog.setDirectory(movie_folder.c_str());
	std::string filename = dialog.getOpenFileName();

	if (Movie::load(filename)) {
		power();
		Movie::startPlayback();
	} else {
		dout("failed to save movie");
	}
}

typedef void(*Callback)(void);
struct Hotkey {
	enum Type {
		QUIT,
		FULLSCREEN,
		MUTE,
		PAUSE,
		STEP_FRAME,
		RECORD,
		PLAYBACK,

		NUM_HOTKEYS
	};
	SDL_Keycode key;
	Callback callback;
};
Hotkey hotkeys[Hotkey::NUM_HOTKEYS] = {
	{ SDLK_ESCAPE, quit },
	{ SDLK_F11, toggleFullscreen},
	{ SDLK_m, toggleMute},
	{ SDLK_p, togglePaused},
	{ SDLK_s, stepFrame},
	{ SDLK_r, toggleRecording},
	{ SDLK_a, togglePlayback}
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

void keyboardEvent(const SDL_Event& event) {
	SDL_Keycode key = event.key.keysym.sym;
	bool pressed = event.key.state == SDL_PRESSED;
	if (pressed) {
		if (key == SDLK_f) {
			dout("fps: " << ave_fps);
			dout("real fps: " << ave_real_fps);
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
				int gui_height = fullscreen ? 0 : GUI_HEIGHT;
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
		window_width, window_height + GUI_HEIGHT,
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
	const SDL_Rect gui_button_rect = { 0, 0, 0, GUI_HEIGHT };
	Gui::DropDown file_dropdown(gui_button_rect, "File");
	Gui::DropDown view_dropdown(gui_button_rect, "View");
	Gui::DropDown options_dropdown(gui_button_rect, "Options");
	Gui::DropDown machine_dropdown(gui_button_rect, "Machine");
	Gui::DynamicTextElement fps_display({ 0, 0, 64, GUI_HEIGHT }, &fps_text);
	top_gui.addElement(file_dropdown);
	top_gui.addElement(view_dropdown);
	top_gui.addElement(options_dropdown);
	top_gui.addElement(machine_dropdown);
	top_gui.addElement(fps_display);

	// file
	Gui::Button load_rom_button(gui_button_rect, "Load", selectRom);
	Gui::Button close_rom_button(gui_button_rect, "Close", closeFile);
	Gui::SubDropDown movie_dropdown(gui_button_rect, "Movie >");
	Gui::Button save_movie_button(gui_button_rect, "Save", saveMovie);
	Gui::Button load_movie_button(gui_button_rect, "Load", loadMovie);
	movie_dropdown.addElement(save_movie_button);
	movie_dropdown.addElement(load_movie_button);
	file_dropdown.addElement(load_rom_button);
	file_dropdown.addElement(close_rom_button);
	file_dropdown.addElement(movie_dropdown);

	// view
	Gui::RadioButton fullscreen_button(gui_button_rect, "Fullscreen", &fullscreen, setFullscreen);
	Gui::Button scale1_button(gui_button_rect, "scale: 1", setResolutionScale1);
	Gui::Button scale2_button(gui_button_rect, "scale: 2", setResolutionScale2);
	Gui::Button scale3_button(gui_button_rect, "scale: 3", setResolutionScale3);
	view_dropdown.addElement(fullscreen_button);
	view_dropdown.addElement(scale1_button);
	view_dropdown.addElement(scale2_button);
	view_dropdown.addElement(scale3_button);

	// machine
	Gui::Button power_button(gui_button_rect, "Power", &power);
	Gui::Button reset_button(gui_button_rect, "Reset", &reset);
	Gui::SubDropDown nes_dropdown(gui_button_rect, "Options >");
	Gui::RadioButton flicker_button(gui_button_rect, "Sprite Flickering", &PPU::sprite_flickering);
	nes_dropdown.addElement(flicker_button);
	machine_dropdown.addElement(power_button);
	machine_dropdown.addElement(reset_button);
	machine_dropdown.addElement(nes_dropdown);


	Gui::RadioButton pause_button(gui_button_rect, "Pause", &paused);
	Gui::RadioButton mute_button(gui_button_rect, "Mute", &muted);
	options_dropdown.addElement(pause_button);
	options_dropdown.addElement(mute_button);

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
			frame_number++;
			double elapsed = SDL_GetTicks() - last_time;
			total_real_fps += 1000.0/elapsed;
		}
		step_frame = false;

		// clear the screen
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // black
		SDL_RenderClear(renderer);

		// render nes & gui
		SDL_UpdateTexture(nes_texture, NULL, screen, SCREEN_WIDTH * sizeof (Pixel));
		SDL_RenderCopy(renderer, nes_texture, NULL, &render_area);
		if (!fullscreen || paused || (cartridge == NULL)) {
			top_gui.render();
		}

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
