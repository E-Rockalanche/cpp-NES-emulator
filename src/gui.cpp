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

	bool Element::mouseMotion(int x, int y) {
		return inRect(x, y, rect);
	}

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
			if (element->click(x, y)) {
				clicked = true;
			}
		}
		return clicked;
	}

	bool Container::mouseMotion(int x, int y) {
		bool in_menu = false;
		for(auto element : elements) {
			if (element->mouseMotion(x, y)) {
				in_menu = true;
			}
		}
		return in_menu;
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

	DropDown::DropDown(SDL_Rect rect, std::string text) : Container(rect),
			text(rect, text), list({rect.x, rect.y + rect.h, 0, 0}) {
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
		} else if (open) {
			clicked = list.click(x, y);
		}
		return clicked;
	}

	bool DropDown::mouseMotion(int x, int y) {
		bool in_menu = inRect(x, y, rect) || (open && list.mouseMotion(x, y));
		if (!in_menu) open = false;
		return in_menu;
	}

	void DropDown::addElement(Element& element) {
		int new_width = MAX(list.rect.w, element.rect.w);
		int new_height = list.rect.h + element.rect.h;
		list.setSize(new_width, new_height);
		list.addElement(element);
	}

	void DropDown::setPosition(int x, int y) {
		int dx = x - rect.x;
		int dy = y - rect.y;
		Container::setPosition(x, y);
		text.setPosition(x, y);
		list.setPosition(list.rect.x + dx, list.rect.y + dy);
	}

	void DropDown::setSize(int width, int height) {
		int dh = height - rect.h;
		Container::setSize(width, height);
		text.setSize(width, height);
		list.rect.y += dh;
	}

	SubDropDown::SubDropDown(SDL_Rect rect, std::string text) : DropDown(rect, text) {
		list.rect.x = rect.x + rect.w;
		list.rect.y = rect.y;
	}

	SubDropDown::~SubDropDown() {}

	void SubDropDown::setSize(int width, int height) {
		int dw = width - rect.w;
		Container::setSize(width, height);
		text.setSize(width, height);
		list.rect.x += dw;
	}

	RadioButton::RadioButton(SDL_Rect rect, std::string text, bool* boolean,
			RadioCallback callback) : TextElement(rect, text), boolean(boolean),
			callback(callback) {
		radio_rect.w = radio_rect.h = text_rect.h - 4;
		setTextPlacement();
	}

	RadioButton::~RadioButton() {}

	void RadioButton::render() {
		TextElement::render();
		// render button
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // black
		if (*boolean) {
			SDL_RenderFillRect(renderer, &radio_rect);
		} else {
			SDL_RenderDrawRect(renderer, &radio_rect);
		}
	}

	void RadioButton::setTextPlacement() {
		int seperation = (rect.w - text_rect.w - radio_rect.w) / 3;
		text_rect.x = rect.x + seperation;
		text_rect.y = rect.y + (rect.h - text_rect.h) / 2;
		radio_rect.x = text_rect.x + text_rect.w + seperation;
		radio_rect.y = rect.y + (rect.h - radio_rect.h) / 2;
	}

	bool RadioButton::click(int x, int y) {
		if (inRect(x, y, rect)) {
			*boolean = !*boolean;
			if (callback != NULL) {
				(*callback)(*boolean);
			}
			return true;
		}
		return false;
	}
};