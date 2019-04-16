#ifndef PIXEL_HPP
#define PIXEL_HPP

#include "common.hpp"
#include "assert.hpp"

struct Pixel {
	// big endian
	Byte red;
	Byte green;
	Byte blue;

	Pixel(Byte red, Byte green, Byte blue)
		: red(red), green(green), blue(blue) {}
		
	Pixel(int rgb = 0) : red((rgb >> 16) & 0xff),
						  green((rgb >> 8) & 0xff),
						  blue(rgb & 0xff) {}

	operator int() {
		return (red << 16) | (green << 8) | (blue << 0);
	}

	Byte& operator [](unsigned int index) {
		switch(index % 3) {
			case 0: return red;
			case 1: return green;
			case 2: return blue;
			default: assert(false, "invalid pixel index");
		}
	}

	static const int r_mask = 0x0000ff;
	static const int g_mask = 0x00ff00;
	static const int b_mask = 0xff0000;
	static const int a_mask = 0;
};

#endif