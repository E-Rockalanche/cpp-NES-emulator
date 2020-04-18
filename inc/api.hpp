#ifndef API_HPP
#define API_HPP

#include <string>

#ifdef _WIN32
	#include <windows.h>
	#include <commdlg.h>
#endif

namespace API {
	typedef void(*VoidCallback)(void);

	bool createDirectory(std::string name);
	bool directoryExists(std::string name);
	bool fileExists(std::string filename);

#ifdef _WIN32
	HWND getWindowHandle();
#endif

	class FileDialog {
	public:
		FileDialog();
		FileDialog(const char* title);
		~FileDialog();

		FileDialog& setTitle(const char* title);
		FileDialog& setFilename(const char* title);
		FileDialog& setFilter(const char* title);
		FileDialog& setDirectory(const char* title);

		std::string getOpenFileName();
		std::string getSaveFileName();

		FileDialog& clear();

	private:
#ifdef _WIN32
		OPENFILENAMEA ofn;
		char filename_buffer[MAX_PATH];
#endif
	};
}

#endif