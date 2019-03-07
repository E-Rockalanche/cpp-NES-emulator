#include <iostream>
#include <string>
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

Cartridge cartridge;
CPU cpu;
PPU ppu;
APU apu;
Controller controller1, controller2;

const Pixel* surface;

bool loadROM(const char* filename) {
	bool ok = false;
	if (cartridge.loadFile(filename)) {
		cpu.setCartridge(&cartridge);
		ppu.setCartridge(&cartridge);

		cpu.setController(&controller1, 0);
		cpu.setController(&controller2, 1);

		ok = true;
	} else {
		dout("NES could not load " << filename);
	}
	return ok;
}

void reset() {
	cpu.reset();
	ppu.reset();

	cpu._break = true;
}

void step() {
	if (cpu.halted()) {
		std::cout << "HALTED\n";
	} else {
		bool executed_instruction = false;
		while (!executed_instruction) {
			executed_instruction = cpu.clockTick();

			ppu.clockTick();
			ppu.clockTick();
			ppu.clockTick();
		} 
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

void renderScene(void)  {
	while(!cpu.halted() && !cpu._break && !ppu.readyToDraw()) {
		cpu.clockTick();

		ppu.clockTick();
		ppu.clockTick();
		ppu.clockTick();
	}
	glDrawPixels(PPU::SCREEN_WIDTH, PPU::SCREEN_HEIGHT,  GL_RGB, GL_UNSIGNED_BYTE, surface);
	glutSwapBuffers();
}

int main(int argc, char* argv[]) {
	surface = ppu.getSurface();

	cpu.setPPU(&ppu);
	cpu.setAPU(&apu);
	ppu.setCPU(&cpu);

	controller1.keymap[Controller::A] = 'v';
	controller1.keymap[Controller::B] = 'c';
	controller1.keymap[Controller::START] = 13;
	controller1.keymap[Controller::SELECT] = ' ';
	controller1.keymap[Controller::RIGHT] = GLUT_KEY_RIGHT;
	controller1.keymap[Controller::LEFT] = GLUT_KEY_LEFT;
	controller1.keymap[Controller::UP] = GLUT_KEY_UP;
	controller1.keymap[Controller::DOWN] = GLUT_KEY_DOWN;

	std::string filename;
	if (argc > 1) {
		filename = argv[1];
		if (loadROM(filename.c_str())) {
			reset();
			std::cout << "Loaded " << filename << '\n';
		} else {
			std::cout << "Could not load " << filename << '\n';
			return 1;
		}
	} else {
		std::cout << "requires ROM filename\n";
		return 1;
	}

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGB);

	glutInitWindowPosition(100,100);
	glutInitWindowSize(PPU::SCREEN_WIDTH, PPU::SCREEN_HEIGHT);
	glutCreateWindow("NES emulator test");

	glViewport(0, 0, PPU::SCREEN_WIDTH, PPU::SCREEN_HEIGHT);

	glutDisplayFunc(renderScene);
	glutIdleFunc(renderScene);

	glutKeyboardFunc(keyboard);
	glutSpecialFunc(specialKeyboard);
	glutKeyboardUpFunc(keyboardRelease);
	glutSpecialUpFunc(specialRelease);

	glutIgnoreKeyRepeat(GLUT_DEVICE_IGNORE_KEY_REPEAT);
	
	glutMainLoop();

	return 0;
}
