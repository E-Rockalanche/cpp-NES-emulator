#ifndef API_HPP
#define API_HPP

#include <string>

namespace API {
	std::string getFilename(const char* title, const char* filter);
	bool createDirectory(std::string name);
	bool directoryExists(std::string name);
	bool fileExists(std::string filename);
}

#endif