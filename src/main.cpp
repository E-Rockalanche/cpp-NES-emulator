// standard library
#include <iostream>
#include <string>
#include <ctime>
#include <cstdlib>

// nes
#include "common.hpp"
#include "main.hpp"
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
#include "config.hpp"

// graphics
#include "SDL2/SDL.h"

Joypad joypad[4];
Zapper zapper;

const int SCREEN_BPP = 24; // bits per pixel
Pixel screen[SCREEN_WIDTH * SCREEN_HEIGHT];

// paths
std::string file_name = "";
std::string save_path = "./";
std::string rom_path = "./";
std::string screenshot_path = "./";

// window size
bool fullscreen = false;
float window_scale = 1;
int screen_width = SCREEN_WIDTH;
int screen_height = SCREEN_HEIGHT;
int x_offset = 0;
int y_offset = 0;

// frame timing
const unsigned int TARGET_FPS = 60;
const double TIME_PER_FRAME = 1000.0 / TARGET_FPS;
int last_time = 0;
int last_render_time = 0;
double last_wait_time = 0;

// frame rate
int total_frames = 0;
float fps = 0;
float real_fps = 0;
float total_fps = 0;
float total_real_fps = 0;
#define ave_fps (total_fps / total_frames)
#define ave_real_fps (total_real_fps / total_frames)

Sound_Queue* sound_queue = NULL;
void newSamples(const blip_sample_t* samples, size_t count)
{
    sound_queue->write(samples, count);
}

#define CONFIG_FILE "nes.cfg"
Config config;
void loadConfig() {
	dout("load config");

	if (!config.load(CONFIG_FILE)) {
		dout("could not load configuration file");
	} else dout("setting variables");

	// ppu options
	PPU::sprite_flickering = config.getBool("sprite_flickering", true);

	// file options
	rom_path = config.getString("rom_path", "./");
	save_path = config.getString("save_path", "./");
	screenshot_path = config.getString("screenshot_path", "./");

	// window options
	fullscreen = config.getBool("fullscreen", false);
	window_scale = config.getFloat("window_scale", 2.0);

	if (config.updated()) {
		dout("saving config");
		config.save(CONFIG_FILE);
	}
	
	/*
	if (fullscreen) glutFullScreen();
	else glutReshapeWindow(SCREEN_WIDTH * window_scale,
						   SCREEN_HEIGHT * window_scale);
	*/
}

bool loadFile(std::string filename) {
	if (cartridge) delete cartridge;
	cartridge = Cartridge::loadFile(filename);
	if (!cartridge) {
		dout("could not load " << filename);
		return false;
	} else {
		if (cartridge->hasSRAM()) {
			file_name = getFilename(filename);
			cartridge->loadSave(save_path + file_name + ".sav");
		}
		return true;
	}
}

bool loadSave(std::string filename) {
	if (cartridge && cartridge->hasSRAM()) {
		return cartridge->loadSave(filename);
	} else {
		return false;
	}
}

