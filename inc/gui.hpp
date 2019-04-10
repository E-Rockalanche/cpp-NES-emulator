#ifndef GUI_HPP
#define GUI_HPP

#include <string>
#include <vector>

#include "SDL2/SDL.h"
#include "SDL2/SDL_ttf.h"

#include "assert.hpp"

namespace Gui {
	typedef void(*Callback)(void);
	typedef void(*RadioCallback)(bool);
	typedef void(*SliderCallback)(float);

	extern TTF_Font* font;

	void init();

	class Container;

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

		// calls setPosition to move by dx and dy
		virtual void move(int dx, int dy);

		// sets width/height of element
		virtual void setSize(int width, int height);

		// tells element what key was pressed
		// returns true is element processed input
		virtual bool keyInput(SDL_Keycode key);

		SDL_Rect rect;
		Container* container;
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

	class Label : public TextElement {
	public:
		Label(SDL_Rect rect, std::string text, Element& element);
		virtual ~Label();
		virtual void render();
		virtual void setPosition(int x, int y);
		virtual void setSize(int width, int height);
		virtual bool click(int x, int y);
		virtual bool mouseMotion(int x, int y);
		virtual bool keyInput(SDL_Keycode key);

	protected:
		Element* element;
	};

	class DynamicTextElement : public Element {
	public:
		DynamicTextElement(SDL_Rect rect, std::string* text = NULL);
		virtual ~DynamicTextElement();
		virtual void render();
		virtual void setPosition(int x, int y);

	protected:
		SDL_Rect text_rect;
		std::string* dynamic_text;
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
		virtual bool keyInput(SDL_Keycode key);

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

	template<typename T>
	class Field : public DynamicTextElement {
	public:
		Field(SDL_Rect rect, T* data = NULL, Callback input_callback = NULL);
		virtual ~Field();
		virtual void render();
		virtual bool click(int x, int y);
		virtual bool mouseMotion(int x, int y);
		virtual bool keyInput(SDL_Keycode key);

	protected:
		T* data;
		bool active;
		std::string display_string;
		std::string input_string;
		Callback input_callback;
		int flash_counter;
		int flash_period;
	};

	template class Field<int>;
	template class Field<float>;
	template class Field<std::string>;
};

#endif