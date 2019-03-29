#ifndef HOTKEYS_HPP
#define HOTKEYS_HPP

#include "globals.hpp"

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

#endif