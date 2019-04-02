#ifndef GLOBALS_HPP
#define GLOBALS_HPP

#include <string>

#include "SDL2/SDL.h"

#include "pixel.hpp"
#include "joypad.hpp"
#include "zapper.hpp"
#include "screen.hpp"

// SDL
extern SDL_Window* window;
extern SDL_Renderer* renderer;

// NES
extern Joypad joypad[4];
extern Zapper zapper;
extern bool paused;
extern bool in_menu;
extern bool muted;

// paths
extern std::string rom_filename;
extern std::string save_filename;
extern std::string save_folder;
extern std::string rom_folder;
extern std::string screenshot_folder;

// window size
extern int window_width;
extern int window_height;
extern bool fullscreen;
extern float render_scale;
extern SDL_Rect render_area;

// frame timing
extern const unsigned int TARGET_FPS;
extern const double TIME_PER_FRAME;
extern int last_time;
extern int last_render_time;
extern double last_wait_time;

// frame rate
extern int frame_number;
extern float fps;
extern float total_fps;
extern float real_fps;
extern float total_real_fps;

#endif