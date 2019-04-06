#ifndef PIXEL_HPP
#define PIXEL_HPP

#include "common.hpp"
#include "assert.hpp"

struct Pixel {
	Byte red;
	Byte green;
	Byte blue;

	Pixel(Byte red, Byte green, Byte blue)
		: red(red), green(green), blue(blue) {}
		
	Pixel(int rgb = 0) : red((rgb >> 16) & 0xff),
						  green((rgb >> 8) & 0xff),
						  blue(rgb & 0xff) {}

	operator int() {
		return red << 16 | green << 8 | blue;
	}

	Byte& operator [](unsigned int index) {
		switch(index % 3) {
			case 0: return red;
			case 1: return green;
			case 2: return blue;
			default: assert(false, "invalid pixel index");
		}
	}
};

#endif