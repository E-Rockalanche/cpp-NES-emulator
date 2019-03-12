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

Joypad::Joypad() {
	for(int n = 0; n < 8; n++) {
		buttons[n] = false;
	}
	strobe = false;
}

Byte Joypad::read() {
	if (strobe) {
		current_button = 0;
	}
	return buttons[current_button++ % 8];
}

void Joypad::write(Byte value) {
	strobe = value & 0x01;
	if (strobe) {
		current_button = 0;
	}
}

void Joypad::reset() {
	for(int n = 0; n < 8; n++) {
		buttons[n] = false;
	}
}

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