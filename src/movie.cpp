#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include "movie.hpp"
#include "assert.hpp"

namespace Movie {
	struct ButtonPress {
		int frame;
		int joypad;
		Joypad::Button button;
		bool pressed;
	};

	State state = NONE;
	std::vector<ButtonPress> button_presses;
	int index = 0;

	bool save(std::string filename) {
		std::fstream fout(filename.c_str(), std::ios::binary);
		if (!fout.is_open()) {
			dout("cannot save movie");
			return false;
		}

		int size = button_presses.size();
		fout.write((char*)&size, sizeof(size));

		for(int i = 0; i < size; i++) {
			ButtonPress press = button_presses[i];
			fout.write((char*)&press, sizeof(press));
		}

		fout.close();
		return true;
	}

	bool load(std::string filename) {
		std::fstream fin(filename.c_str(), std::ios::binary);
		if (!fin.is_open()) {
			dout("cannot open movie");
			return false;
		}

		button_presses.clear();

		int size;
		fin.read((char*)&size, sizeof(size));

		for(int i = 0; i < size; i++) {
			ButtonPress press;
			fin.read((char*)&press, sizeof(press));
			button_presses.push_back(press);
		}

		fin.close();
		return true;
	}

	State getState() {
		return state;
	}

	void startRecording() {
		assert(state == NONE, "cannot start recording movie");
		dout("recording movie");

		button_presses.clear();
		state = RECORDING;
	}

	void stopRecording() {
		assert(state == RECORDING, "not recording movie");
		dout("stopping recording");

		state = NONE;
	}

	bool isRecording() {
		return state == RECORDING;
	}
	
	void recordButtonState(int frame, int joypad, Joypad::Button button, bool pressed) {
		assert(state == RECORDING, "cannot record button presses while not recording");
		button_presses.push_back({ frame, joypad, button, pressed });
	}

	void startPlayback() {
		assert(state == NONE, "cannot start playing movie");
		dout("playing movie");

		index = 0;
		state = PLAYING;
	}

	void stopPlayback() {
		assert(state == PLAYING, "not playing movie");
		dout("stopping movie");

		state = NONE;
	}

	bool isPlaying() {
		return state == PLAYING;
	}

	void updateInput(int frame) {
		while(index < (int)button_presses.size()) {
			const ButtonPress& press = button_presses[index];
			if (press.frame < frame) {
				index++;
			} else if (press.frame == frame) {
				joypad[press.joypad].setButtonState(press.button, press.pressed);
				index++;
			} else {
				break;
			}
		}
		
		if (index >= (int)button_presses.size()) {
			// stop playback
			state = NONE;
		}
	}
}