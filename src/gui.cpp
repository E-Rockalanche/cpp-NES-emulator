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
		display_ptr = 0;
	#endif
	}

	TextElement::TextElement(const std::string& name) : Element() {
		setName(name);
	#ifdef _WIN32
		flags = MF_STRING;
	#endif
	}

	void TextElement::setName(const std::string& name) {
		this->name = name;
	#ifdef _WIN32
		display_ptr = this->name.c_str();
	#endif
	}

	Button::Button(const std::string& name, VoidCallback callback)
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

	MenuSeperator::MenuSeperator() {
	#ifdef _WIN32
		flags = MF_SEPARATOR;
	#endif
	}

	MenuBreak::MenuBreak() {
	#ifdef _WIN32
		flags = MF_MENUBARBREAK;
	#endif
	}

	Menu::Menu(const std::string& name) : TextElement(name) {
	#ifdef _WIN32
		flags = MF_POPUP;
		id = (UINT_PTR)menu_handle;
	#endif
	}

	void Menu::append(Element& element) {
	#ifdef _WIN32
		AppendMenu(menu_handle,
			element.getFlags(),
			element.getId(),
			element.getDisplay());
	#endif
	}

	void Menu::setMenuBar(SDL_Window* window) {
		assert(window != NULL, "Menu::setMenuBar() SDL window is NULL");

	#ifdef _WIN32
		SDL_SysWMinfo infoWindow;
		SDL_VERSION(&infoWindow.version);
		bool ok = SDL_GetWindowWMInfo(window, &infoWindow);
		assert(ok, "Could not get window info");

		window_handler = (infoWindow.info.win.window);
		SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);

		SetMenu(window_handler, menu_handle);
	#endif
	}

	void Menu::hide() {
	#ifdef _WIN32
		SetMenu(window_handler, 0);
	#endif
	}

	void Menu::show() {
	#ifdef _WIN32
		SetMenu(window_handler, menu_handle);
	#endif
	}

} // end namespace GUI