#ifndef GUI_HPP
#define GUI_HPP

#include <string>
#include <vector>

#include "SDL2/SDL.h"
#include "SDL2/SDL_ttf.h"

#include "assert.hpp"

#define GUI_BAR_HEIGHT 16

namespace Gui {
	typedef void(*ButtonCallback)(void);
	typedef void(*RadioCallback)(bool);
	typedef void(*SliderCallback)(float);

	extern TTF_Font* font;
	const SDL_Color TEXT_COLOUR = { 0, 0, 0 };

	void init();

	class Element {
	public:
		Element(SDL_Rect rect);
		virtual ~Element();
		virtual void render();
		virtual void click(int x, int y);
		virtual void setPosition(int x, int y);
		virtual void setSize(int width, int height);

		SDL_Rect rect;

	protected:
	};

	class TextElement : public Element {
	public:
		TextElement(SDL_Rect rect, std::string text);
		virtual ~TextElement();
		virtual void render();
		virtual void setPosition(int x, int y);
		virtual void setSize(int width, int height);
		void setTextPlacement();

	protected:
		SDL_Rect text_rect;
		SDL_Texture* text_tex;
	};

	class Button : public TextElement {
	public:
		Button(SDL_Rect rect, std::string text, ButtonCallback callback);
		virtual ~Button();
		virtual void click(int x, int y);

	protected:
		ButtonCallback callback;
	};

	class Container : public Element {
	public:
		Container(SDL_Rect rect);
		virtual ~Container();
		virtual void render();
		virtual void click(int x, int y);
		virtual void setPosition(int x, int y);
		virtual void setSize(int width, int height);
		virtual void addElement(Element* element);
		virtual void placeElements(int start = 0);

	protected:
		std::vector<Element*> elements;
	};

	class HBox : public Container {
	public:
		HBox(SDL_Rect rect);
		virtual ~HBox();
		virtual void placeElements(int start = 0);
	protected:
	};
};

#endif