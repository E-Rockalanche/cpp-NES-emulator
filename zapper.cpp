#include "zapper.hpp"
#include "debugging.hpp"

Zapper::Zapper(const Pixel* screen) {
	this->screen = screen;
}

void Zapper::setScreen(const Pixel* screen) {
	this->screen = screen;
}

void Zapper::aim(int x, int y) {
	this->x = x;
	this->y = y;
}

void Zapper::pull() {
	trigger_held = true;
}

void Zapper::release() {
	trigger_held = false;
}

bool Zapper::detectingLight() {
	bool light = false;
	if (x >= 0 && x < 256 && y >= 0 && y < 240) {
		Pixel p = screen[x + (239 - y) * 256];
		light = (p.red >= 0xf8) && (p.blue >= 0xf8) && (p.green >= 0xf8);
		if (light) {
			dout("detected light");
		}
	}
	return light;
}

Byte Zapper::read() {
	return (trigger_held * 0x10) | (detectingLight() * 0x08);
}

void Zapper::reset() {
	trigger_held = false;
}