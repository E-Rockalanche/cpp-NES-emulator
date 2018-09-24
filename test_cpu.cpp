#include <iostream>
#include <string>
#include "debugging.hpp"
#include "nes.hpp"
#include "cpu.hpp"

#define TEST_FPS true

#define COMMANDS "\
==========================\n\
 L: Load ROM\n\
 R: Reset\n\
 X: Execute\n\
 D: Add debug Operation\n\
 C: Clear debug operations\n\
 U: Dump\n\
 Q: Quit test\n\
==========================\n\
"

int readAddress() {
	std::string raw;
	std::cin >> raw;
	return std::stoi(raw, NULL, 16);
}

int main(int argc, char** argv) {
	NES nes;

	CPU* cpu = nes.getCPU();

	std::string filename;
	if (argc > 1) {
		filename = argv[1];
		if (nes.loadFile(filename.c_str())) {
			nes.reset();
			std::cout << "Loaded " << filename << '\n';
		} else {
			std::cout << "Could not load " << filename << '\n';
		}
	}

	char command;
	
	CPU::DebugOperationType type;
	CPU::DebugOperationCondition condition;
	Word address;
	
	do {
		std::cin >> command;
		switch(command) {
			case 'l':
			case 'L':
				std::cin >> filename;
				if (nes.loadFile(filename.c_str())) {
					nes.reset();
				} else {
					std::cout << "Could not load " << filename << '\n';
				}
				break;

			case 'r':
			case 'R':
				nes.reset();
				break;

			case 'x':
			case 'X':
				std::cout << "executing\n";

				if (nes.execute()) {
					std::cout << "breaked\n";
				} else {
					std::cout << "crashed\n";
				}

				break;

			case 'd':
			case 'D':
				std::cout << "type: <b>reakpoint, <d>ump, d<i>splay ops: ";
				std::cin >> command;
				switch(command) {
					default:
					case 'b': type = CPU::BREAKPOINT; break;
					case 'd': type = CPU::DUMP; break;
					case 'i': type = CPU::DISPLAY_OPS; break;
				}
				std::cout << "condition: <p>rogram counter, <r>ead, <w>rite: ";
				std::cin >> command;
				switch(command) {
					default:
					case 'p': condition = CPU::PROGRAM_POSITION; break;
					case 'r': condition = CPU::READ_FROM; break;
					case 'w': condition = CPU::WRITE_TO; break;
				}
				std::cout << "address: ";
				address = readAddress();
				cpu->addDebugOperation(type, condition, address);
				break;

			case 'c':
			case 'C':
				cpu->clearDebugOperations();
				std::cout << "Cleared debug operations\n";
				break;

			case 'u':
			case 'U':
				std::cout << "address: ";
				address = readAddress();
				cpu->dumpState();
				cpu->dumpStack();
				cpu->dump(address);
				break;

			case 'q':
			case 'Q':
				std::cout << "Goodbye\n";
				break;

			case 'h':
			case 'H':
				std::cout << COMMANDS;
				break;

			default:
				std::cout << "Invalid command\n";
		}
	}while(command != 'q' && command != 'Q');

	return 0;
}