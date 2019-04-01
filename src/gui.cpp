#include "gui.hpp"
#include "common.hpp"
#include "globals.hpp"

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

	bool Element::click(int x, int y) {
		return inRect(x, y, rect);
	}

	void Element::mouseMotion(int x, int y) {}

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

	Button::Button(SDL_Rect rect, std::string text, Callback callback)
		: TextElement(rect, text), callback(callback) {}

	Button::~Button() {}

	bool Button::click(int x, int y) {
		if (inRect(x, y, rect)) {
			if (callback != NULL) {
				(*callback)();
			}
			return true;
		}
		return false;
	}

	Container::Container(SDL_Rect rect) : Element(rect) {}

	Container::~Container() {}

	void Container::render() {
		Element::render();
		for(auto element : elements) {
			element->render();
		}
	}

	bool Container::click(int x, int y) {
		bool clicked = false;
		for(auto element : elements) {
			clicked = clicked || element->click(x, y);
		}
		return clicked;
	}

	void Container::mouseMotion(int x, int y) {
		for(auto element : elements) {
			element->mouseMotion(x, y);
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

	void Container::addElement(Element& element) {
		elements.push_back(&element);
		this->placeElements(elements.size()-1);
	}

	void Container::placeElements(int start) {}

	HBox::HBox(SDL_Rect rect) : Container(rect) {}

	HBox::~HBox() {}

	void HBox::placeElements(int start) {
		for (int i = start; i < (int)elements.size(); i++) {
			if (i == 0) {
				elements[i]->setPosition(rect.x, rect.y);
			} else {
				const SDL_Rect& prev_rect = elements[i-1]->rect;
				elements[i]->setPosition(prev_rect.x + prev_rect.w, rect.y);
			}
		}
	}

	VBox::VBox(SDL_Rect rect) : Container(rect) {}

	VBox::~VBox() {}

	void VBox::placeElements(int start) {
		for (int i = start; i < (int)elements.size(); i++) {
			if (i == 0) {
				elements[i]->setPosition(rect.x, rect.y);
			} else {
				const SDL_Rect& prev_rect = elements[i-1]->rect;
				elements[i]->setPosition(rect.x, prev_rect.y + prev_rect.h);
			}
		}
	}

	DropDown::DropDown(SDL_Rect rect, std::string text, Callback open_callback)
			: Container(rect), text(rect, text), open_callback(open_callback),
			list({rect.x, rect.y + rect.h, 0, 0}) {
		elements.push_back(&list);
		open = false;
	}

	DropDown::~DropDown() {}

	void DropDown::render() {
		text.render();
		if (open) {
			list.render();
		}
	}

	bool DropDown::click(int x, int y) {
		bool clicked = false;
		if (inRect(x, y, rect)) {
			open = !open;
			clicked = true;
			if (open && open_callback != NULL) {
				(*open_callback)();
			}
		} else if (open) {
			clicked = list.click(x, y);
		}
		return clicked;
	}

	void DropDown::mouseMotion(int x, int y) {
		if (!inRect(x, y, rect) && !inRect(x, y, list.rect)) {
			open = false;
		}
	}

	void DropDown::addElement(Element& element) {
		int new_width = MAX(list.rect.w, element.rect.w);
		int new_height = list.rect.h + element.rect.h;
		list.setSize(new_width, new_height);
		list.addElement(element);
	}

	void DropDown::setPosition(int x, int y) {
		Container::setPosition(x, y);
		list.setPosition(x, y + rect.h);
		text.setPosition(x, y);
	}

	void DropDown::setSize(int width, int height) {
		Container::setSize(width, height);
		text.setSize(width, height);
	}

	RadioButton::RadioButton(SDL_Rect rect, std::string text, RadioCallback callback,
			bool default_on) : TextElement(rect, text), callback(callback),
			on(default_on) {
		radio_rect.w = radio_rect.h = text_rect.h;
		setTextPlacement();
	}

	RadioButton::~RadioButton() {}

	void RadioButton::render() {
		TextElement::render();
		// render button
	}

	void RadioButton::setTextPlacement() {
		text_rect.x = rect.x + (rect.w - text_rect.w - radio_rect.w)/2;
		text_rect.y = rect.y + (rect.h - text_rect.h - radio_rect.h)/2;
		radio_rect.x = text_rect.x + text_rect.w;
		radio_rect.y = text_rect.y;
	}

	bool RadioButton::click(int x, int y) {
		if (inRect(x, y, rect)) {
			on = !on;
			if (callback != NULL) {
				(*callback)(on);
			}
			return true;
		}
		return false;
	}
};