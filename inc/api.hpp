#ifndef API_HPP
#define API_HPP

#include <string>

#ifdef _WIN32
	#include <windows.h>
	#include <commdlg.h>
#endif

namespace API {
	
	class FileDialog {
	public:
		FileDialog();
		FileDialog(const char* title);
		~FileDialog();

		FileDialog& setTitle(const char* title);
		FileDialog& setFilter(const char* title);
		FileDialog& setDirectory(const char* title);

		std::string getOpenFileName();
		std::string getSaveFileName();

		FileDialog& clear();

	private:
		OPENFILENAME ofn;
		char filename_buffer[MAX_PATH];
	};

	bool createDirectory(std::string name);
	bool directoryExists(std::string name);
	bool fileExists(std::string filename);
}

#endif