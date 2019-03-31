#ifndef GUI_HPP
#define GUI_HPP

#include "SDL2/SDL.h"
#include "SDL2/SDL_ttf.h"
#include <string>
#include "assert.hpp"

namespace Gui {
	TTF_Font* font;
	const SDL_Color WHITE = { 255, 255, 255 };
	const SDL_Color BLACK = { 0, 0, 0 };

	void init() {
		assert(TTF_Init() >= 0, "TTF did not initialize");
		font = TTF_OpenFont("./fonts/PressStart2P.ttf", 8);
		assert(font != NULL, "could not open font");
	}

	class Element {
	public:
		Element(SDL_Rect rect);
		virtual ~Element();
		virtual void render();

	protected:
		SDL_Rect rect;
		SDL_Texture* texture;
	};

	Element::Element(SDL_Rect rect) : rect(rect), texture(NULL) {}

	Element::~Element(){
		if (texture) {
			SDL_DestroyTexture(texture);
		}
	}

	void Element::render() {
		if (texture) {
			SDL_RenderCopy(sdl_renderer, texture, NULL, &rect);
		}
	}

	class TextElement : public Element {
	public:
		TextElement(SDL_Rect rect, std::string text);
		virtual ~TextElement();
		virtual void render();

	protected:
		SDL_Rect text_rect;
		SDL_Texture* text_tex;
	};

	TextElement::TextElement(SDL_Rect rect, std::string text) : Element(rect) {
		// create text surface
		SDL_Surface* text_surf = TTF_RenderText_Solid(font, text.c_str(), WHITE);
		text_tex = SDL_CreateTextureFromSurface(sdl_renderer, text_surf);
		SDL_FreeSurface(text_surf);

		// calculate text placement
		TTF_SizeText(font, text.c_str(), &text_rect.w, &text_rect.h);
		text_rect.x = rect.x + (rect.w - text_rect.w)/2;
		text_rect.y = rect.y + (rect.h - text_rect.h)/2;
	}

	TextElement::~TextElement() {
		SDL_DestroyTexture(text_tex);
	}

	void TextElement::render() {
		SDL_RenderCopy(sdl_renderer, text_tex, NULL, &text_rect);
	}
};

#endif