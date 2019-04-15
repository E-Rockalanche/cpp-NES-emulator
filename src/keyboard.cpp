#include <string>
#include <map>
#include <cctype>
#include "SDL2/SDL.h"

#define KEY(name) { #name, SDLK_##name }

SDL_Keycode getKeycode(std::string str) {
	static const std::map<std::string, SDL_Keycode> keymap = {
		KEY(BACKSPACE),
		KEY(TAB),
		KEY(RETURN),
		{ "ENTER", SDLK_RETURN },
		KEY(ESCAPE),
		KEY(SPACE),
		KEY(EXCLAIM),
		KEY(QUOTEDBL),
		{ "DOUBLEQUOTE", SDLK_QUOTEDBL },
		KEY(HASH),
		KEY(DOLLAR),
		KEY(PERCENT),
		KEY(AMPERSAND),
		KEY(QUOTE),
		{ "SINGLEQUOTE", SDLK_QUOTEDBL },
		KEY(LEFTPAREN),
		KEY(RIGHTPAREN),
		KEY(ASTERISK),
		KEY(PLUS),
		KEY(COMMA),
		KEY(MINUS),
		KEY(PERIOD),
		KEY(SLASH),
		KEY(0),
		KEY(1),
		KEY(2),
		KEY(3),
		KEY(4),
		KEY(5),
		KEY(6),
		KEY(7),
		KEY(8),
		KEY(9),
		KEY(COLON),
		KEY(SEMICOLON),
		KEY(LESS),
		KEY(EQUALS),
		KEY(GREATER),
		KEY(QUESTION),
		KEY(AT),
		KEY(LEFTBRACKET),
		KEY(BACKSLASH),
		KEY(RIGHTBRACKET),
		KEY(CARET),
		KEY(UNDERSCORE),
		KEY(BACKQUOTE),
		KEY(a),
		KEY(b),
		KEY(c),
		KEY(d),
		KEY(e),
		KEY(f),
		KEY(g),
		KEY(h),
		KEY(i),
		KEY(j),
		KEY(k),
		KEY(l),
		KEY(m),
		KEY(n),
		KEY(o),
		KEY(p),
		KEY(q),
		KEY(r),
		KEY(s),
		KEY(t),
		KEY(u),
		KEY(v),
		KEY(w),
		KEY(x),
		KEY(y),
		KEY(z),
		KEY(DELETE),
		KEY(CAPSLOCK),
		KEY(F1),
		KEY(F2),
		KEY(F3),
		KEY(F4),
		KEY(F5),
		KEY(F6),
		KEY(F7),
		KEY(F8),
		KEY(F9),
		KEY(F10),
		KEY(F11),
		KEY(F12),
		KEY(PRINTSCREEN),
		KEY(SCROLLLOCK),
		KEY(PAUSE),
		KEY(INSERT),
		KEY(HOME),
		KEY(PAGEUP),
		KEY(END),
		KEY(PAGEDOWN),
		KEY(RIGHT),
		KEY(LEFT),
		KEY(DOWN),
		KEY(UP),
		KEY(NUMLOCKCLEAR),
		KEY(KP_DIVIDE),
		KEY(KP_MULTIPLY),
		KEY(KP_MINUS),
		KEY(KP_PLUS),
		KEY(KP_ENTER),
		KEY(KP_1),
		KEY(KP_2),
		KEY(KP_3),
		KEY(KP_4),
		KEY(KP_5),
		KEY(KP_6),
		KEY(KP_7),
		KEY(KP_8),
		KEY(KP_9),
		KEY(KP_0),
		KEY(KP_PERIOD)
	};

	if (str.size() == 1) {
		// a-z
		str[0] = std::tolower(str[0]);
	} else {
		for(int i = 0; i < (int)str.size(); i++) {
			str[i] = std::toupper(str[i]);
		}
	}
	auto it = keymap.find(str);
	if (it != keymap.end()) {
		return it->second;
	} else {
		return 0;
	}
}

#undef KEY