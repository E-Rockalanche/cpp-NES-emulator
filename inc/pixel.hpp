#ifndef PIXEL_HPP
#define PIXEL_HPP

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
};

#endif