#include "sdl.hpp"
#include "gui.hpp"
#include "screen.hpp"

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

int window_width = SCREEN_WIDTH;
int window_height = SCREEN_HEIGHT;
bool fullscreen = false;
float render_scale = 1;
SDL_Rect render_area = { 0, GUI_BAR_HEIGHT, 256, 240 };