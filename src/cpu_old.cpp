#include <iomanip>
#include <iostream>
#include <bitset>

#include "Nes_Apu.h"

#include "cpu.hpp"
#include "debugging.hpp"
#include "common.hpp"
#include "controller.hpp"
#include "apu.hpp"

namespace CPU
{
	bool _break;
	bool debug = false;

#define debugOperation(pc, opcode) if (debug) std::cout \
	<< "[" << toHex(pc, 2) \
	<< "] op: " << toHex(opcode, 1) << ", " \
	<< instruction_names[operations[opcode].instruction] << ' ' \
	<< ((operations[opcode].address_mode != IMPLIED) \
		? address_mode_names[operations[opcode].address_mode] \
		: "") \
	<< '\n'

#define debugSpecific(message) if (debug) std::cout \
	<< message << '\n'

#define debugStatus(flag) if (debug) std::cout \
	<< #flag << ": " \
	<< getStatusFlag(flag) << '\n'

int test_ticks;




typedef void (*InstructionFunction)(AddressMode);

enum ByteInterpretation {
	RAW,
	OPCODE,
	ASCII,
	NUM_BYTE_INTERPRETATIONS
};

int wait_cycles = 0;

Byte ram[RAM_SIZE];

Byte accumulator;
Byte x_register;
Byte y_register;
Word program_counter;
Byte stack_pointer;
Byte status;

bool odd_cycle;

bool _halt;
int _nmi; // time since nmi was set
int _irq; // time since irq was set

int cycles;






} // end namespace