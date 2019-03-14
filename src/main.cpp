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

#include <GL/gl.h>
#include <GL/glu.h>
#include "GL/glut.h"

#include <windows.h>

bool good;

CPU cpu;
PPU ppu;
APU apu;
Joypad joypad;
Zapper zapper;
Cartridge* cartridge = NULL;

std::string file_path;
std::string file_name;

std::string rom_folder;
std::string save_folder;

const Pixel* surface;

const unsigned int TARGET_FPS = 60;
const double TIME_PER_FRAME = 1000.0 / TARGET_FPS;
int g_start_time;
int g_current_frame_number;

bool loadFile(std::string filename) {
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
		cpu.setCartridge(cartridge);
		ppu.setCartridge(cartridge);
		cartridge->setCPU(&cpu);
		return true;
	}
}

// close program callback
void quit() {
	if (cartridge && cartridge->hasSRAM()) {
		cartridge->saveGame(file_path + file_name + ".sav");
	}
}
ProgramEnd pe(quit);

void reset() {
	cpu.reset();
	ppu.reset();

	joypad.reset();
	zapper.reset();
}

void power() {
	cpu.power();
	ppu.power();

	joypad.reset();
	zapper.reset();
}

void step() {
	if (cpu.halted()) {
		std::cout << "HALTED\n";
	} else {
		cpu.execute();
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
			cpu._break = false;
			cpu.debug = false;
			break;

		case 'r': // reset ROM
			reset();
			break;

		case 'q':
		case 27: // escape key
			exit(0);
			break;

		case 'b': // break
			cpu._break = true;
			break;

		case 'f': // frame step
			cpu._break = true;
			next_frame = true;
			break;

		case 's': // step
			cpu.debug = true;
			cpu._break = true;
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
			cpu.addBreakpoint(address, condition);
		} break;

		case 'd': // dump
			std::cout << "dump address: ";
			cpu.dump(readAddress());
			break;

		case 't': // dump stack/state
			cpu.dumpState();
			cpu.dumpStack();
			break;

		case 'p':
			ppu.dump();
			break;

		case 'h':
			std::cout << HELP;
			break;

		default:
			joypad.pressKey(key);
	}
}

void specialKeyboard(int key, int x, int y) {
	joypad.pressKey(key);
}

void keyboardRelease(unsigned char key, int x, int y) {
	joypad.releaseKey(key);
}

void specialRelease(int key, int x, int y) {
	joypad.releaseKey(key);
}

void mouseButton(int button, int state, int x, int y) {
	if (button == GLUT_LEFT_BUTTON) {
		if (state == GLUT_DOWN) {
			zapper.pull();
		}
	}
}

void mouseMotion(int x, int y) {
	zapper.aim(x, y);
}
void mousePassiveMotion(int x, int y) {
	zapper.aim(x, y);
}

void renderScene()  {
	while(!cpu.halted() && (!cpu._break || next_frame) && !ppu.readyToDraw()) {
		cpu.execute();
	}
	next_frame = false;
	glDrawPixels(PPU::SCREEN_WIDTH, PPU::SCREEN_HEIGHT,  GL_RGB, GL_UNSIGNED_BYTE, surface);
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

	surface = ppu.getSurface();

	cpu.setPPU(&ppu);
	cpu.setAPU(&apu);
	ppu.setCPU(&cpu);

	zapper.setScreen(surface);
	
	cpu.setController(&joypad, 0);
	cpu.setController(&zapper, 1);

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
	glutInitWindowSize(PPU::SCREEN_WIDTH, PPU::SCREEN_HEIGHT);
	glutCreateWindow("NES emulator test");

	glViewport(0, 0, PPU::SCREEN_WIDTH, PPU::SCREEN_HEIGHT);

	glutDisplayFunc(renderScene);
	glutIdleFunc(idle);

	glutKeyboardFunc(keyboard);
	glutSpecialFunc(specialKeyboard);
	glutKeyboardUpFunc(keyboardRelease);
	glutSpecialUpFunc(specialRelease);

	glutMouseFunc(mouseButton);
	glutMotionFunc(mouseMotion);
	glutPassiveMotionFunc(mousePassiveMotion);

	glutIgnoreKeyRepeat(GLUT_DEVICE_IGNORE_KEY_REPEAT);
	
	glutMainLoop();

	return 0;
}
