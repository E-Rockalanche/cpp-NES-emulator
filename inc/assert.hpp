#include <iostream>

#undef assert
#define assert(condition, message) \
	if (!(condition)) { \
		std::cout << "ERROR: " << __FILE__ << " (" << __LINE__ << "): " << message << '\n'; \
		exit(0); \
	}

#undef assertBounds
#define assertBounds(index, size) \
	assert((index) < (size), "index " << (index) << " out of bounds " << (size))
