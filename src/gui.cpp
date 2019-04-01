#include "gui.hpp"
#include "sdl.hpp"
#include "common.hpp"

#define inRect(x, y, rect) inBounds(x, y, rect.x, rect.y, rect.w, rect.h)

namespace Gui {
	TTF_Font* font;

	void init() {
		assert(TTF_Init() >= 0, "TTF did not initialize");
		font = TTF_OpenFont("./fonts/segoeui.ttf", 12);
		assert(font != NULL, "could not open font");
	}

	Element::Element(SDL_Rect rect) : rect(rect) {}

	Element::~Element(){}

	void Element::render() {
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // white
		SDL_RenderFillRect(renderer, &rect);
	}

	void Element::setPosition(int x, int y) {
		rect.x = x;
		rect.y = y;
	}

	void Element::setSize(int width, int height) {
		rect.w = width;
		rect.h = height;
	}

	void Element::click(int x, int y) {}

	TextElement::TextElement(SDL_Rect rect, std::string text) : Element(rect) {
		// create text surface
		SDL_Surface* text_surf = TTF_RenderText_Blended(font, text.c_str(), TEXT_COLOUR);
		text_tex = SDL_CreateTextureFromSurface(renderer, text_surf);
		SDL_FreeSurface(text_surf);

		// calculate text placement
		TTF_SizeText(font, text.c_str(), &text_rect.w, &text_rect.h);
		setTextPlacement();
	}

	TextElement::~TextElement() {
		SDL_DestroyTexture(text_tex);
	}

	void TextElement::render() {
		Element::render();
		SDL_RenderCopy(renderer, text_tex, NULL, &text_rect);
	}

	void TextElement::setPosition(int x, int y) {
		Element::setPosition(x, y);
		setTextPlacement();
	}

	void TextElement::setSize(int width, int height) {
		Element::setSize(width, height);
		setTextPlacement();
	}

	void TextElement::setTextPlacement() {
		text_rect.x = rect.x + (rect.w - text_rect.w)/2;
		text_rect.y = rect.y + (rect.h - text_rect.h)/2;
	}

	Button::Button(SDL_Rect rect, std::string text, ButtonCallback callback)
		: TextElement(rect, text), callback(callback) {}

	Button::~Button() {}

	void Button::click(int x, int y) {
		if (inRect(x, y, rect)) {
			(*callback)();
		}
	}

	Container::Container(SDL_Rect rect) : Element(rect) {}

	Container::~Container() {}

	void Container::render() {
		Element::render();
		for(auto element : elements) {
			element->render();
		}
	}

	void Container::click(int x, int y) {
		if (inRect(x, y, rect)) {
			for(auto element : elements) {
				element->click(x, y);
			}
		}
	}

	void Container::setPosition(int x, int y) {
		Element::setPosition(x, y);
		this->placeElements();
	}

	void Container::setSize(int width, int height) {
		Element::setSize(width, height);
		this->placeElements();
	}

	void Container::addElement(Element* element) {
		elements.push_back(element);
		this->placeElements(elements.size()-1);
	}

	void Container::placeElements(int start) {}

	HBox::HBox(SDL_Rect rect) : Container(rect) {}

	HBox::~HBox() {}

	void HBox::placeElements(int start) {
		for (int i = start; i < elements.size(); i++) {
			if (i == 0) {
				elements[i]->setPosition(rect.x, rect.y);
			} else {
				const SDL_Rect& prev_rect = elements[i-1]->rect;
				elements[i]->setPosition(prev_rect.x + prev_rect.w, rect.y);
			}
		}
	}
};