#include <fstream>

#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"

#include "hotkeys.hpp"
#include "globals.hpp"
#include "movie.hpp"
#include "api.hpp"
#include "apu.hpp"
#include "cartridge.hpp"

void quit() { exit(0); }

void setFullscreen(bool on) {
	fullscreen = on;
	int flags = fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;
	SDL_SetWindowFullscreen(window, flags);
}
void toggleFullscreen() {
	setFullscreen(!fullscreen);
}

void setResolutionScale(float scale) {
	setFullscreen(false);
	resizeWindow(crop_area.w * scale, crop_area.h * scale);
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

void setPaused(bool pause) {
	paused = pause;
}
void togglePaused() {
	setPaused(!paused);
}

void stepFrame() {
	step_frame = true;
}

void setMute(bool mute) {
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

void takeScreenshot() {
	SDL_Surface* surface = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT,
		24, Pixel::r_mask, Pixel::g_mask, Pixel::b_mask, Pixel::a_mask);
	surface->pixels = screen;

	int timestamp = std::time(NULL);
	std::string name = "screenshot_" + std::to_string(timestamp) + ".png";
	fs::path filename = screenshot_folder / name;
    IMG_SavePNG(surface, filename.c_str());
}

void saveState() {
	fs::path filename = savestate_folder
		/ rom_filename.filename().replace_extension(savestate_ext);

	std::ofstream fout(filename.c_str(), std::ios::binary);
	if (fout.is_open()) {
		CPU::saveState(fout);
		PPU::saveState(fout);
		APU::saveState(fout);
		cartridge->saveState(fout);
		fout.close();
	}
}

void loadState() {
	fs::path filename = savestate_folder
		/ rom_filename.filename().replace_extension(savestate_ext);
	
	std::ifstream fin(filename.c_str(), std::ios::binary);
	if (fin.is_open()) {
		CPU::loadState(fin);
		PPU::loadState(fin);
		APU::loadState(fin);
		cartridge->loadState(fin);
		fin.close();
	}
}

std::vector<Hotkey> hotkeys = {
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

void pressHotkey(SDL_Keycode key) {
	for(int i = 0; i < (int)hotkeys.size(); i++) {
		if (key == hotkeys[i].key) {
			(*hotkeys[i].callback)();
		}
	}
}