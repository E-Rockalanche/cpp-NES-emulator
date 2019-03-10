#ifndef ZAPPER_HPP
#define ZAPPER_HPP

#include "controller.hpp"

class Zapper : public Controller {
public:
	Zapper() {}
	~Zapper() {}
	void pull();
	void release();
	void setLight(bool light);
};

#endif