

#include "menu_elements.hpp"
#include "api.hpp"
#include "debug.hpp"

#include <string>
#include <vector>

#ifdef _WIN32
	#include <windows.h>
	#include "SDL2/SDL_syswm.h"
#endif

#include "SDL2/SDL.h"

#define setFlag(mask) (flags) |= (mask)
#define clearFlag(mask) (flags) &= ~(mask)

namespace Menu {
	#ifdef _WIN32
		std::vector<VoidCallback> void_callbacks;
		HMENU menu_bar_handle = 0;
	#endif

	Seperator seperator;
	Break breaker;

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

	void Element::enable(bool e) {
	#ifdef _WIN32
		UINT enable_flag = e ? MF_ENABLED : MF_GRAYED;
		EnableMenuItem(menu_bar_handle, id, enable_flag);
		if (e) {
			clearFlag(MF_GRAYED);
		} else {
			setFlag(MF_GRAYED);
		}
	#endif
	}

	void Element::disable() {
		enable(false);
	}

	TextElement::TextElement() : TextElement("") {}

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

	void TextElement::operator=(const TextElement& other) {
		Element::operator=(static_cast<const Element&>(other));
		setName(other.name);
	}

	Button::Button() : Button("", NULL) {}

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

	void Button::operator=(const Button& other) {
		TextElement::operator=(static_cast<const TextElement&>(other));
		callback = other.callback;
	}

	Checkbox::Checkbox() : Button("", NULL) {
		uncheck();
	} 

	Checkbox::Checkbox(const std::string& name, VoidCallback callback, bool checked)
	: Button(name, callback) {
		check(checked);
	}

	void Checkbox::check(bool check) {
		int check_flag = check ? MF_CHECKED : MF_UNCHECKED;
		CheckMenuItem(menu_bar_handle, id, check_flag);
		if (check) {
			setFlag(MF_CHECKED);
		} else {
			clearFlag(MF_CHECKED);
		}
	}

	void Checkbox::uncheck() {
		check(false);
	}

	Seperator::Seperator() {
	#ifdef _WIN32
		flags = MF_SEPARATOR;
	#endif
	}

	Break::Break() {
	#ifdef _WIN32
		flags = MF_MENUBARBREAK;
	#endif
	}

	Menu::Menu() : Menu("") {}

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

	void Menu::seperateMenu() {
		append(seperator);
	}

	void Menu::breakMenu() {
		append(breaker);
	}

	void Menu::setMenuBar(SDL_Window* window) {
		dbAssertMessage(window != nullptr, "Menu::setMenuBar() SDL window is NULL");

	#ifdef _WIN32
		// enable windows menu bar events
		SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);

		// set this as menu bar
		HWND window_handle = API::getWindowHandle();
		SetMenu(window_handle, menu_handle);

		// let other elements know this is the menu bar
		menu_bar_handle = menu_handle;
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

	void Menu::operator=(const Menu& other) {
		TextElement::operator=(other);
		window_handler = other.window_handler;
	}

} // end namespace Menu