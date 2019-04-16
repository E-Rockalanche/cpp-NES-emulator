#ifdef _WIN32
	#include <windows.h>
#endif

#include "message.hpp"
#include "api.hpp"
#include "debugging.hpp"

#ifdef _WIN32
	#define YES_BUTTON IDYES
#endif

int createMessageBox(const std::string& title, const std::string& message, int flags) {
#ifdef _WIN32
	return MessageBox(
		API::getWindowHandle(),
		message.c_str(),
		title.c_str(),
		flags
	);
#endif
}

void showMessage(const std::string& title, const std::string& message, int flags) {
	createMessageBox(title, message, flags | OK_OPTION);
}

void showError(const std::string& title, const std::string& message, int flags) {
	createMessageBox(title, message, flags | OK_OPTION | ERROR_ICON);
}

bool askYesNo(const std::string& title, const std::string& message, int flags) {
	return YES_BUTTON == createMessageBox(title, message, flags | YESNO_OPTION);
}