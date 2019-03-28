#ifndef FILE_PATH_HPP
#define FILE_PATH_HPP

#include <string>

std::string getFilename(std::string path);
std::string getFileExt(std::string filename);
std::string getPath(std::string filename);

#endif

std::string getFilename(std::string path) {
	std::string filename = "";
	unsigned int end = path.size();
	unsigned int i = path.size();
	while(i > 0) {
		i--;
		if ((path[i] == '.') && (end == path.size())) {
		end = i;
		}
		if ((path[i] == '/') || (path[i] == '\\')) {
			i++;
			break;
		}
	}
	for(; i < end; i++) {
		filename += path[i];
	}
	return filename;
}

std::string getFileExt(std::string filename) {
	std::string ext = "";
	unsigned int i = filename.size();
	while(i > 0) {
		i--;
		if (filename[i] == '.') {
			i++;
			break;
		}
	}
	for(; i < filename.size(); i++) {
		ext += filename[i];
	}
	return ext;
}

std::string getPath(std::string filename) {
	std::string path;
	std::string buffer;
	for(unsigned int i = 0; i < filename.size(); i++) {
		char c = filename[i];
		buffer += c;
		if ((c == '/') || (c == '\\')) {
			path += buffer;
			buffer = "";
		}
	}
	return path;
}