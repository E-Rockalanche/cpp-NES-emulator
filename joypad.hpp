#ifndef JOYPAD_HPP
#define JOYPAD_HPP

#include "common.hpp"
#include "controller.hpp"

class Joypad : public Controller{
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

	Joypad();
	~Joypad() {}
	void pressKey(int key);
	void releaseKey(int key);
	Byte read();
	void write(Byte value);
	void reset();

private:
	static const char* button_names[NUM_BUTTONS];
	int current_button;
	bool buttons[NUM_BUTTONS];
	bool strobe;
};

#endif