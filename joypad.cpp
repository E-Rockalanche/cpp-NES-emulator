#include "joypad.hpp"

const char* Joypad::button_names[] = {
	"A",
	"B",
	"select",
	"start",
	"up",
	"down",
	"left",
	"right"
};

void Joypad::pressKey(int key) {
	for(int n = 0; n < 8; n++) {
		if (keymap[n] == key) {
			buttons[n] = true;
		}
	}
}

void Joypad::releaseKey(int key) {
	for(int n = 0; n < 8; n++) {
		if (keymap[n] == key) {
			buttons[n] = false;
		}
	}
}