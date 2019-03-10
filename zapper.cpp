#include "zapper.hpp"

void Zapper::pull() {
	buttons[4] = true;
}

void Zapper::release() {
	buttons[4] = false;
}

void Zapper::setLight(bool light) {
	buttons[3] = light;
}