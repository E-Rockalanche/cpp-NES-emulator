#include "joypad.hpp"

Joypad joypad[4];

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

const char* Joypad::getButtonName(Joypad::Button button) {
	return button_names[button];
}

Joypad::Joypad() {
	for(int n = 0; n < NUM_BUTTONS; n++) {
		buttons[n] = false;
	}
	strobe = false;
}

Byte Joypad::read() {
	if (strobe) {
		current_button = 0;
	}
	return buttons[current_button++ % NUM_BUTTONS];
}

void Joypad::write(Byte value) {
	strobe = value & 0x01;
	if (strobe) {
		current_button = 0;
	}
}

void Joypad::reset() {
	for(int n = 0; n < NUM_BUTTONS; n++) {
		buttons[n] = false;
	}
}

void Joypad::pressButton(Joypad::Button button) {
	buttons[button] = true;
}

void Joypad::releaseButton(Joypad::Button button) {
	buttons[button] = false;
}

void Joypad::pressKey(int key) {
	for(int n = 0; n < NUM_BUTTONS; n++) {
		if (keymap[n] == key) {
			buttons[n] = true;
		}
	}
}

void Joypad::releaseKey(int key) {
	for(int n = 0; n < NUM_BUTTONS; n++) {
		if (keymap[n] == key) {
			buttons[n] = false;
		}
	}
}

void Joypad::mapButtons(const int keys[Joypad::NUM_BUTTONS]) {
	for(int n = 0; n < NUM_BUTTONS; n++) {
		keymap[n] = keys[n];
	}
}

void Joypad::mapButton(Joypad::Button button, int key) {
	keymap[button] = key;
}