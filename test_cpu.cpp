#include <iostream>
#include <string>
#include <windows.h>
#include <cstdlib>
#include <ctime>

#include "debugging.hpp"
#include "cpu.hpp"
#include "ppu.hpp"
#include "apu.hpp"
#include "cartridge.hpp"
#include "controller.hpp"

#include <GL/gl.h>
#include <GL/glu.h>
#include "GL/glut.h"

bool good;

CPU cpu;
PPU ppu;
APU apu;
Controller controller1, controller2;

const Pixel* surface;

const unsigned int TARGET_FPS = 60;
const double TIME_PER_FRAME = 1000.0 / TARGET_FPS;
int g_start_time;
int g_current_frame_number;

bool loadFile(const char* filename) {
	Cartridge* cartridge = Cartridge::loadFile(filename);
	if (!cartridge) {
		dout("could not load " << filename);
		return false;
	} else {
		cpu.setCartridge(cartridge);
		ppu.setCartridge(cartridge);
		return true;
	}
}

void reset() {
	cpu.reset();
	ppu.reset();

	for(int i = 0; i < rand() % 4; i++) {
		ppu.clockTick();
	}

	controller1.resetButtons();
	controller2.resetButtons();
}

void power() {
	cpu.power();
	ppu.power();

	for(int i = 0; i < rand() % 4; i++) {
		ppu.clockTick();
	}

	controller1.resetButtons();
	controller2.resetButtons();
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
  b - break\n\
  s - step\n\
  k - breakpoint\n\
  d - dump\n\
  t - dump state\n\
  p - dump ppu\n\
  h - help\n\
==================\n"

void keyboard(unsigned char key, int x, int y)  {
	switch(key) {
		case 'x': // execute
			cpu._break = false;
			cpu.debug = false;
			break;

		case 'r': // reset ROM
			reset();
			break;

		case 'b': // break
			cpu._break = true;
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
			controller1.pressKey(key);
			controller2.pressKey(key);
	}
}

void specialKeyboard(int key, int x, int y) {
	controller1.pressKey(key);
	controller2.pressKey(key);
}

void keyboardRelease(unsigned char key, int x, int y) {
	controller1.releaseKey(key);
	controller2.releaseKey(key);
}

void specialRelease(int key, int x, int y) {
	controller1.releaseKey(key);
	controller2.releaseKey(key);
}

void renderScene()  {
	while(!cpu.halted() && !cpu._break && !ppu.readyToDraw()) {
		cpu.execute();
	}
	glDrawPixels(PPU::SCREEN_WIDTH, PPU::SCREEN_HEIGHT,  GL_RGB, GL_UNSIGNED_BYTE, surface);
	glutSwapBuffers();
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
	
	cpu.setController(&controller1, 0);
	cpu.setController(&controller2, 1);

	controller1.keymap[Controller::A] = 'v';
	controller1.keymap[Controller::B] = 'c';
	controller1.keymap[Controller::START] = 13; // enter
	controller1.keymap[Controller::SELECT] = ' '; // spacebar
	controller1.keymap[Controller::RIGHT] = GLUT_KEY_RIGHT;
	controller1.keymap[Controller::LEFT] = GLUT_KEY_LEFT;
	controller1.keymap[Controller::UP] = GLUT_KEY_UP;
	controller1.keymap[Controller::DOWN] = GLUT_KEY_DOWN;

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

	glutIgnoreKeyRepeat(GLUT_DEVICE_IGNORE_KEY_REPEAT);
	
	glutMainLoop();

	return 0;
}
