#ifndef CONTROLLER_HPP
#define CONTROLLER_HPP

#include "common.hpp"

class Controller {
public:
	Controller() {};
	virtual ~Controller() {}
	virtual Byte read() = 0;
	virtual void write(Byte value) = 0;
	virtual void reset() = 0;
};

extern Controller* controller_ports[2];

#endif