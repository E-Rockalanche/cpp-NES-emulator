#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include "movie.hpp"
#include "assert.hpp"
#include "menu.hpp"

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

	bool empty() { return button_presses.empty(); }
	void clear() {
		button_presses.clear();
		save_movie_button.disable();
		play_movie_button.disable();
		state = NONE;
		index = 0;
	}

	bool save(std::string filename) {
		if (isRecording()) {
			stopRecording();
		}

		std::ofstream fout(filename.c_str(), std::ios::binary);
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
		stopRecording();
		stopPlayback();
		
		std::ifstream fin(filename.c_str(), std::ios::binary);
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

		save_movie_button.enable(!empty());
		fin.close();
		return true;
	}

	State getState() {
		return state;
	}

	void startRecording() {
		if (state == NONE) {
			dout("recording movie");

			button_presses.clear();
			state = RECORDING;

			save_movie_button.disable();
			play_movie_button.disable();
			record_movie_button.check();
		}
	}

	void stopRecording() {
		if (isRecording()) {
			dout("stopping recording");

			state = NONE;

			save_movie_button.enable();
			play_movie_button.enable();
			record_movie_button.uncheck();
		}
	}

	bool isRecording() {
		return state == RECORDING;
	}
	
	void recordButtonState(int frame, int joypad, Joypad::Button button, bool pressed) {
		assert(state == RECORDING, "cannot record button presses while not recording");
		button_presses.push_back({ frame, joypad, button, pressed });
	}

	void startPlayback() {
		if (state == NONE && !empty()) {
			dout("playing movie");

			index = 0;
			state = PLAYING;

			play_movie_button.check();
			record_movie_button.disable();
		}
	}

	void stopPlayback() {
		if (isPlaying()) {
			dout("stopping movie");

			state = NONE;

			play_movie_button.uncheck();
			record_movie_button.enable();
		}
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
			dout("movie has ended at frame " << frame);
		}
	}
}