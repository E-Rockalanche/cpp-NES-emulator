#ifndef GLOBALS_HPP
#define GLOBALS_HPP

#include <string>

#include "SDL.h"
#include "filesystem.hpp"

#include "pixel.hpp"
#include "joypad.hpp"
#include "zapper.hpp"
#include "Nes.hpp"

constexpr int DefaultCrop = 8;
constexpr int MaxCrop = 8;

// SDL
extern SDL_Window* window;
extern SDL_Renderer* renderer;

// NES
extern nes::Joypad joypad[ 4 ];
extern nes::Zapper zapper;
extern nes::Nes s_nes;
extern bool paused;
extern bool step_frame;
extern bool in_menu;
extern bool muted;

// paths
extern fs::path rom_filename;
extern fs::path save_filename;
extern fs::path save_folder;
extern fs::path rom_folder;
extern fs::path screenshot_folder;
extern fs::path movie_folder;
extern fs::path savestate_folder;

extern std::string rom_ext;
extern std::string save_ext;
extern std::string savestate_ext;
extern std::string movie_ext;

// window size
extern int window_width;
extern int window_height;
extern bool fullscreen;
extern float render_scale;
extern SDL_Rect render_area;
extern SDL_Rect crop_area;

// frame timing
extern const unsigned int TARGET_FPS;
extern const float TIME_PER_FRAME;
extern int last_time;
extern int last_render_time;
extern float last_wait_time;

// frame rate
extern int frame_number;
extern float fps;
extern float total_fps;
extern float real_fps;
extern float total_real_fps;

void resetFrameNumber();
void reset();
void power();
bool loadFile(std::string filename);
bool loadSave(std::string filename);
void saveGame();
void resizeRenderArea(bool round_scale = false);
void resizeWindow(int width, int height);
void cropScreen(int dx, int dy);

#endif