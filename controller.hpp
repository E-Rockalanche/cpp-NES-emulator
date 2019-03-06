#ifndef CONTROLLER_HPP
#define CONTROLLER_HPP

#include "common.hpp"

class Controller {
public:
	enum Button {
		A,
		B,
		SELECT,
		START,
		UP,
		DOWN,
		LEFT,
		RIGHT,

		NUM_BUTTONS
	};

	int keymap[NUM_BUTTONS];

	Controller();
	bool read();
	void write(Byte value);
	void pressKey(int key);
	void resetButtons();

private:
	static const char* key_names[NUM_BUTTONS];

	bool strobe = false;
	int current_button = 0;
	bool buttons[NUM_BUTTONS];
};

#endif