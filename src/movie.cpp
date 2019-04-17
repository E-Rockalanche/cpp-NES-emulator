#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include "movie.hpp"
#include "assert.hpp"
#include "menu_bar.hpp"
#include "cartridge.hpp"
#include "message.hpp"

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
		load_movie_button.enable(cartridge != NULL);
		record_movie_button.enable(cartridge != NULL);
		state = NONE;
		index = 0;
	}

	bool save(std::string filename) {
		if (isRecording()) {
			stopRecording();
		}

		std::ofstream fout(filename.c_str(), std::ios::binary);
		if (!fout.is_open()) {
			showError("Error", "Cannot save movie to " + filename);
			return false;
		}

		unsigned int checksum = cartridge->getChecksum();
		writeBinary(fout, checksum);

		int size = button_presses.size();
		writeBinary(fout, size);

		for(int i = 0; i < size; i++) {
			ButtonPress press = button_presses[i];
			writeBinary(fout, press);
		}

		fout.close();
		return true;
	}

	bool load(std::string filename) {
		stopRecording();
		stopPlayback();
		
		std::ifstream fin(filename.c_str(), std::ios::binary);
		if (!fin.is_open()) {
			showError("Error", "Cannot load movie from " + filename);
			return false;
		}

		button_presses.clear();

		unsigned int checksum = 0;
		readBinary(fin, checksum);
		bool load = true;
		if (checksum != cartridge->getChecksum()) {
			load = askYesNo("Warning",
				"This movie appears to be for a different game. Load anyway?",
				WARNING_ICON);
		}
		if (load) {
			int size = 0;
			readBinary(fin, size);

			for(int i = 0; i < size; i++) {
				ButtonPress press;
				readBinary(fin, press);

				assert(press.button < 8, "loaded button out of bounds");

				button_presses.push_back(press);
			}

			save_movie_button.enable(!empty());
		}
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
			index = 0;
			state = PLAYING;

			play_movie_button.check();
			record_movie_button.disable();
		}
	}

	void stopPlayback() {
		if (isPlaying()) {
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
			showMessage("", "Movie has ended");
		}
	}
}