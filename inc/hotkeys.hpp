#ifndef HOTKEYS_HPP
#define HOTKEYS_HPP

#include <vector>
#include "SDL.h"
#include "common.hpp"

typedef void(*Callback)(void);
struct Hotkey {
	SDL_Keycode key;
	Callback callback;
};

extern std::vector<Hotkey> hotkeys;

void quit();

void power();
void reset();

void toggleSpriteFlickering();

void setFullscreen(bool on = true);
void toggleFullscreen();

void setResolutionScale(float scale);
void setResolutionScale1();
void setResolutionScale2();
void setResolutionScale3();

void setPaused(bool pause = true);
void togglePaused();

void stepFrame();

void setMute(bool mute = true);
void toggleMute();

void selectRom();

void closeFile();

void toggleRecording();
void togglePlayback();

void saveMovie();
void loadMovie();

void takeScreenshot();

void saveState();
void saveState(const std::string& filename);
void loadState();
void loadState(const std::string& filename);

void pressHotkey(SDL_Keycode key);

#endif