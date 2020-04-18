#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <string>
#include "SDL.h"

#define ERROR_ICON SDL_MESSAGEBOX_ERROR
#define WARNING_ICON SDL_MESSAGEBOX_WARNING
#define INFORMATION_ICON SDL_MESSAGEBOX_INFORMATION

void showMessage(const std::string& title, const std::string& message);
void showError(const std::string& title, const std::string& message);
bool askYesNo(const std::string& title, const std::string& message, unsigned int flags = 0);

#endif