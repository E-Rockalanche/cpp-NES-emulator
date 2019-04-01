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
		
	Pixel(unsigned int rgb = 0) : red((rgb >> 16) & 0xff),
						  green((rgb >> 8) & 0xff),
						  blue(rgb & 0xff) {}

	operator int() {
		return red << 16 | green << 8 | blue;
	}

	Byte operator [](int index) {
		switch(index) {
			case 0: return red;
			case 1: return green;
			case 2: return blue;
			default: assert(false, "invalid pixel index");
		}
		return 0;
	}
};

#endif