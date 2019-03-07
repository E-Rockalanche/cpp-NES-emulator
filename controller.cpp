#include "controller.hpp"
#include "debugging.hpp"

const char* Controller::key_names[] = {
	"A",
	"B",
	"select",
	"start",
	"up",
	"down",
	"left",
	"right"
};

Controller::Controller() {
	for(int n = 0; n < NUM_BUTTONS; n++) {
		buttons[n] = false;
	}
}

bool Controller::read() {
	return buttons[current_button++];
	if (strobe) {
		current_button = 0;
	}
}

void Controller::write(Byte value) {
	strobe = value & 0x01;
	if (strobe) {
		current_button = 0;
	}
}

void Controller::pressKey(int key) {
	for(int n = 0; n < 8; n++) {
		if (keymap[n] == key) {
			buttons[n] = true;
		}
	}
}

void Controller::releaseKey(int key) {
	for(int n = 0; n < 8; n++) {
		if (keymap[n] == key) {
			buttons[n] = false;
		}
	}
}

void Controller::resetButtons() {
	for(int n = 0; n < 8; n++) {
		buttons[n] = false;
	}
}