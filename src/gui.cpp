#include "gui.hpp"
#include "debugging.hpp"

#include <string>
#include <vector>

#ifdef _WIN32
	#include <windows.h>
	#include "SDL2/SDL_syswm.h"
#endif

#include "SDL2/SDL.h"

#include "assert.hpp"

#define setFlag(mask) (flags) |= (mask)
#define clearFlag(mask) (flags) &= ~(mask)

namespace GUI {
	#ifdef _WIN32
		std::vector<GUI::VoidCallback> void_callbacks;
		HMENU menu_bar_handle = 0;
	#endif

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

	TextElement::TextElement() : Element() {
		setName("");
	#ifdef _WIN32
		flags = MF_STRING;
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

	void TextElement::operator=(const TextElement& other) {
		Element::operator=(static_cast<const Element&>(other));
		setName(other.name);
	}

	Button::Button() : TextElement(""), callback(NULL) {
	#ifdef _WIN32
		flags = MF_STRING;
		id = void_callbacks.size();
		void_callbacks.push_back(callback);
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

	Menu::Menu() : TextElement("") {
	#ifdef _WIN32
		flags = MF_POPUP;
		id = (UINT_PTR)menu_handle;
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

} // end namespace GUI