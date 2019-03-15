#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <map>

class Config {
public:
	Config();
	Config(std::string filename);
	bool load(std::string filename);
	bool save(std::string filename);
	bool save();
	bool updated();

	int getInt(std::string key, int default_value = 0);
	float getFloat(std::string key, float default_value = 0);
	std::string getString(std::string key, std::string default_value = "");
	bool getBool(std::string key, bool default_value = false);

	void set(std::string key, int value);
	void set(std::string key, float value);
	void set(std::string key, std::string value);
	void set(std::string key, bool value);

private:
	std::map<std::string, std::string> values;
	std::string filename;
	bool _updated = false;
};

#endif