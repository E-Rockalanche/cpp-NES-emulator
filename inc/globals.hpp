#ifndef GLOBALS_HPP
#define GLOBALS_HPP

#include "SDL2/SDL.h"
#include "joypad.hpp"

extern SDL_Window* sdl_window;
extern SDL_Renderer* sdl_renderer;

extern Joypad joypad[4];
extern bool paused;
extern bool fullscreen;

extern std::string file_name;
extern std::string save_path;
extern std::string rom_path;
extern std::string screenshot_path;

extern int window_width;
extern int window_height;
extern bool fullscreen;
extern float render_scale;
extern int render_width;
extern int render_height;

extern int last_time;
extern int last_render_time;
extern double last_wait_time;

#endif