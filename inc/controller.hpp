#ifndef CONTROLLER_HPP
#define CONTROLLER_HPP

#include "types.hpp"

namespace nes
{

	class Controller
	{
	public:

		virtual ~Controller() = default;

		virtual Byte read() = 0;

		virtual void write(Byte value) = 0;

		virtual void reset() = 0;
	};

}

#endif