#ifndef MOUSE_HPP
#define MOUSE_HPP

namespace Mouse {

enum Button {
	MB_NONE = -1,

	MB_LEFT = 0,
	MB_MIDDLE,
	MB_RIGHT,

	NUM_BUTTONS
};

int getX();
int getY();
bool buttonPressed(Button button);
bool buttonHeld(Button button);
bool buttonReleased(Button button);
int getWheel();

void update();
void setPos(int x, int y);
void pressButton(Button button);
void releaseButton(Button button);
void setWheel(int scroll);

};

#endif