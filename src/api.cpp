#include <string>
#include <cstring>

#ifdef _WIN32
	#include <windows.h>
	#include <commdlg.h>
#endif

#include "api.hpp"
#include "common.hpp"
#include "debugging.hpp"

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
		dout("set filename: " << filename);
		std::strcpy(ofn.lpstrFile, filename);
		dout("lpstrFile: " << ofn.lpstrFile);
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
			dout("Could not create directory");
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

} // end API namespace