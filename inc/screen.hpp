#ifndef SCREEN_HPP
#define SCREEN_HPP

#include "pixel.hpp"

#define SCREEN_WIDTH 256
#define SCREEN_HEIGHT 240

void clearScreen(Pixel colour = Pixel(0));

extern Pixel screen[SCREEN_WIDTH * SCREEN_HEIGHT];

#endif