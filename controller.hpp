#ifndef CONTROLLER_HPP
#define CONTROLLER_HPP

#include "common.hpp"

class Controller {
public:
	Controller();
	virtual ~Controller() {}
	bool read();
	void write(Byte value);
	void reset();

protected:
	int current_button;
	bool buttons[8];
	bool strobe;
};

#endif