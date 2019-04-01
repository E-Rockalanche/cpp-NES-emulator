#ifndef SDL_HPP
#define SDL_HPP

#include "SDL2/SDL.h"

extern SDL_Window*  window;
extern SDL_Renderer* renderer;

extern int window_width;
extern int window_height;
extern bool fullscreen;
extern float render_scale;
extern SDL_Rect render_area;

#endif