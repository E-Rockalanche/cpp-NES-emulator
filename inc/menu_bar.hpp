#ifndef MENU_BAR_HPP
#define MENU_BAR_HPP

#include "menu_elements.hpp"

extern Menu::Menu menu_bar;

extern Menu::Menu file_menu;
extern Menu::Button load_rom_button;
extern Menu::Button close_rom_button;

extern Menu::Menu movie_submenu;
extern Menu::Button load_movie_button;
extern Menu::Button save_movie_button;
extern Menu::Checkbox record_movie_button;
extern Menu::Checkbox play_movie_button;

extern Menu::Menu view_menu;
extern Menu::Checkbox fullscreen_button;

extern Menu::Menu scale_menu;
extern Menu::Button scale1_button;
extern Menu::Button scale2_button;
extern Menu::Button scale3_button;

extern Menu::Menu machine_menu;
extern Menu::Button power_button;
extern Menu::Button reset_button;
extern Menu::Checkbox sprite_flicker_button;

extern Menu::Menu options_menu;
extern Menu::Checkbox pause_button;
extern Menu::Checkbox mute_button;

void constructMenu();

#endif