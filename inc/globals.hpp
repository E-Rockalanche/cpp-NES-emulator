#ifndef GLOBALS_HPP
#define GLOBALS_HPP

// SDL
extern SDL_Window* sdl_window;
extern SDL_Renderer* sdl_renderer;

// NES
extern Joypad joypad[4];
extern Zapper zapper;
extern bool paused;
extern bool muted;

extern const int SCREEN_BPP; // bits per pixel
extern Pixel screen[SCREEN_WIDTH * SCREEN_HEIGHT];

// paths
extern std::string file_name;
extern std::string save_path;
extern std::string rom_path;
extern std::string screenshot_path;

// window size
extern int window_width;
extern int window_height;
extern bool fullscreen;
extern float render_scale;
extern int render_width;
extern int render_height;

// frame timing
extern const unsigned int TARGET_FPS;
extern const double TIME_PER_FRAME;
extern int last_time;
extern int last_render_time;
extern double last_wait_time;

// frame rate
extern int total_frames;
extern float fps;
extern float total_fps;
extern float real_fps;
extern float total_real_fps;

#endif