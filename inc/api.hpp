#ifndef API_HPP
#define API_HPP

#include <string>

#ifdef _WIN32
	#include <windows.h>
	#include <commdlg.h>
#endif

namespace API {
	enum Flags {
		NONE = 0,
#ifdef _WIN32
		FILE_MUST_EXIST = OFN_FILEMUSTEXIST
#endif
	};
	std::string chooseOpenFile(const char* title, const char* filter, const char* directory = NULL);
	std::string chooseSaveFile(const char* title, const char* filter, const char* directory = NULL);
	bool createDirectory(std::string name);
	bool directoryExists(std::string name);
	bool fileExists(std::string filename);
}

#endif