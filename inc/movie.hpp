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
	bool isRecording();
	void recordButtonState(int frame, int joypad, Joypad::Button button, bool pressed);

	void startPlayback();
	void stopPlayback();
	bool isPlaying();
	void updateInput(int frame);
};

#endif