#ifndef JOYPAD_HPP
#define JOYPAD_HPP

#include "common.hpp"
#include "controller.hpp"

class Joypad : public Controller{
public:
	enum Button {
		NONE = -1,
		A = 0,
		B,
		SELECT,
		START,
		UP,
		DOWN,
		LEFT,
		RIGHT,

		NUM_BUTTONS
	};

	Joypad();
	~Joypad() {}
	void pressKey(int key);
	void releaseKey(int key);
	void pressButton(Button button);
	void releaseButton(Button button);
	Byte read();
	void write(Byte value);
	void reset();
	void mapButtons(const int keys[NUM_BUTTONS]);
	void mapButton(Button button, int key);

	static const char* getButtonName(Button button);

private:
	static const char* button_names[NUM_BUTTONS];

	int keymap[NUM_BUTTONS];
	int current_button;
	bool buttons[NUM_BUTTONS];
	bool strobe;
};

extern Joypad joypad[4];

#endif