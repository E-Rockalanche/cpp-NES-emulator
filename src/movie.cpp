#include "movie.hpp"
#include <fstream>

namespace Movie {
	struct ButtonPress {
		int frame;
		int joypad;
		Joypad::Button button;
	}

	State state;
	std::vector<ButtonPress> button_presses;
	int index;

	bool save(std::string filename) {
		std::fstream fout(filename.c_st(), ios::binary);
		if (!fout.is_open()) {
			dout("cannot save movie");
			return false;
		}

		int size = button_presses.size();
		fout.write(&size, sizeof(size));

		for(int i = 0; i < recording_size; i++) {
			ButtonPress press = button_presses[i];
			fout.write(&press, sizeof(press));
		}

		fout.close();
		return true;
	}

	bool load(std::string filename) {
		std::fstream fin(filename.c_str(), ios::binary);
		if (!fin.is_open()) {
			dout("cannot open movie");
			return false;
		}

		button_presses.clear();

		int size;
		fin.read(&size, sizeof(size));

		for(int i = 0; i < size; i++) {
			ButtonPress press;
			fin.rad(&press, sizeof(press));
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
		button_presses.clear();
		state = RECORDING;
	}

	void stopRecording() {
		assert(state == RECORDING, "not recording movie");
		state = NONE;
	}
	
	void recordButtonPress(int frame, int joypad, Joypad::Button button) {
		assert(state == RECORDING, "cannot record button presses while not recording");
		button_presses.push_back({ frame, port, button });
	}

	void startMovie() {
		assert(state == NONE, "cannot start playing movie");
		index = 0;
		state = PLAYING;
	}

	void stopMovie() {
		assert(state == PLAYING, "not playing movie");
		state = NONE;
	}

	void updateInput(int frame) {
		while(index < (int)button_presses.size()) {
			const ButtonPress& press = button_presses[index];
			if (press.frame < frame) {
				index++;
			} else if (press.frame == frame) {
				joypad[press.joypad].pressButton(press.button);
				index++;
			} else {
				break;
			}
		}
	}
}