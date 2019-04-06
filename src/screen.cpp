#include "screen.hpp"

void clearScreen(Pixel colour) {
	for(int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
		screen[i] = colour;
	}
}