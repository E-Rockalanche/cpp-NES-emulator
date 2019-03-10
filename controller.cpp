#include "controller.hpp"

Controller::Controller() {
	for(int n = 0; n < 8; n++) {
		buttons[n] = false;
	}
	strobe = false;
}

bool Controller::read() {
	if (strobe) {
		current_button = 0;
	}
	return buttons[current_button++ % 8];
}

void Controller::write(Byte value) {
	strobe = value & 0x01;
	if (strobe) {
		current_button = 0;
	}
}

void Controller::reset() {
	for(int n = 0; n < 8; n++) {
		buttons[n] = false;
	}
}