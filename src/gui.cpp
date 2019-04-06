#include "gui.hpp"
#include "common.hpp"
#include "globals.hpp"
// #include "SDL_FontCache.h"

#define inRect(x, y, rect) inBounds(x, y, rect.x, rect.y, rect.w, rect.h)

namespace Gui {
	TTF_Font* ttf_font = NULL;
	// FC_Font* fc_font = NULL;

	const char* FONT_FILENAME = "fonts/segoeui.ttf";

	const SDL_Color TEXT_COLOUR = { 0, 0, 0, 255 };
	const SDL_Color BACK_COLOUR = { 255, 255, 255, 255 };

	void init() {
		// ttf
		assert(TTF_Init() >= 0, "TTF did not initialize");
		ttf_font = TTF_OpenFont(FONT_FILENAME, 12);
		assert(ttf_font != NULL, "could not open ttf_font");
		/*
		// cached
		fc_font = FC_CreateFont();
		assert(fc_font != NULL, "FC_Font is NULL");
		FC_LoadFontFromTTF(fc_font, renderer, ttf_font, TEXT_COLOUR);
		*/
	}

	Element::Element(SDL_Rect rect) : rect(rect), container(NULL) {}

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
		if (container) container->placeElements();
	}

	bool Element::click(int x, int y) {
		return inRect(x, y, rect);
	}

	bool Element::mouseMotion(int x, int y) {
		return inRect(x, y, rect);
	}

	TextElement::TextElement(SDL_Rect rect, std::string text) : Element(rect) {
		// create text surface
		SDL_Surface* text_surf = TTF_RenderText_Blended(ttf_font, text.c_str(), TEXT_COLOUR);
		text_tex = SDL_CreateTextureFromSurface(renderer, text_surf);
		SDL_FreeSurface(text_surf);

		// calculate text size
		TTF_SizeText(ttf_font, text.c_str(), &text_rect.w, &text_rect.h);

		// adjust size and text placement
		setSize(MAX(rect.w, text_rect.w + 8), MAX(rect.h, text_rect.h + 8));
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

	DynamicTextElement::DynamicTextElement(SDL_Rect rect, std::string* text)
			: Element(rect), dynamic_text(text) {
		text_rect.x = rect.x + 4;
		text_rect.y = rect.y + 4;
	}

	DynamicTextElement::~DynamicTextElement() {}

	void DynamicTextElement::render() {
		Element::render();
		/*
		if (dynamic_text) {
			FC_Draw(fc_font, renderer, text_rect.x, text_rect.y, dynamic_text->c_str());
		}
		*/

		// create text surface
		SDL_Surface* text_surf = TTF_RenderText_Blended(ttf_font, dynamic_text->c_str(), TEXT_COLOUR);
		SDL_Texture* text_tex = SDL_CreateTextureFromSurface(renderer, text_surf);
		SDL_FreeSurface(text_surf);

		// calculate text size
		TTF_SizeText(ttf_font, dynamic_text->c_str(), &text_rect.w, &text_rect.h);
		int margin = (rect.h - text_rect.h) / 2;
		text_rect.x = rect.x + margin;
		text_rect.y = rect.y + margin;

		SDL_RenderCopy(renderer, text_tex, NULL, &text_rect);

		SDL_DestroyTexture(text_tex);
	}

	void DynamicTextElement::setPosition(int x, int y) {
		int dx = x - rect.x;
		int dy = y - rect.y;
		Element::setPosition(x, y);
		text_rect.x += dx;
		text_rect.y += dy;
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
		bool in_menu = inRect(x, y, rect);
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
		element.container = this;
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

		setSize(MAX(this->rect.w, this->text.rect.w),
			MAX(this->rect.h, this->text.rect.h));
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
		list.rect.x = this->rect.x + this->rect.w;
		list.rect.y = this->rect.y;
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
		setSize(MAX(this->rect.w, this->text_rect.w + radio_rect.w + 12),
			MAX(this->rect.h, this->text_rect.h + 8));
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
		int h_seperation = (rect.w - text_rect.w - radio_rect.w) / 3;

		radio_rect.x = rect.x + h_seperation;
		radio_rect.y = rect.y + (rect.h - radio_rect.h) / 2;

		text_rect.x = radio_rect.x + radio_rect.w + h_seperation;
		text_rect.y = rect.y + (rect.h - text_rect.h) / 2;
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