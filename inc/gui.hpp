#ifndef GUI_HPP
#define GUI_HPP

#include <string>
#include <vector>

#include "SDL2/SDL.h"
#include "SDL2/SDL_ttf.h"

#include "assert.hpp"

#define GUI_BAR_HEIGHT 16

namespace Gui {
	typedef void(*Callback)(void);
	typedef void(*RadioCallback)(bool);
	typedef void(*SliderCallback)(float);

	extern TTF_Font* font;
	const SDL_Color TEXT_COLOUR = { 0, 0, 0, 255 };

	void init();

	class Element {
	public:
		Element(SDL_Rect rect);
		virtual ~Element();

		// render element
		virtual void render();

		// tell element where mouse is
		// returns true if mouse is on element or sub-element
		virtual bool mouseMotion(int x, int y);

		// tell element where the mouse clicked
		// returns true if element or sub-element was clicked on
		virtual bool click(int x, int y);

		// sets x/y position of element
		virtual void setPosition(int x, int y);

		// sets width/height of element
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

	protected:
		virtual void setTextPlacement();

		SDL_Rect text_rect;
		SDL_Texture* text_tex;
	};

	class Button : public TextElement {
	public:
		Button(SDL_Rect rect, std::string text, Callback callback);
		virtual ~Button();
		virtual bool click(int x, int y);

	protected:
		Callback callback;
	};

	class Container : public Element {
	public:
		Container(SDL_Rect rect);
		virtual ~Container();
		virtual void render();
		virtual bool click(int x, int y);
		virtual bool mouseMotion(int x, int y);
		virtual void setPosition(int x, int y);
		virtual void setSize(int width, int height);

		// adds element to container
		virtual void addElement(Element& element);

		// sets x/y positions of sub-elements starting at index start
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

	class VBox : public Container {
	public:
		VBox(SDL_Rect rect);
		virtual ~VBox();
		virtual void placeElements(int start = 0);
	protected:
	};

	class DropDown : public Container {
	public:
		DropDown(SDL_Rect rect, std::string text);
		virtual ~DropDown();
		virtual void render();
		virtual bool click(int x, int y);
		virtual bool mouseMotion(int x, int y);
		virtual void addElement(Element& element);
		virtual void setPosition(int x, int y);
		virtual void setSize(int width, int height);

		bool open;

	protected:
		TextElement text;
		VBox list;
	};

	class SubDropDown : public DropDown {
	public:
		SubDropDown(SDL_Rect rect, std::string text);
		virtual ~SubDropDown();
		virtual void setSize(int width, int height);
	};

	class RadioButton : public TextElement {
	public:
		RadioButton(SDL_Rect rect, std::string text, bool* boolean = NULL, RadioCallback callback = NULL);
		virtual ~RadioButton();
		virtual void render();
		virtual bool click(int x, int y);

	protected:
		virtual void setTextPlacement();
		SDL_Rect radio_rect;
		bool* boolean;
		RadioCallback callback;
	};
};

#endif