#include <iostream>
#include <string>
#include <ctime>
#include <cstdlib>

#include "debugging.hpp"
#include "cpu.hpp"
#include "ppu.hpp"
#include "apu.hpp"
#include "cartridge.hpp"
#include "joypad.hpp"
#include "zapper.hpp"
#include "file_path.hpp"
#include "program_end.hpp"
#include "screen.hpp"

#include <GL/gl.h>
#include <GL/glu.h>
#include "GL/glut.h"

#include <windows.h>

Joypad joypad;
Zapper zapper;

Pixel screen[SCREEN_WIDTH * SCREEN_HEIGHT];

std::string file_path;
std::string file_name;

int window_width = SCREEN_WIDTH * 3;
int window_height = SCREEN_HEIGHT * 3;
int x_offset = 0;
int y_offset = 0;
bool fullscreen = false;

// frame timing
const unsigned int TARGET_FPS = 60;
const double TIME_PER_FRAME = 1000.0 / TARGET_FPS;
int g_start_time;
int g_current_frame_number;

bool loadFile(std::string filename) {
	if (cartridge) delete cartridge;
	cartridge = Cartridge::loadFile(filename);
	if (!cartridge) {
		dout("could not load " << filename);
		return false;
	} else {
		if (cartridge->hasSRAM()) {
			file_path = getPath(filename);
			file_name = getFilename(filename);
			cartridge->loadSave(file_path + file_name + ".sav");
		}
		return true;
	}
}

// close program callback
void saveGame() {
	if (cartridge && cartridge->hasSRAM()) {
		cartridge->saveGame(file_path + file_name + ".sav");
	}
}
ProgramEnd pe(saveGame);

void reset() {
	if (cartridge != NULL) {
		CPU::reset();
		PPU::reset();
	}
}

void power() {
	if (cartridge != NULL) {
		CPU::power();
		PPU::power();
	}
}

void step() {
	if (CPU::halted()) {
		std::cout << "HALTED\n";
	} else {
		CPU::execute();
	}
}

int readAddress() {
	std::string raw;
	std::cin >> raw;
	return std::stoi(raw, NULL, 16);
}

#define HELP "\
====== HELP ======\n\
  x - execute\n\
  r - reset\n\
  q - quit\n\
  b - break\n\
  f - frame step\n\
  s - step\n\
  k - breakpoint\n\
  d - dump\n\
  t - dump state\n\
  p - dump ppu\n\
  h - help\n\
==================\n"

int next_frame = false;

void keyboard(unsigned char key, int x, int y)  {
	switch(key) {
		case 'x': // execute
			CPU::_break = false;
			CPU::debug = false;
			break;

		case 'r': // reset ROM
			reset();
			break;

		case 'q':
		case 27: // escape key
			exit(0);
			break;

		case 'l': {// load rom
			saveGame();
			std::string filename;
			std::cin >> filename;
			if (loadFile(filename)) {
				power();
			}
			break;
		}


		case 'b': // break
			CPU::_break = true;
			break;

		case 'f': // frame step
			CPU::_break = true;
			next_frame = true;
			break;

		case 's': // step
			CPU::debug = true;
			CPU::_break = true;
			step();
			break;

		case 'k': { // set breakpoint
			int address = 0;
			char type;
			std::cout << "breakpoint:\n";
			std::cout << "type (<r>ead, <w>rite, <p>osition, <n>mi, <i>rq): ";
			std::cin >> type;
			CPU::BreakpointCondition condition;
			switch(type) {
				case 'r': condition = CPU::READ_FROM; break;
				case 'w': condition = CPU::WRITE_TO; break;
				case 'p': condition = CPU::PROGRAM_POSITION; break;
				case 'n': condition = CPU::NMI; break;
				case 'i': condition = CPU::IRQ; break;
				default: std::cout << "invalid condition\n";
					return;
			}
			if (condition != CPU::NMI && condition != CPU::IRQ) {
				std::cout << "address: ";
				address = readAddress();
			}
			CPU::addBreakpoint(address, condition);
		} break;

		case 'd': // dump
			std::cout << "dump address: ";
			CPU::dump(readAddress());
			break;

		case 't': // dump stack/state
			CPU::dumpState();
			CPU::dumpStack();
			break;

		case 'p':
			PPU::dump();
			break;

		case 'h':
			std::cout << HELP;
			break;

		case '1' ... '3': {
			int scale = key - '0';
			glutReshapeWindow(SCREEN_WIDTH * scale, SCREEN_HEIGHT * scale);
			} break;

		default:
			joypad.pressKey(key);
	}
}

