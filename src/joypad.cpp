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

void Joypad::setButtonState(Joypad::Button button, bool pressed) {
	buttons[button] = pressed;
}

void Joypad::pressButton(Joypad::Button button) {
	setButtonState(button, true);
}

void Joypad::releaseButton(Joypad::Button button) {
	setButtonState(button, false);
}

Joypad::Button Joypad::setKeyState(int key, bool pressed) {
	for(int n = 0; n < NUM_BUTTONS; n++) {
		if (keymap[n] == key) {
			buttons[n] = pressed;
			return static_cast<Button>(n);
		}
	}
	return NONE;
}

Joypad::Button Joypad::pressKey(int key) {
	return setKeyState(key, true);
}

Joypad::Button Joypad::releaseKey(int key) {
	return setKeyState(key, false);
}

void Joypad::mapButtons(const int keys[Joypad::NUM_BUTTONS]) {
	for(int n = 0; n < NUM_BUTTONS; n++) {
		keymap[n] = keys[n];
	}
}

void Joypad::mapButton(Joypad::Button button, int key) {
	keymap[button] = key;
}