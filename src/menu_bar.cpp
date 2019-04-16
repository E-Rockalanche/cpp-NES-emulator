#include "menu_bar.hpp"
#include "menu_elements.hpp"
#include "globals.hpp"
#include "hotkeys.hpp"
#include "ppu.hpp"

Menu::Menu menu_bar;

Menu::Menu file_menu;
Menu::Button load_rom_button;
Menu::Button close_rom_button;
Menu::Button exit_button;

Menu::Menu movie_submenu;
Menu::Button load_movie_button;
Menu::Button save_movie_button;
Menu::Checkbox record_movie_button;
Menu::Checkbox play_movie_button;

Menu::Menu view_menu;
Menu::Checkbox fullscreen_button;
Menu::Menu scale_menu;
Menu::Button scale1_button;
Menu::Button scale2_button;
Menu::Button scale3_button;

Menu::Menu machine_menu;
Menu::Button power_button;
Menu::Button reset_button;
Menu::Checkbox sprite_flicker_button;

Menu::Menu options_menu;
Menu::Checkbox pause_button;
Menu::Checkbox mute_button;

void constructMenu() {
	menu_bar = Menu::Menu("Menu bar");
	menu_bar.setMenuBar(window);

	file_menu = Menu::Menu("File");
	load_rom_button = Menu::Button("Open", selectRom);
	close_rom_button = Menu::Button("Close", closeFile);
	exit_button = Menu::Button("Exit", quit);

	movie_submenu = Menu::Menu("Movie");
	load_movie_button = Menu::Button("Load", loadMovie);
	save_movie_button = Menu::Button("Save", saveMovie);
	save_movie_button.disable();
	record_movie_button = Menu::Checkbox("Record", toggleRecording);
	play_movie_button = Menu::Checkbox("Play", togglePlayback);

	view_menu = Menu::Menu("View");
	fullscreen_button = Menu::Checkbox("Fullscreen (F11)", toggleFullscreen);
	scale_menu = Menu::Menu("Scale");
	scale1_button = Menu::Button("x1", setResolutionScale1);
	scale2_button = Menu::Button("x2", setResolutionScale2);
	scale3_button = Menu::Button("x3", setResolutionScale3);

	machine_menu = Menu::Menu("Machine");
	power_button = Menu::Button("Power", power);
	reset_button = Menu::Button("Reset", reset);
	sprite_flicker_button = Menu::Checkbox("Sprite Flickering", toggleSpriteFlickering);
	sprite_flicker_button.check(PPU::sprite_flickering);

	options_menu = Menu::Menu("Options");
	pause_button = Menu::Checkbox("Pause (P)", togglePaused);
	mute_button = Menu::Checkbox("Mute (M)", toggleMute);

	// append items
	menu_bar.append(file_menu);
	menu_bar.append(view_menu);
	menu_bar.append(machine_menu);
	menu_bar.append(options_menu);

	movie_submenu.append(save_movie_button);
	movie_submenu.append(load_movie_button);
	movie_submenu.seperateMenu();
	movie_submenu.append(record_movie_button);
	movie_submenu.append(play_movie_button);

	file_menu.append(load_rom_button);
	file_menu.append(close_rom_button);
	file_menu.seperateMenu();
	file_menu.append(movie_submenu);
	file_menu.seperateMenu();
	file_menu.append(exit_button);

	scale_menu.append(scale1_button);
	scale_menu.append(scale2_button);
	scale_menu.append(scale3_button);

	view_menu.append(fullscreen_button);
	view_menu.append(scale_menu);

	machine_menu.append(power_button);
	machine_menu.append(reset_button);
	machine_menu.seperateMenu();
	machine_menu.append(sprite_flicker_button);

	options_menu.append(pause_button);
	options_menu.append(mute_button);
}