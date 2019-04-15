#ifndef MENU_HPP
#define MENU_HPP

#include "gui.hpp"

extern GUI::Menu menu_bar;

extern GUI::Menu file_menu;
extern GUI::Button load_rom_button;
extern GUI::Button close_rom_button;

extern GUI::Menu movie_submenu;
extern GUI::Button load_movie_button;
extern GUI::Button save_movie_button;
extern GUI::Checkbox record_movie_button;
extern GUI::Checkbox play_movie_button;

extern GUI::Menu view_menu;
extern GUI::Checkbox fullscreen_button;

extern GUI::Menu scale_menu;
extern GUI::Button scale1_button;
extern GUI::Button scale2_button;
extern GUI::Button scale3_button;

extern GUI::Menu machine_menu;
extern GUI::Button power_button;
extern GUI::Button reset_button;
extern GUI::Checkbox sprite_flicker_button;

extern GUI::Menu options_menu;
extern GUI::Checkbox pause_button;
extern GUI::Checkbox mute_button;

void constructMenu();

#endif