#include "config.hpp"
#include <fstream>
#include <cctype>
#include <sstream>

std::string trim(std::string str) {
	int start, end;
	for(start = 0; start < (int)str.size(); start++) {
		if (!isspace(str[start])) break;
	}
	for(end = (int)str.size() - 1; end >= 0; end--) {
		if (!isspace(str[end])) break;
	}
	return std::string(str, start, end-start+1);
}

Config::Config(std::string filename) : filename(filename) {
	load(filename);
}

bool Config::load(std::string filename) {
	this->filename = filename;
	std::ifstream fin(filename);

	if (!fin.is_open() || !fin.good()) {
		return false;
	}

	std::string line_string;
	while (std::getline(fin, line_string)) {
		std::istringstream line_stream(line_string);
		std::string key;
		if (std::getline(line_stream, key, '=')) {
			key = trim(key);
			std::string value;
			if (std::getline(line_stream, value)) {
				if (key[0] == '#' || key[0] == ';') continue; // comment
				values[key] = trim(value);
			}
		}
	}

	fin.close();
	return true;
}

bool Config::save(std::string filename) {
	this->filename = filename;
	std::ofstream fout(filename);

	if (!fout.is_open() || !fout.good()) {
		return false;
	}

	for(auto it = values.begin(); it != values.end(); it++) {
		fout << it->first << " = " << it->second << '\n';
	}

	fout.close();
	return true;
}

bool Config::save() {
	if (filename == "") return false;
	return save(filename);
}

int Config::getInt(std::string key) {
	int value;
	try {
		value = std::stoi(values[key]);
	} catch(...) {
		value = 0;
	}
	return value;
}

float Config::getFloat(std::string key) {
	float value;
	try {
		value = std::stof(values[key]);
	} catch(...) {
		value = 0;
	}
	return value;
}

std::string Config::getString(std::string key) {
	return values[key];
}

bool Config::getBool(std::string key) {
	bool ok = false;
	std::string& value = values[key];
	if ((value == "TRUE") || (value == "true")) {
		ok = true;
	} else if ((value != "FALSE") && (value != "false")) {
		try {
			ok = std::stoi(value) == 1;
		} catch(...) {
			values[key] = "FALSE";
		}
	}
	return ok;
}

void Config::set(std::string key, int value) {
	values[key] = std::to_string(value);
}

void Config::set(std::string key, float value) {
	values[key] = std::to_string(value);
}

void Config::set(std::string key, std::string value) {
	values[key] = value;
}

void Config::set(std::string key, bool value) {
	values[key] = value ? "TRUE" : "FALSE";
}