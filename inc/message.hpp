#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <string>

#ifdef _WIN32
	#include <windows.h>
#endif

enum MessageBoxFlag {
	NONE = 0,

#ifdef _WIN32
	OK_OPTION = MB_OK,
	YESNO_OPTION = MB_YESNO,
	ERROR_ICON = MB_ICONERROR,
	WARNING_ICON = MB_ICONWARNING
#else
	OK_OPTION,
	YESNO_OPTION,
	ERROR_ICON,
	WARNING_ICON
#endif

};

void showMessage(const std::string& title, const std::string& message, int flags = NONE);
void showError(const std::string& title, const std::string& message, int flags = NONE);
bool askYesNo(const std::string& title, const std::string& message, int flags = NONE);

#endif