#include "zapper.hpp"
#include "debugging.hpp"
#include "screen.hpp"

Zapper::Zapper() {
	trigger_held = 0;
}

void Zapper::aim(int x, int y) {
	this->x = x;
	this->y = y;
}

void Zapper::pull() {
	// initiate fire
	if (trigger_held == 0) trigger_held++;
}

void Zapper::update() {
	// trigger releases after 2 frames (min amount required by games)
	if (trigger_held > 0)
		trigger_held = (trigger_held + 1) % 3;
}

bool Zapper::detectingLight() {
	bool light = false;
	if (x >= 0 && x < 256 && y >= 0 && y < 240) {
		Pixel p = screen[x + y * 256];
		light = (p.red >= 0xf8) && (p.blue >= 0xf8) && (p.green >= 0xf8);
	}
	return light;
}

Byte Zapper::read() {
	return ((trigger_held >= 1) ? 0x10 : 0) | (detectingLight() ? 0 : 0x08);
}

void Zapper::reset() {
	trigger_held = 0;
}