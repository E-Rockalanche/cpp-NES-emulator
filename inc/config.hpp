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
			"sprite flickering": true
		},
		"folders": {
			"rom folder": "roms/",
			"save folder": "saves/",
			"screenshot folder": "screenshots/"
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
		window_width = render_scale * SCREEN_WIDTH;
		window_height = render_scale * SCREEN_HEIGHT;

		const json& folders = config["folders"];
		rom_folder = folders["rom folder"].get<std::string>();
		save_folder = folders["save folder"].get<std::string>();
		screenshot_folder = folders["screenshot folder"].get<std::string>();
		API::createDirectory(rom_folder);
		API::createDirectory(save_folder);
		API::createDirectory(screenshot_folder);

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