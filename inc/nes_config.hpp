#include "config.hpp"

#define CONFIG_FILE "nes.cfg"

Config config;

std::string save_path = "./";
std::string rom_path = "./";

void saveDefaultConfig() {
	config.set("sprite_flickering", PPU::sprite_flickering);
	config.set("save_path", save_path);
	config.set("rom_path", rom_path);
	config.save(CONFIG_FILE);
}

void loadConfig() {
	if (config.load(CONFIG_FILE)) {
		PPU::sprite_flickering = config.getBool("sprite_flickering");
		save_path = config.getString("save_path");
		rom_path = config.getString("rom_path");
	} else {
		saveDefaultConfig();
	}
}