#include "menu.hpp"
#include "gui.hpp"
#include "globals.hpp"
#include "hotkeys.hpp"
#include "ppu.hpp"

GUI::Menu menu_bar;

GUI::Menu file_menu;
GUI::Button load_rom_button;
GUI::Button close_rom_button;

GUI::Menu movie_submenu;
GUI::Button load_movie_button;
GUI::Button save_movie_button;
GUI::Checkbox record_movie_button;
GUI::Checkbox play_movie_button;

GUI::Menu view_menu;
GUI::Checkbox fullscreen_button;
GUI::Menu scale_menu;
GUI::Button scale1_button;
GUI::Button scale2_button;
GUI::Button scale3_button;

GUI::Menu machine_menu;
GUI::Button power_button;
GUI::Button reset_button;
GUI::Checkbox sprite_flicker_button;

GUI::Menu options_menu;
GUI::Checkbox pause_button;
GUI::Checkbox mute_button;

void constructMenu() {
	menu_bar = GUI::Menu("Menu bar");
	menu_bar.setMenuBar(window);

	file_menu = GUI::Menu("File");
	load_rom_button = GUI::Button("Open", selectRom);
	close_rom_button = GUI::Button("Close", closeFile);

	movie_submenu = GUI::Menu("Movie");
	load_movie_button = GUI::Button("Load", loadMovie);
	save_movie_button = GUI::Button("Save", saveMovie);
	save_movie_button.disable();
	record_movie_button = GUI::Checkbox("Record", toggleRecording);
	play_movie_button = GUI::Checkbox("Play", togglePlayback);

	view_menu = GUI::Menu("View");
	fullscreen_button = GUI::Checkbox("Fullscreen (F11)", toggleFullscreen);
	scale_menu = GUI::Menu("Scale");
	scale1_button = GUI::Button("x1", setResolutionScale1);
	scale2_button = GUI::Button("x2", setResolutionScale2);
	scale3_button = GUI::Button("x3", setResolutionScale3);

	machine_menu = GUI::Menu("Machine");
	power_button = GUI::Button("Power", power);
	reset_button = GUI::Button("Reset", reset);
	sprite_flicker_button = GUI::Checkbox("Sprite Flickering", toggleSpriteFlickering);
	sprite_flicker_button.check(PPU::sprite_flickering);

	options_menu = GUI::Menu("Options");
	pause_button = GUI::Checkbox("Pause (P)", togglePaused);
	mute_button = GUI::Checkbox("Mute (M)", toggleMute);

	// append items
	menu_bar.append(file_menu);
	menu_bar.append(view_menu);
	menu_bar.append(machine_menu);
	menu_bar.append(options_menu);

	movie_submenu.append(save_movie_button);
	movie_submenu.append(load_movie_button);
	movie_submenu.append(record_movie_button);
	movie_submenu.append(play_movie_button);

	file_menu.append(load_rom_button);
	file_menu.append(close_rom_button);
	file_menu.append(movie_submenu);

	scale_menu.append(scale1_button);
	scale_menu.append(scale2_button);
	scale_menu.append(scale3_button);

	view_menu.append(fullscreen_button);
	view_menu.append(scale_menu);

	machine_menu.append(power_button);
	machine_menu.append(reset_button);
	machine_menu.append(sprite_flicker_button);

	options_menu.append(pause_button);
	options_menu.append(mute_button);
}