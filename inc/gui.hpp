#ifndef GUI_HPP
#define GUI_HPP

#include <string>

#ifdef _WIN32
	#include <windows.h>
#endif

#include "SDL2/SDL.h"

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
		#endif

	public:
		#ifdef _WIN32
			UINT getFlags() { return flags; }
			HMENU getHandle() { return menu_handle; }
			UINT_PTR getId() { return id; }
		#endif
	};

	class TextElement : public Element {
	protected:
		std::string name;

	public:
		TextElement(std::string name = "");
		void setName(std::string name);

		const std::string& getName() { return name; }
	};

	class Button : public TextElement {
	public:
		Button(std::string name = "", VoidCallback callback = NULL);
		void setCallback(VoidCallback callback);

	protected:
		VoidCallback callback;
	};

	class Menu : public TextElement {
	public:
		Menu(std::string name = "");
		void append(TextElement& element);
		void setMenuBar(SDL_Window* window);
	};

} // end namespace GUI

#endif