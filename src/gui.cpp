#include "gui.hpp"

#include <string>
#include <vector>

#ifdef _WIN32
	#include <windows.h>
	#include "SDL2/SDL_syswm.h"
#endif

#include "SDL2/SDL.h"

#include "assert.hpp"

#ifdef _WIN32
	std::vector<GUI::VoidCallback> void_callbacks;
#endif

namespace GUI {
	void handleMenuEvent(const SDL_Event& event) {
	#ifdef _WIN32
	    if (event.syswm.msg->msg.win.msg == WM_COMMAND) {
	    	UINT command_id = LOWORD(event.syswm.msg->msg.win.wParam);
	    	if (command_id < void_callbacks.size()) {
	    		VoidCallback callback = void_callbacks[command_id];
	    		(*callback)();
	    	}
	    }
	#endif
	}

	Element::Element() {
	#ifdef _WIN32
		flags = 0;
		id = 0;
		menu_handle = CreateMenu();
	#endif
	}

	TextElement::TextElement(std::string name) : Element(), name(name) {
	#ifdef _WIN32
		flags = MF_STRING;
	#endif
	}

	void TextElement::setName(std::string name) {
		this->name = name;
	}

	Button::Button(std::string name, VoidCallback callback)
		: TextElement(name), callback(callback) {
	#ifdef _WIN32
		flags = MF_STRING;
		id = void_callbacks.size();
		void_callbacks.push_back(callback);
	#endif
	}

	void Button::setCallback(VoidCallback callback) {
		this->callback = callback;
	#ifdef _WIN32
		void_callbacks[id] = callback;
	#endif
	}

	Menu::Menu(std::string name) : TextElement(name) {
	#ifdef _WIN32
		flags = MF_POPUP;
		id = (UINT_PTR)menu_handle;
	#endif
	}

	void Menu::append(TextElement& element) {
	#ifdef _WIN32
		AppendMenu(menu_handle,
			element.getFlags(),
			element.getId(),
			element.getName().c_str());
	#endif
	}

	void Menu::setMenuBar(SDL_Window* window) {
		assert(window != NULL, "Menu::setMenuBar() SDL window is NULL");

		SDL_SysWMinfo infoWindow;
		SDL_VERSION(&infoWindow.version);
		bool ok = SDL_GetWindowWMInfo(window, &infoWindow);
		assert(ok, "Could not get window info");

		HWND window_handler = (infoWindow.info.win.window);
		SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);

		SetMenu(window_handler, menu_handle);
	}

} // end namespace GUI