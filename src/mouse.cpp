#include "mouse.hpp"
#include "assert.hpp"

namespace Mouse {

enum ButtonState {
	STATE_UP,
	STATE_PRESSED,
	STATE_HELD,
	STATE_RELEASED
};

int mouse_x = 0;
int mouse_y = 0;
int wheel_scroll = 0;
ButtonState buttons[NUM_BUTTONS] = { STATE_UP, STATE_UP, STATE_UP };

int getX() { return mouse_x; }
int getY(){ return mouse_y; }

bool buttonPressed(Button button) {
	return buttons[button] == STATE_PRESSED;
}

bool buttonHeld(Button button) {
	return (buttons[button] == STATE_PRESSED)
		|| (buttons[button] == STATE_HELD);
}

bool buttonReleased(Button button) {
	return buttons[button] == STATE_RELEASED;
}

void update() {
	wheel_scroll = 0;
	for(int i = 0; i < NUM_BUTTONS; i++) {
		ButtonState state = buttons[i];
		switch(state) {
			case STATE_PRESSED: state = STATE_HELD; break;
			case STATE_RELEASED: state = STATE_UP; break;
			default: break;
		}
		buttons[i] = state;
	}
}

int getWheel() { return wheel_scroll; }

void setPos(int x, int y) {
	mouse_x = x;
	mouse_y = y;
}

void pressButton(Button button) {
	assert(buttons[button] == STATE_UP, "button cannot be pressed");
	buttons[button] = STATE_PRESSED;
}

void releaseButton(Button button) {
	assert(buttons[button] == STATE_HELD, "button cannot be released");
	buttons[button] = STATE_RELEASED;
}

void setWheel(int scroll) {
	wheel_scroll = scroll;
}

};