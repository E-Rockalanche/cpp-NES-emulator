#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <map>

class Config {
public:
	Config() {}
	Config(std::string filename);
	bool load(std::string filename);
	bool save(std::string filename);
	bool save();

	int getInt(std::string key);
	float getFloat(std::string key);
	std::string getString(std::string key);
	bool getBool(std::string key);

	void set(std::string key, int value);
	void set(std::string key, float value);
	void set(std::string key, std::string value);
	void set(std::string key, bool value);
private:
	std::map<std::string, std::string> values;
	std::string filename;
};

#endif