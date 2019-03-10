#ifndef ZAPPER_HPP
#define ZAPPER_HPP

#include "controller.hpp"

class Zapper : public Controller {
public:
	Zapper(const Pixel* screen = NULL);
	~Zapper() {}
	void setScreen(const Pixel* screen);
	Byte read();
	void write(Byte value) {}
	void pull();
	void release();
	bool detectingLight();
	void reset();
	void aim(int x, int y);

private:
	bool trigger_held;
	int x, y;
	const Pixel* screen;
};

#endif