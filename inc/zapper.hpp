#ifndef ZAPPER_HPP
#define ZAPPER_HPP

#include "controller.hpp"

class Zapper : public Controller {
public:
	Zapper();
	~Zapper() {}
	Byte read();
	void write(Byte value) {}
	void pull();
	void update();
	void release();
	bool detectingLight();
	void reset();
	void aim(int x, int y);

private:
	int trigger_held;
	int x, y;
};

#endif