void specialKeyboard(int key, int x, int y) {
	switch(key) {
		case GLUT_KEY_F11:
			fullscreen = !fullscreen;
			if (fullscreen) glutFullScreen();
			else {
				glutReshapeWindow(SCREEN_WIDTH, SCREEN_HEIGHT);
				glutPositionWindow(100, 100);
			}
		default:
			joypad.pressKey(key);
			break;
	}
}

void keyboardRelease(unsigned char key, int x, int y) {
	joypad.releaseKey(key);
}

void specialRelease(int key, int x, int y) {
	joypad.releaseKey(key);
}

// Zapper
void mouseButton(int button, int state, int x, int y) {
	if (button == GLUT_LEFT_BUTTON) {
		if (state == GLUT_DOWN) {
			zapper.pull();
		}
	}
}
#define scaleX(x) (((float)(x) * SCREEN_WIDTH) / window_width)
#define scaleY(x) (((float)(y) * SCREEN_HEIGHT) / window_height)
void mouseMotion(int x, int y) {
	zapper.aim(scaleX(x - x_offset), scaleY(y - y_offset));
}
void mousePassiveMotion(int x, int y) {
	zapper.aim(scaleX(x - x_offset), scaleY(y - y_offset));
}

#define min(x, y) (((x) < (y)) ? (x) : (y))
void resizeWindow(int width, int height) {
	float x_scale = (float)width / SCREEN_WIDTH;
	float y_scale = (float)height / SCREEN_HEIGHT;
	float scale = min(x_scale, y_scale);

	window_width = scale * SCREEN_WIDTH;
	window_height = scale * SCREEN_HEIGHT;
	glViewport(0, 0, window_width, window_height);
	glPixelZoom(scale, scale);

	x_offset = (width - window_width) / 2;
	y_offset = (height - window_height) / 2;
	glWindowPos2i(x_offset, y_offset);
}

void renderScene()  {
	while(!CPU::halted() && (!CPU::_break || next_frame) && !PPU::readyToDraw()) {
		CPU::execute();
	}
	next_frame = false;
	glDrawPixels(SCREEN_WIDTH, SCREEN_HEIGHT,  GL_RGB, GL_UNSIGNED_BYTE, screen);
	glutSwapBuffers();

	zapper.update();
}

void idle() {
    double end_frame_time, end_rendering_time, waste_time;
    glutPostRedisplay();

    // wait until it is time to draw the current frame
    end_frame_time = g_start_time + (g_current_frame_number + 1) * TIME_PER_FRAME;
    end_rendering_time = glutGet(GLUT_ELAPSED_TIME);
    waste_time = end_frame_time - end_rendering_time;
    if (waste_time > 0.0) {
    	Sleep(waste_time / 1000.0);    // sleep parameter should be in seconds
    }

    // update frame number
    g_current_frame_number = g_current_frame_number + 1;
}

int main(int argc, char* argv[]) {
	srand(time(NULL));

	controller_ports[0] = &joypad;
	controller_ports[1] = &zapper;
	CPU::init();

	joypad.keymap[Joypad::A] = 'v';
	joypad.keymap[Joypad::B] = 'c';
	joypad.keymap[Joypad::START] = 13; // enter
	joypad.keymap[Joypad::SELECT] = ' '; // spacebar
	joypad.keymap[Joypad::RIGHT] = GLUT_KEY_RIGHT;
	joypad.keymap[Joypad::LEFT] = GLUT_KEY_LEFT;
	joypad.keymap[Joypad::UP] = GLUT_KEY_UP;
	joypad.keymap[Joypad::DOWN] = GLUT_KEY_DOWN;

	std::string filename;
	if (argc > 1) {
		filename = argv[1];
		if (loadFile(filename.c_str())) {
			power();
			std::cout << "Loaded " << filename << '\n';
		} else {
			std::cout << "Could not load " << filename << '\n';
			return 1;
		}
	} else {
		std::cout << "requires ROM filename\n";
		return 1;
	}

	g_start_time = glutGet(GLUT_ELAPSED_TIME);
    g_current_frame_number = 0;

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGB);

	glutInitWindowPosition(100,100);
	glutInitWindowSize(window_width, window_height);
	glutCreateWindow("NES emulator");

	glViewport(0, 0, window_width, window_height);

	glutDisplayFunc(renderScene);
	glutIdleFunc(idle);

	glutKeyboardFunc(keyboard);
	glutSpecialFunc(specialKeyboard);
	glutKeyboardUpFunc(keyboardRelease);
	glutSpecialUpFunc(specialRelease);

	glutMouseFunc(mouseButton);
	glutMotionFunc(mouseMotion);
	glutPassiveMotionFunc(mousePassiveMotion);

	glutReshapeFunc(resizeWindow);

	glutIgnoreKeyRepeat(GLUT_DEVICE_IGNORE_KEY_REPEAT);
	
	glutMainLoop();

	return 0;
}
