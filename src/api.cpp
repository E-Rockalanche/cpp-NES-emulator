
#include "api.hpp"

#include "debug.hpp"
#include "globals.hpp"
#include "common.hpp"

#include <string>
#include <cstring>

#ifdef _WIN32
	#include <windows.h>
	#include <commdlg.h>
	#include "SDL2/SDL_syswm.h"
#endif

#include "SDL2/SDL.h"

namespace
{
	template <typename MASK, typename FLAG>
	bool testFlag( MASK mask, FLAG flag )
	{
		return mask & flag;
	}
}


namespace API {

	FileDialog::FileDialog() {
		clear();
	}

	FileDialog::FileDialog(const char* title) {
		clear();
		setTitle(title);
	}

	FileDialog::~FileDialog() {}

	FileDialog& FileDialog::setTitle(const char* title) {
		ofn.lpstrTitle = title;
		return *this;
	}

	FileDialog& FileDialog::setFilename(const char* filename) {
		std::strcpy(ofn.lpstrFile, filename);
		return *this;
	}

	FileDialog& FileDialog::setFilter(const char* filter) {
		ofn.lpstrFilter = filter;
		return *this;
	}

	FileDialog& FileDialog::setDirectory(const char* directory) {
		ofn.lpstrInitialDir = directory;
		return *this;
	}

	std::string FileDialog::getOpenFileName() {
		std::string filename;
		if (GetOpenFileNameA(&ofn)) {
			filename = filename_buffer;
		}
		return filename;
	}

	std::string FileDialog::getSaveFileName() {
		std::string filename;
		if (GetSaveFileNameA(&ofn)) {
			filename = filename_buffer;
		}
		return filename;
	}

	FileDialog& FileDialog::clear() {
		ZeroMemory(filename_buffer, MAX_PATH);
		ZeroMemory(&ofn, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = NULL;
		ofn.lpstrFile = filename_buffer;
		ofn.nMaxFile = MAX_PATH;
		ofn.Flags = OFN_EXPLORER | OFN_DONTADDTORECENT
			| OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

		return *this;
	}

	bool createDirectory(std::string name) {
		bool ok = false;
		
	#ifdef _WIN32
		if (CreateDirectory(name.c_str(), NULL)) {
		    ok = true;
		} else if (ERROR_ALREADY_EXISTS != GetLastError()) {
			dbLogError( "could not create directory: %s", name.c_str() );
		}
	#endif

		return ok;
	}

	bool directoryExists(std::string name) {
		bool ok = false;

	#ifdef _WIN32
		DWORD ftyp = GetFileAttributesA(name.c_str());
		if (ftyp != INVALID_FILE_ATTRIBUTES
				&& testFlag(ftyp, FILE_ATTRIBUTE_DIRECTORY)) {
			ok = true;
		}
	#endif

		return ok;
	}

	bool fileExists(std::string filename) {
		bool ok = false;

	#ifdef _WIN32
		ok = true;
		if(INVALID_FILE_ATTRIBUTES == GetFileAttributes(filename.c_str())
				&& GetLastError() == ERROR_FILE_NOT_FOUND) {
		    ok = false;
		}
	#endif

		return ok;
	}

#ifdef _WIN32
	HWND getWindowHandle() {
		SDL_SysWMinfo infoWindow;
		SDL_VERSION(&infoWindow.version);
		bool ok = SDL_GetWindowWMInfo(window, &infoWindow);
		dbAssertMessage(ok, "Could not get window info");

		return infoWindow.info.win.window;
	}
#endif

} // end API namespace