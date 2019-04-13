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
	};

	class TextElement : public Element {
	protected:
		std::string name;

	public:
		TextElement(const std::string& name);
		void setName(const std::string& name);

		const std::string& getName() { return name; }
	};

	class Button : public TextElement {
	public:
		Button(const std::string& name, VoidCallback callback = NULL);
		void setCallback(VoidCallback callback);

	protected:
		VoidCallback callback;
	};

	class MenuSeperator : public Element {
		MenuSeperator();
	};

	class MenuBreak : public Element {
		MenuBreak();
	};

	class Menu : public TextElement {
	public:
		Menu(const std::string& name);
		void append(Element& element);
		void setMenuBar(SDL_Window* window);
		void hide();
		void show();

	private:
	#ifdef _WIN32
		HWND window_handler;
	#endif
	};

} // end namespace GUI

#endif