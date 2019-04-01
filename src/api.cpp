#include <string>
#include "api.hpp"
#include "common.hpp"

#ifdef WIN32
	#include <windows.h>
	#include <commdlg.h>
#endif

namespace API {

std::string getFilename(const char* title, const char* filter) {
	std::string filename = "";

#ifdef WIN32
	char filename_buffer[MAX_PATH];
	ZeroMemory(filename_buffer, MAX_PATH);

	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL;
	ofn.lpstrFilter = filter;
	ofn.lpstrFile = filename_buffer;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrTitle = title;
	ofn.Flags = OFN_EXPLORER | OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST
		| OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

	if (GetOpenFileNameA(&ofn)) {
		filename = filename_buffer;
	}
#endif

	return filename;
}

bool createDirectory(std::string name) {
	bool ok = false;

#ifdef WIN32
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

#ifdef WIN32
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

#ifdef WIN32
	ok = true;
	if(INVALID_FILE_ATTRIBUTES == GetFileAttributes(filename.c_str())
			&& GetLastError() == ERROR_FILE_NOT_FOUND) {
	    ok = false;
	}
#endif

	return ok;
}

} // end namespace