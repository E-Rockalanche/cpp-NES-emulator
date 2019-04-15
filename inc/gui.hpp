#ifndef GUI_HPP
#define GUI_HPP

#include <string>

#ifdef _WIN32
	#include <windows.h>
#endif

#include "SDL2/SDL.h"

#ifdef _WIN32
	#define SDL_GUI_EVENT SDL_SYSWMEVENT
#else
	#define SDL_GUI_EVENT -1
#endif

namespace GUI {
	typedef void(*VoidCallback)(void);

	void handleMenuEvent(const SDL_Event& event);

	class Element {
	protected:
		Element();

		#ifdef _WIN32
			UINT flags; // type of menu element
			HMENU menu_handle;
			UINT_PTR id; // command or menu handle ptr
			LPCTSTR display_ptr; // string or bitmap
		#endif

	public:
		#ifdef _WIN32
			UINT getFlags() { return flags; }
			HMENU getHandle() { return menu_handle; }
			UINT_PTR getId() { return id; }
			LPCTSTR getDisplay() { return display_ptr; }
		#endif
			
		void enable(bool e = true);
		void disable();
	};

	class TextElement : public Element {
	protected:
		std::string name;

	public:
		TextElement();
		TextElement(const std::string& name);
		void setName(const std::string& name);
		void operator=(const TextElement& other);

		const std::string& getName() { return name; }
	};

	class Button : public TextElement {
	public:
		Button();
		Button(const std::string& name, VoidCallback callback = NULL);
		void setCallback(VoidCallback callback);
		void operator=(const Button& other);

	protected:
		VoidCallback callback;
	};

	class Checkbox : public Button {
	public:
		Checkbox();
		Checkbox(const std::string& name, VoidCallback callback = NULL, bool checked = false);
		void check(bool check = true);
		void uncheck();
		void operator=(const Checkbox& other) {
			Button::operator=(static_cast<const Button&>(other));
		}
	};

	class MenuSeperator : public Element {
	public:
		MenuSeperator();
	};

	class MenuBreak : public Element {
	public:
		MenuBreak();
	};

	class Menu : public TextElement {
	public:
		Menu();
		Menu(const std::string& name);
		void append(Element& element);
		void seperateMenu();
		void breakMenu();
		void setMenuBar(SDL_Window* window);
		void hide();
		void show();
		void operator=(const Menu& other);

	private:
	#ifdef _WIN32
		HWND window_handler;
	#endif
	};

} // end namespace GUI

#endif