// close program callback
void saveGame() {
	if (cartridge && cartridge->hasSRAM()) {
		cartridge->saveGame(save_path + file_name + ".sav");
	}
	config.save();
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

/*
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

		case 'i':
			PPU::sprite_flickering = !PPU::sprite_flickering;
			std::cout << "sprite flickering: " << (PPU::sprite_flickering ? "ON" : "OFF");
			break;


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

		case 'a':
			std::cout << "ave fps: " << ave_fps << '\n';
			std::cout << "ave real fps: " << ave_real_fps << '\n';
			break;

		case 'h':
			std::cout << HELP;
			break;

		case '1' ... '3':
			window_scale = key - '0';
			glutReshapeWindow(SCREEN_WIDTH * window_scale,
							  SCREEN_HEIGHT * window_scale);
			break;

		default:
			for(int i = 0; i < 4; i++) joypad[i].pressKey(key);
	}
}

void specialKeyboard(int key, int x, int y) {
	switch(key) {
		case GLUT_KEY_F11:
			fullscreen = !fullscreen;
			if (fullscreen) glutFullScreen();
			else {
				glutReshapeWindow(SCREEN_WIDTH * window_scale,
								  SCREEN_HEIGHT * window_scale);
				glutPositionWindow(100, 100);
			}
			break;

		default:
			for(int i = 0; i < 4; i++) joypad[i].pressKey(key);
			break;
	}
}

void keyboardRelease(unsigned char key, int x, int y) {
	for(int i = 0; i < 4; i++) joypad[i].releaseKey(key);
}

void specialRelease(int key, int x, int y) {
	for(int i = 0; i < 4; i++) joypad[i].releaseKey(key);
}

// Zapper
void mouseButton(int button, int state, int x, int y) {
	if (button == GLUT_LEFT_BUTTON) {
		if (state == GLUT_DOWN) {
			zapper.pull();
		}
	}
}
#define scaleX(x) (((float)(x) * SCREEN_WIDTH) / screen_width)
#define scaleY(x) (((float)(y) * SCREEN_HEIGHT) / screen_height)
void mouseMotion(int x, int y) {
	zapper.aim(scaleX(x - x_offset), scaleY(y - y_offset));
}
void mousePassiveMotion(int x, int y) {
	zapper.aim(scaleX(x - x_offset), scaleY(y - y_offset));
}

void resizeWindow(int window_width, int window_height) {
	// calculate largest screen scale in current window size
	float x_scale = (float)window_width / SCREEN_WIDTH;
	float y_scale = (float)window_height / SCREEN_HEIGHT;
	float scale = MIN(x_scale, y_scale);

	// set new screen size
	screen_width = scale * SCREEN_WIDTH;
	screen_height = scale * SCREEN_HEIGHT;
	glViewport(0, 0, screen_width, screen_height);
	glPixelZoom(scale, scale);

	// center screen in window
	x_offset = (window_width - screen_width) / 2;
	y_offset = (window_height - screen_height) / 2;
	glWindowPos2i(x_offset, y_offset);
}

void renderScene()  {
	CPU::runFrame();

	glDrawPixels(SCREEN_WIDTH, SCREEN_HEIGHT,  GL_RGB, GL_UNSIGNED_BYTE, screen);
	glutSwapBuffers();

	zapper.update();
	total_frames++;
}

void idle() {
	int current_time = glutGet(GLUT_ELAPSED_TIME);
	int elapsed_time = current_time - last_time;

	double wait_time = TIME_PER_FRAME - elapsed_time;
	// reduce wait time if last frame took too long
	if (last_wait_time < 0.0) {
		wait_time += last_wait_time;
	}
	last_wait_time = wait_time;

    real_fps = TIME_PER_FRAME / (current_time - last_render_time) * TARGET_FPS;
    fps = TIME_PER_FRAME / (current_time - last_time) * TARGET_FPS;
    total_fps += fps;
    total_real_fps += real_fps;

	if (wait_time >= 0.0) {
		Sleep(wait_time);
	}

	last_render_time = glutGet(GLUT_ELAPSED_TIME);
	last_time = current_time;

	glutPostRedisplay();
}


void initializeGlut(int& argc, char* argv[]) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGB);

	glutInitWindowPosition(100,100);
	glutInitWindowSize(screen_width, screen_height);
	glutCreateWindow("NES emulator");

	glViewport(0, 0, screen_width, screen_height);

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
}
*/

int main(int argc, char* argv[]) {
	srand(time(NULL));

	loadConfig();

	assert(SDL_Init(SDL_INIT_VIDEO) == 0, "failed to initialize SDL");

	SDL_Window* sdl_window = SDL_CreateWindow("NES emulator",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		SCREEN_WIDTH, SCREEN_HEIGHT,
		SDL_WINDOW_OPENGL);
	assert(sdl_window != NULL, "failed to create screen");

	SDL_Renderer* sdl_renderer = SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_PRESENTVSYNC);

	SDL_Texture* sdl_texture = SDL_CreateTexture(sdl_renderer,
		SDL_PIXELFORMAT_RGB888,
		SDL_TEXTUREACCESS_STREAMING,
		SCREEN_WIDTH, SCREEN_HEIGHT);

	controller_ports[0] = &joypad[0];
	controller_ports[1] = &zapper;
	CPU::init();
	APU::init();
    sound_queue = new Sound_Queue;
    sound_queue->init(96000);

    /*
	joypad[0].mapButtons((const int[8]){ 'v', 'c', ' ', 13,
		GLUT_KEY_UP, GLUT_KEY_DOWN, GLUT_KEY_LEFT, GLUT_KEY_RIGHT });
	*/

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

	// last_time = glutGet(GLUT_ELAPSED_TIME);

	// glutMainLoop();

	SDL_UpdateTexture(sdl_texture, NULL, screen, SCREEN_WIDTH * sizeof (Pixel));

	SDL_RenderClear(sdl_renderer);
	SDL_RenderCopy(sdl_renderer, sdl_texture, NULL, NULL);
	SDL_RenderPresent(sdl_renderer);

	SDL_Quit();
	dout("Goodbye!");
	return 0;
}
