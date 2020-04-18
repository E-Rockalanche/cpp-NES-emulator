#include "SDL.h"

#include "message.hpp"

void showMessage(const std::string& title, const std::string& message) {
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, title.c_str(), message.c_str(), NULL);
}

void showError(const std::string& title, const std::string& message) {
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title.c_str(), message.c_str(), NULL);
}

bool askYesNo(const std::string& title, const std::string& message, unsigned int flags) {
	static const int NUM_BUTTONS = 2;
	static const int YES_ID = 1;
	static const int NO_ID = 0;
	const SDL_MessageBoxButtonData buttons[] = {
		{ SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, NO_ID, "No" },
		{ 0, YES_ID, "Yes" }
	};
	const SDL_MessageBoxData mb_data = {
		flags,
		NULL,
		title.c_str(),
		message.c_str(),
		NUM_BUTTONS,
		buttons,
		NULL
	};
	int button_id;
	if (SDL_ShowMessageBox(&mb_data, &button_id) >= 0) {
		return button_id == 1;
	} else {
		return 0;
	}
}