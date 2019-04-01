#ifndef MOVIE_HPP
#define MOVIE_HPP

#include <string>
#include "controller.hpp"
#include "joypad.hpp"

namespace Movie {
	enum State {
		NONE,
		RECORDING,
		PLAYING
	};

	bool save(std::string filename);
	bool load(std::string filename);

	State getState();

	void startRecording();
	void stopRecording();
	void recordButtonPress(int frame, int joypad, Joypad::Button button);

	void startMovie();
	void stopMovie();
	void updateInput(int frame);
}

#endif