#include "config.hpp"
#include "debugging.hpp"
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

Config::Config() {
	_updated = false;
	filename = "";
}

Config::Config(std::string filename) : filename(filename) {
	load(filename);
	_updated = false;
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

bool Config::updated() {
	return _updated;
}

int Config::getInt(std::string key, int default_value) {
	int value = default_value;
	auto it = values.find(key);
	if (it == values.end()) {
		set(key, default_value);
	} else {
		try {
			value = std::stoi(it->second);
		} catch(...) {
			set(key, default_value);
		}
	}
	return value;
}

float Config::getFloat(std::string key, float default_value) {
	float value = default_value;
	auto it = values.find(key);
	if (it == values.end()) {
		set(key, default_value);
	} else {
		try {
			value = std::stof(it->second);
		} catch(...) {
			set(key, default_value);
		}
	}
	return value;
}

std::string Config::getString(std::string key, std::string default_value) {
	std::string value = default_value;
	auto it = values.find(key);
	if (it == values.end()) {
		set(key, default_value);
	} else {
		value = it->second;
	}
	return value;
}

bool Config::getBool(std::string key, bool default_value) {
	bool ok = default_value;
	auto it = values.find(key);
	if (it == values.end()) {
		set(key, default_value);
	} else {
		std::string& value = it->second;
		if ((value == "TRUE") || (value == "true")) {
			ok = true;
		} else if ((value != "FALSE") && (value != "false")) {
			try {
				ok = std::stoi(value) == 1;
			} catch(...) {
				set(key, false);
			}
		}
	}
	return ok;
}

void Config::set(std::string key, int value) {
	values[key] = std::to_string(value);
	_updated = true;
}

void Config::set(std::string key, float value) {
	values[key] = std::to_string(value);
	_updated = true;
}

void Config::set(std::string key, std::string value) {
	values[key] = value;
	_updated = true;
}

void Config::set(std::string key, bool value) {
	values[key] = value ? "TRUE" : "FALSE";
	_updated = true;
}