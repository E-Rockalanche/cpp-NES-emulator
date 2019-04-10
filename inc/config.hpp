#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <fstream>
#include <iomanip>

#include "json.hpp"
using nlohmann::json;

#include "globals.hpp"
#include "joypad.hpp"
#include "keyboard.hpp"
#include "api.hpp"
#include "assert.hpp"

json config;

const char* CONFIG_FILE = "config.json";

const char* DEFAULT_CONFIG = R"(
	{
		"general": {
			"fullscreen": false,
			"scale": 2.0,
			"sprite flickering": true,
			"crop x": 8,
			"crop y": 8
		},
		"paths": {
			"rom folder": "roms",
			"save folder": "saves",
			"screenshot folder": "screenshots",
			"movie folder": "movies",
			"savestate folder": "savestates",

			"save extension": ".sav",
			"movie extension": ".nesmov",
			"savestate extension": ".state"
		},
		"controls": [
			{
				"A": "X",
				"B": "Z",
				"select": "RIGHTSHIFT",
				"start": "RETURN",
				"up": "UP",
				"down": "DOWN",
				"left": "LEFT",
				"right": "RIGHT"
			}, {
				"A": "X",
				"B": "Z",
				"select": "RIGHTSHIFT",
				"start": "RETURN",
				"up": "UP",
				"down": "DOWN",
				"left": "LEFT",
				"right": "RIGHT"
			}, {
				"A": "X",
				"B": "Z",
				"select": "RIGHTSHIFT",
				"start": "RETURN",
				"up": "UP",
				"down": "DOWN",
				"left": "LEFT",
				"right": "RIGHT"
			}, {
				"A": "X",
				"B": "Z",
				"select": "RIGHTSHIFT",
				"start": "RETURN",
				"up": "UP",
				"down": "DOWN",
				"left": "LEFT",
				"right": "RIGHT"
			}
		]
	})";

void saveConfig(const json& config) {
	std::ofstream fout(CONFIG_FILE);
	assert((fout.is_open()), "Cannot save default config");
	fout << std::setw(4) << config;
	fout.close();
}

void loadConfig() {
	config = json::parse(DEFAULT_CONFIG);

	std::ifstream fin(CONFIG_FILE);
	if (fin.is_open()) {
		// load config
		json config_patch;
		fin >> config_patch;
		fin.close();
		config.merge_patch(config_patch);
	} else {
		saveConfig(config);
	}

	try {
		const json& general = config["general"];
		PPU::sprite_flickering = general["sprite flickering"].get<bool>();
		fullscreen = general["fullscreen"].get<bool>();
		render_scale = general["scale"].get<float>();
		crop_area.x = general["crop x"].get<int>();
		crop_area.y = general["crop y"].get<int>();

		crop_area.x = CLAMP(crop_area.x, 0, MAX_CROP);
		crop_area.y = CLAMP(crop_area.y, 0, MAX_CROP);
		crop_area.w = SCREEN_WIDTH - 2 * crop_area.x;
		crop_area.h = SCREEN_HEIGHT - 2 * crop_area.y;
		window_width = render_scale * crop_area.w;
		window_height = render_scale * crop_area.h + GUI_HEIGHT;

		const json& paths = config["paths"];
		rom_folder = paths["rom folder"].get<std::string>();
		save_folder = paths["save folder"].get<std::string>();
		screenshot_folder = paths["screenshot folder"].get<std::string>();
		movie_folder = paths["movie folder"].get<std::string>();
		savestate_folder = paths["savestate folder"].get<std::string>();

		save_ext = paths["save extension"].get<std::string>();
		movie_ext = paths["movie extension"].get<std::string>();
		savestate_ext = paths["savestate extension"].get<std::string>();

		API::createDirectory(rom_folder);
		API::createDirectory(save_folder);
		API::createDirectory(screenshot_folder);
		API::createDirectory(movie_folder);
		API::createDirectory(savestate_folder);

		// joypads
		for(int j = 0; j < 4; j++) {
			const json& keys = config["controls"][j];
			for(int b = 0; b < Joypad::NUM_BUTTONS; b++) {
				const char* button_name = Joypad::getButtonName((Joypad::Button)b);
				std::string key_name = keys[button_name].get<std::string>();
				SDL_Keycode keycode = getKeycode(key_name);
				joypad[j].mapButton((Joypad::Button)b, keycode);
			}
		}
	} catch( ... ) {
		dout("A config variable was invalid");
	}
}

#endif