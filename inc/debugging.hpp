#ifndef DEBUGGING_HPP
#define DEBUGGING_HPP

#define DEBUG true

#if (DEBUG)
	#define dout(message) std::cout << "DEBUG: " << __FILE__ << " (" << __LINE__ << "): " << message << '\n'
#else
	#define dout(message)
#endif

#endif