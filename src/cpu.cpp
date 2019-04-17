#include <iomanip>
#include <iostream>
#include <bitset>

#include "Nes_Apu.h"

#include "cpu.hpp"
#include "debugging.hpp"
#include "common.hpp"
#include "controller.hpp"
#include "apu.hpp"

namespace CPU {
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

bool in_nmi = false;

#define crossPage(addr1, addr2) (((addr1) & 0xff00) != ((addr2) & 0xff00))
#define isNegative(byte) ((bool)((byte) & 0x80))
#define twosComp(byte) ((Byte)(1 + ~(byte)))
#define setOverflow(reg, value, result) \
	setStatusFlag(OVERFLOW, ~((reg) ^ (value)) & ((reg) ^ (result)) & 0x80)

enum Instruction {
	ADC, // add with carry
	AND, // bitwise and
	ASL, // shift left one bit

	BCC, // branch on carry clear (C = 0)
	BCS, // branch on carry set (C = 1)
	BEQ, // branch on equal to zero (Z = 1)
	BIT, // test bits in memory with acc
	BMI, // branch on minus (N = 1)
	BNE, // branch on not equal to zero (Z = 0)
	BPL, // branch on plus (N = 0)
	BRK, // force break
	BVC, // branch on overflow clear (V = 0)
	BVS, // branch on overflow set (V = 1)

	CLC, // clear carry flag
	CLD, // clear decimal mode
	CLI, // clear interrupt disable bit
	CLV, // clear overflow flag
	CMP, // compare memory and acc
	CPX, // compare memory and index x
	CPY, // compare memory and index y

	DEC, // dedcrement memory by 1
	DEX, // decrement index x by 1
	DEY, // decrement index y by 1

	EOR, // exclusive or memory with acc

	INC, // increment memory by 1
	INX, // increment index x by 1
	INY, // increment index y by 1

	JMP, // jump to location
	JSR, // jump to subroutine

	LDA, // load acc with memory
	LDX, // load index x with memory
	LDY, // load index y with memory
	LSR, // shift right one bit

	NOP, // no operation

	ORA, // "or" memory with acc

	PHA, // push acc on stack
	PHP, // push status on stack
	PLA, // pop acc from stack
	PLP, // pop status from stack

	ROL, // rotate one bit left
	ROR, // rotate one bit right
	RTI, // return from interrupt
	RTS, // return from subroutine

	SBC, // subtract memory from acc with borrow
	SEC, // set carry flag
	SED, // set decimal mode
	SEI, // set interrupt disable flag
	STA, // store acc in memory
	STX, // store index x in memory
	STY, // store index y in memory

	TAX, // transfer acc to index x
	TAY, // transfer acc to index y
	TSX, // transfer stack pointer to index x
	TXA, // transfer index x to acc
	TXS, // transfer index x to stack pointer
	TYA, // transfer index y to acc

	ILL, // illegal opcodes

	NUM_INSTRUCTIONS
};


const char* instruction_names[NUM_INSTRUCTIONS] = {
	"ADC",
	"AND",
	"ASL",

	"BCC",
	"BCS",
	"BEQ",
	"BIT",
	"BMI",
	"BNE",
	"BPL",
	"BRK",
	"BVC",
	"BVS",

	"CLC",
	"CLD",
	"CLI",
	"CLV",
	"CMP",
	"CPX",
	"CPY",

	"DEC",
	"DEX",
	"DEY",

	"EOR",

	"INC",
	"INX",
	"INY",

	"JMP",
	"JSR",

	"LDA",
	"LDX",
	"LDY",
	"LSR",

	"NOP",

	"ORA",

	"PHA",
	"PHP",
	"PLA",
	"PLP",

	"ROL",
	"ROR",
	"RTI",
	"RTS",

	"SBC",
	"SEC",
	"SED",
	"SEI",
	"STA",
	"STX",
	"STY",

	"TAX",
	"TAY",
	"TSX",
	"TXA",
	"TXS",
	"TYA",

	"ILL"
};

enum AddressMode {
	IMMEDIATE,
	/*
	operand's value given in instruction (next byte)
	ex) LDA #$0a
	*/

	ABSOLUTE,
	/*
	operands address is given
	ex) LDA $31f6
	*/

	ZERO_PAGE,
	/*
	only low byte required
	ex) LDA $f4
	*/

	ZERO_PAGE_X,
	/*
	address given added to value in x index
	ex) LDA $20, x
	*/

	ZERO_PAGE_Y,
	/*
	address given added to value in y index
	ex) LDA $20, y
	*/

	IMPLIED,
	/*
	no operand addresses are required
	ex) TAX
	*/

	ACCUMULATOR,
	/*
	instruction operates on data in accumulator
	ex) LSR
	*/

	ABSOLUTE_X,
	ABSOLUTE_X_STORE, // always an extra cycle regardless of page cross
	/*
	address given added to value in x index
	ex) LDA $31f6, x
	*/

	ABSOLUTE_Y,
	ABSOLUTE_Y_STORE, // always an extra cycle regardless of page cross
	/*
	address given added to value in y index
	ex) LDA $31f6, y
	*/

	INDIRECT,
	/*
	used with jump instruction
	operand is address of value where new address is stored
	ex) JMP ($215f)
	[$215f] = $76
	[$2160] = $30
	*/

	INDIRECT_X,
	/*
	zero-page address is added to contents of x register to give the address
	of the bytes holding to address of the operand
	ex) LDA ($3e, x)
	[x-register] = $05
	[$0043] = $15
	[$0044] = $24
	[$2415] = $6e
	*/

	INDIRECT_Y,
	INDIRECT_Y_STORE, // always an extra cycle regardless of page cross
	/*
	contents of zero page address (and following byte) give the indirect address
	which is added to the contents of the y register to yield the actual address
	of the operand
	ex) LDA ($4c), y
	[$004c] = $00
	[$004d] = $21
	[y-register] = $05
	[$2105] = $6d
	*/

	RELATIVE_MODE,
	/*
	used with branch on condition instructions. A 1 byte value is added to the
	program counter. The 1 byte is treated as a signed number
	ex) BEQ $a7
	*/

	NUM_ADDRESS_MODES
};

const char* address_mode_names[NUM_ADDRESS_MODES] = {
	"IMMEDIATE",
	"ABSOLUTE",
	"ZERO_PAGE",
	"ZERO_PAGE_X",
	"ZERO_PAGE_Y",
	"IMPLIED",
	"ACCUMULATOR",
	"ABSOLUTE_X",
	"ABSOLUTE_X (store)",
	"ABSOLUTE_Y",
	"ABSOLUTE_Y (store)",
	"INDIRECT",
	"INDIRECT_X",
	"INDIRECT_Y",
	"INDIRECT_Y (store)",
	"RELATIVE",
};

enum StatusFlag {
	CARRY,
	ZERO,
	DISABLE_INTERRUPTS,
	DECIMAL,
	BREAK,
	UNUSED,
	OVERFLOW,
	NEGATIVE
};

const int RAM_START = 0x0000;
const int RAM_SIZE = 0x0800;
const int RAM_END = 0x1fff;
/*
memory from 0x0800-0x1fff mirrors internal ram memory
*/


const int PPU_START = 0x2000;
const int PPU_SIZE = 0x0008;
const int PPU_END = 0x3fff;
/*
memory from 0x2008-0x3fff mirrors ppu registers
*/

const int OAM_DMA = 0x4014;

const int APU_START = 0x4000;
const int APU_SIZE = 0x0016;
const int APU_END = 0x4015;

const int APU_STATUS = 0x4015;
const int APU_FRAME_COUNT = 0x4017;

const int JOY1 = 0x4016;
const int JOY2 = 0x4017;
/*
memory from 0x4018-0x401f is normally disabled
*/

const int CARTRIDGE_START = 0x4020;
const int CARTRIDGE_END = 0xffff;

const int NMI_VECTOR = 0xfffa;
const int RESET_VECTOR = 0xfffc;
const int IRQ_VECTOR = 0xfffe;

const Byte STACK_START = 0xfd;
const int STACK_OFFSET = 0x0100;

const Byte STATUS_START = 0x34;

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




// debug
struct Operation {
	Instruction instruction;
	AddressMode address_mode;
	short clock_cycles;
};

Operation operations[256];
InstructionFunction instruction_functions[NUM_INSTRUCTIONS];




struct Breakpoint {
	BreakpointCondition condition;
	Word address;
};
std::vector<Breakpoint> breakpoints;

void debugReadOperation(Word address, Byte value);
void debugWriteOperation(Word address, Byte value);
void debugProgramPosition(Word address);
void debugInterrupt(BreakpointCondition condition);





void clockTick();

Byte read(Word address);
Byte readByte(Word address);
Word readWord(Word address);
Word readWordBug(Word address);
void write(Word address, Byte value);
void writeByte(Word address, Byte value);

Word getAddress(AddressMode address_mode);

template <class Boolean>
void setStatusFlag(int bit, Boolean value = Constant<bool, true>());
bool getStatusFlag(int bit);

void conditionalBranch(bool branch);
void compareWithValue(AddressMode address_mode, Byte value);
void pushByteToStack(Byte value);
void pushWordToStack(Word value);
Byte popByteFromStack();
Word popWordFromStack();
void addValueToAcc(Byte value);
void setArithmeticFlags(Byte value);
void halt();
void nmi();
void irq();
void oamDmaTransfer(Byte high);

int cycles;

void runFrame() {
	cycles = 0;
	while(!CPU::halted() && !CPU::_break && !PPU::readyToDraw()) {
		CPU::execute();
	}
	APU::runFrame(cycles);
}

void clockTick() {
	odd_cycle = !odd_cycle;
	for(int i = 0; i < 3; i++) {
		if (_nmi >= 0) _nmi++;
		if (_irq >= 0) _irq++;
		PPU::clockTick();
	}
	cycles++;

	test_ticks++; // debug
}

int readDMC(void*, cpu_addr_t address) {
	return read(address);
}

bool halted() {
	return _halt;
}

bool breaked() {
	return status & (1 << BREAK);
}

void power() {
	stack_pointer = STACK_START;
	status = STATUS_START;
	accumulator = 0;
	x_register = 0;
	y_register = 0;

	write(APU_STATUS, 0);
	write(APU_FRAME_COUNT, 0);
	for(int n = 0; n < 16; n++) {
		write(APU_START + n, 0);
	}

	cycles = 0;

	program_counter = readWord(RESET_VECTOR);

	_halt = false;
	_nmi = -1;
	_irq = -1;
	odd_cycle = false;

	for(int i = 0; i < RAM_SIZE; i++) {
		ram[i] = 0xff;
	}

	_break = false;
}

void reset() {
	stack_pointer -= 3;
	setStatusFlag(DISABLE_INTERRUPTS, true);
	write(APU_STATUS, 0);
	
	program_counter = readWord(RESET_VECTOR);

	cycles = 0;

	_halt = false;
	_nmi = -1;
	_irq = -1;
	odd_cycle = false;

	_break = false;
}

void setNMI(bool on) {
	_nmi = on ? 0 : -1;
}

void setIRQ(bool on) {
	_irq = on ? 0 : -1;
}
	
Byte read(Word address) {
	Byte value = 0;
	switch(address) {
		case RAM_START ... RAM_END:
			value = ram[(address - RAM_START) % RAM_SIZE];
			break;
		
		case PPU_START ... PPU_END:
			value = PPU::readByte((address - PPU_START) % PPU_SIZE);
			break;

		case APU_START ... APU_END:
			if (address == PPU::OAM_DMA) {
				dout("trying to read from OAM_DMA");
				break;
			}

			value = APU::readByte(cycles, address);
			break;

		case JOY1 ... JOY2:
			{
				Controller* controller = controller_ports[address - JOY1];
				if (controller) value = controller->read();
			}
			break;

		case 0x4018 ... 0x401f:
			dout("read from disabled");
			break;

		case CARTRIDGE_START ... CARTRIDGE_END:
			value = cartridge->readPRG(address);
			break;
	}

	return value;
}

Byte readByte(Word address) {
	Byte value;
	clockTick();
	value = read(address);
	debugReadOperation(address, value);
	return value;
}

void write(Word address, Byte value) {
	switch(address) {
		case RAM_START ... RAM_END:
			ram[(address - RAM_START) % RAM_SIZE] = value;
			break;

		case PPU_START ... PPU_END:
			PPU::writeByte((address - PPU_START) % PPU_SIZE, value);
			break;

		case APU_START ... APU_END:
		case JOY2:
			if (address == OAM_DMA) {
				oamDmaTransfer(value);
			} else {
				APU::writeByte(cycles, address, value);
			}
			break;

		case JOY1:
			{
				Controller* controller = controller_ports[address - JOY1];
				if (controller) controller->write(value);
			}
			break;

		case 0x4018 ... 0x401f:
			// disabled
			break;

		case CARTRIDGE_START ... CARTRIDGE_END:
			cartridge->writePRG(address, value);
			break;
	}
}

void writeByte(Word address, Byte value) {
	debugWriteOperation(address, value);
	clockTick();
	write(address, value);
}

void execute() {
	wait_cycles = 0;
	test_ticks = 0;

	if (!_halt) {
		_irq = getStatusFlag(DISABLE_INTERRUPTS) ? -1 : _irq;

		if (_nmi > 1) {
			_nmi = -1;
			nmi();

			if (test_ticks != wait_cycles) {
				std::cout << '\n';
				dout("NMI");
				dout("wait cycles: " << wait_cycles);
				dout("test ticks: " << test_ticks);
			}
		} else if (_irq > 1) {
			_irq = -1;
			irq();

			if (test_ticks != wait_cycles) {
				std::cout << '\n';
				dout("IRQ");
				dout("wait cycles: " << wait_cycles);
				dout("test ticks: " << test_ticks);
			}
		} else {
			Byte opcode = readByte(program_counter++);
			Operation op = operations[opcode];
			wait_cycles = op.clock_cycles;

			debugOperation(program_counter-1, opcode);

			InstructionFunction function = instruction_functions[op.instruction];
			(*function)(op.address_mode);

			if (_halt) dout("opcode: " << toHex(opcode));

			if (test_ticks != wait_cycles) {
				std::cout << '\n';
				std::cout << "inconsistent timing\n";
				dout("instruction: " << instruction_names[op.instruction]);
				dout("address mode: " << address_mode_names[op.address_mode]);
				dout("wait cycles: " << wait_cycles);
				dout("test ticks: " << test_ticks);
			}
		}

		debugProgramPosition(program_counter);
	}
}

void nmi() {
	wait_cycles = 7;
	clockTick();
	clockTick();
	pushWordToStack(program_counter);
	pushByteToStack(status);
	setStatusFlag(DISABLE_INTERRUPTS, Constant<bool, true>());
	program_counter = readWord(NMI_VECTOR);

	debugInterrupt(NMI);
	in_nmi = true;
}

void irq() {
	wait_cycles = 7;
	clockTick();
	clockTick();
	setStatusFlag(BREAK, Constant<bool, false>());
	pushWordToStack(program_counter);
	pushByteToStack(status);
	setStatusFlag(DISABLE_INTERRUPTS, Constant<bool, true>());
	program_counter = readWord(IRQ_VECTOR);

	debugInterrupt(IRQ);
}

Word getAddress(AddressMode address_mode) {
	Word address = 0;
	Word page1 = 0;
	Byte zp = 0;
	switch(address_mode) {
		case IMMEDIATE:
			address = program_counter++;
			break;

		case ABSOLUTE:
			address = readWord(program_counter);
			program_counter += 2;
			break;

		case ZERO_PAGE:
			address = readByte(program_counter++);
			break;

		case ZERO_PAGE_X:
			address = (readByte(program_counter++) + x_register) & 0xff;
			clockTick();
			break;

		case ZERO_PAGE_Y:
			address = (readByte(program_counter++) + y_register) & 0xff;
			clockTick();
			break;

		case ABSOLUTE_X:
			page1 = readWord(program_counter);
			program_counter += 2;
			address = page1 + x_register;
			if (crossPage(page1, address)) {
				wait_cycles++;
				readByte(program_counter); // dummy read
			}
			break;

		case ABSOLUTE_X_STORE:
			page1 = readWord(program_counter);
			program_counter += 2;
			address = page1 + x_register;
			readByte(program_counter); // dummy read
			break;

		case ABSOLUTE_Y:
			page1 = readWord(program_counter);
			program_counter += 2;
			address = page1 + y_register;
			if (crossPage(page1, address)) {
				wait_cycles++;
				readByte(program_counter); // dummy read
			}
			break;

		case ABSOLUTE_Y_STORE:
			page1 = readWord(program_counter);
			program_counter += 2;
			address = page1 + y_register;
			readByte(program_counter); // dummy read
			break;

		case INDIRECT:
			address = readWordBug(readWord(program_counter));
			program_counter += 2;
			break;

		case INDIRECT_X:
			zp = (readByte(program_counter++) + x_register) & 0xff;
			clockTick();
			address = readWordBug(zp);
			break;

		case INDIRECT_Y:
			zp = readByte(program_counter++);
			address = readWordBug(zp) + y_register;
			if (crossPage(address - y_register, address)) {
				readByte(program_counter); // dummy read
				wait_cycles++;
			}
			break;

		case INDIRECT_Y_STORE:
			zp = readByte(program_counter++);
			address = readWordBug(zp) + y_register;
			readByte(program_counter); // dummy read
			break;

		case RELATIVE_MODE:
			address = program_counter + 1 + (signed char)readByte(program_counter);
			program_counter++;
			break;

		default:
			dout("Invalid address mode: " << address_mode);
			halt();
	}
	return address;
}

void halt() {
	dout("halt");
	_halt = true;
}

Word readWord(Word address) {
	Word low = readByte(address);
	Word high = readByte(address + 1);
	return (high << 8) | low;
}

Word readWordBug(Word address) {
	Word low = readByte(address);
	Word high = readByte(((address & 0xff) == 0xff)
		? (address & 0xff00)
		: (address + 1));
	return (high << 8) | low;
}

bool getStatusFlag(int bit) {
	return status & (1 << bit);
}

template <class Boolean>
void setStatusFlag(int bit, Boolean value) {
	if (value) {
		status |= (1 << bit);
	} else {
		status &= ~(1 << bit);
	}
}

void setArithmeticFlags(Byte value) {
	setStatusFlag(NEGATIVE, value & 0x80);
	setStatusFlag(ZERO, value == 0);
}

void conditionalBranch(bool branch) {
	int offset = (signed char)readByte(program_counter++);
	int page1 = program_counter;
	if (branch) {
		clockTick();
		wait_cycles++;
		program_counter += offset;

		if (crossPage(page1, program_counter)) {
			wait_cycles++;
			clockTick();
		}
		debugSpecific("branch to " << toHex(program_counter));
	} else {
		debugSpecific("no branch");
	}
}

void compareWithValue(AddressMode address_mode, Byte reg) {
	Byte value = readByte(getAddress(address_mode));
	setStatusFlag(CARRY, reg >= value);
	setArithmeticFlags(reg - value);

	debugSpecific("compare " << (int)value << " with " << (int)reg);
	debugStatus(NEGATIVE);
	debugStatus(OVERFLOW);
	debugStatus(ZERO);
}

void pushByteToStack(Byte value) {
	debugSpecific("push " << toHex(value));
	writeByte(STACK_OFFSET | stack_pointer--, value);
}

void pushWordToStack(Word value) {
	pushByteToStack(value >> 8);
	pushByteToStack(value & 255);
}

Byte popByteFromStack() {
	Byte value;
	value = readByte(STACK_OFFSET | ++stack_pointer);
	debugSpecific("pop " << toHex(value));
	return value;
}

Word popWordFromStack() {
	Word low = popByteFromStack();
	Word high = popByteFromStack();
	return (high << 8) | low;
}

void addValueToAcc(Byte value) {
	unsigned int result = accumulator + value + getStatusFlag(CARRY);
	bool carry = result > 0xff;
	setOverflow(accumulator, value, result);
	setStatusFlag(CARRY, carry);
	accumulator = result & 0xff;
	setArithmeticFlags(accumulator);
}

void addWithCarry(AddressMode address_mode) {
	Byte value = readByte(getAddress(address_mode));
	addValueToAcc(value);

	debugSpecific("acc + " << (int)value << " + " << getStatusFlag(CARRY)
		<< " = " << (int)accumulator);
	debugStatus(NEGATIVE);
	debugStatus(OVERFLOW);
	debugStatus(ZERO);
	debugStatus(CARRY);
}

void bitwiseAnd(AddressMode address_mode) {
	Byte value = readByte(getAddress(address_mode));
	accumulator &= value;
	setArithmeticFlags(accumulator);

	debugSpecific("acc & " << (int)value << " = " << (int)accumulator);
	debugStatus(NEGATIVE);
	debugStatus(OVERFLOW);
	debugStatus(ZERO);
	debugStatus(CARRY);
}

void shiftLeft(AddressMode address_mode) {
	int result;
	if (address_mode == ACCUMULATOR) {
		readByte(program_counter); // dummy read
		result = accumulator << 1;
		accumulator = result & 0xff;

		debugSpecific("acc << 1 = " << toHex(accumulator));
	} else {
		Word address = getAddress(address_mode);
		result = readByte(address) << 1;
		clockTick();
		writeByte(address, result & 0xff);
		debugSpecific("mem[" << toHex(address) << "] << 1 = " << (int)result);
	}
	setArithmeticFlags(result & 0xff);
	setStatusFlag(CARRY, result & 0x100);
}

void branchOnCarryClear(AddressMode address_mode) {
	conditionalBranch(!getStatusFlag(CARRY));
}

void branchOnCarrySet(AddressMode address_mode) {
	conditionalBranch(getStatusFlag(CARRY));
}

void branchOnZero(AddressMode address_mode) {
	conditionalBranch(getStatusFlag(ZERO));
}

void testBits(AddressMode address_mode) {
	Byte value = readByte(getAddress(address_mode));
	setStatusFlag(NEGATIVE, value & (1 << NEGATIVE));
	setStatusFlag(OVERFLOW, value & (1 << OVERFLOW));
	setStatusFlag(ZERO, (value & accumulator) == 0);

	debugSpecific("test " << (int)value);
	debugStatus(NEGATIVE);
	debugStatus(OVERFLOW);
	debugStatus(ZERO);
}

void branchOnNegative(AddressMode address_mode) {
	conditionalBranch(getStatusFlag(NEGATIVE));
}

void branchOnNotZero(AddressMode address_mode) {
	conditionalBranch(!getStatusFlag(ZERO));
}

void branchOnPositive(AddressMode address_mode) {
	conditionalBranch(!getStatusFlag(NEGATIVE));
}

void forceBreak(AddressMode address_mode) {
	readByte(program_counter); // dummy read
	setStatusFlag(BREAK, Constant<bool, true>());
	pushWordToStack(program_counter+1);
	pushByteToStack(status);
	setStatusFlag(DISABLE_INTERRUPTS, Constant<bool, true>());
	program_counter = readWord(IRQ_VECTOR);
}

void branchOnOverflowClear(AddressMode address_mode) {
	conditionalBranch(!getStatusFlag(OVERFLOW));
}

void branchOnOverflowSet(AddressMode address_mode) {
	conditionalBranch(getStatusFlag(OVERFLOW));
}

void clearCarryFlag(AddressMode address_mode) {
	setStatusFlag(CARRY, Constant<bool, false>());
	clockTick();

	debugStatus(CARRY);
}

void clearDecimalFlag(AddressMode address_mode) {
	setStatusFlag(DECIMAL, Constant<bool, false>());
	clockTick();

	debugStatus(DECIMAL);
}

void clearInterruptDisableFlag(AddressMode address_mode) {
	setStatusFlag(DISABLE_INTERRUPTS, Constant<bool, false>());
	clockTick();

	debugStatus(DISABLE_INTERRUPTS);
}

void clearOverflowFlag(AddressMode address_mode) {
	setStatusFlag(OVERFLOW, Constant<bool, false>());
	clockTick();

	debugStatus(OVERFLOW);
}

void compareWithAcc(AddressMode address_mode) {
	compareWithValue(address_mode, accumulator);
}

void compareWithX(AddressMode address_mode) {
	compareWithValue(address_mode, x_register);
}

void compareWithY(AddressMode address_mode) {
	compareWithValue(address_mode, y_register);
}

void decrement(AddressMode address_mode) {
	Word address = getAddress(address_mode);
	Byte result = readByte(address) - 1;
	clockTick();
	setArithmeticFlags(result);
	writeByte(address, result);

	debugSpecific("mem[" << toHex(address) << "] - 1 = " << (int)result);
}

void decrementX(AddressMode address_mode) {
	setArithmeticFlags(--x_register);
	clockTick();

	debugSpecific("x - 1 = " << (int)x_register);
}

void decrementY(AddressMode address_mode) {
	setArithmeticFlags(--y_register);
	clockTick();

	debugSpecific("y - 1 = " << (int)y_register);
}

void exclusiveOr(AddressMode address_mode) {
	Byte value = readByte(getAddress(address_mode));
	accumulator ^= value;
	setArithmeticFlags(accumulator);

	debugSpecific("acc ^ " << (int)value << " = " << (int)accumulator);
}

void increment(AddressMode address_mode) {
	Word address = getAddress(address_mode);
	Byte result = readByte(address) + 1;
	clockTick();
	setArithmeticFlags(result);
	writeByte(address, result);

	debugSpecific("mem[" << toHex(address) << "] + 1 = " << (int)result);
}

void incrementX(AddressMode address_mode) {
	setArithmeticFlags(++x_register);
	clockTick();

	debugSpecific("x + 1 = " << (int)x_register);
}

void incrementY(AddressMode address_mode) {
	setArithmeticFlags(++y_register);
	clockTick();

	debugSpecific("y + 1 = " << (int)y_register);
}

void jump(AddressMode address_mode) {
	program_counter = getAddress(address_mode);
	debugSpecific("pc = " << toHex(program_counter));
}

void jumpToSubroutine(AddressMode address_mode) {
	clockTick();
	pushWordToStack(program_counter + 1);

	debugSpecific("return addr = " << toHex(program_counter + 2, 2));

	program_counter = readWord(program_counter);

	debugSpecific("pc = " << toHex(program_counter));
}

void loadAcc(AddressMode address_mode) {
	Word address = getAddress(address_mode);
	accumulator = readByte(address);
	setArithmeticFlags(accumulator);

	debugSpecific("acc = mem[" << toHex(address) << "] = " << (int)accumulator);
}

void loadX(AddressMode address_mode) {
	Word address = getAddress(address_mode);
	x_register = readByte(address);
	setArithmeticFlags(x_register);

	debugSpecific("x = mem[" << toHex(address) << "] = " << (int)x_register);
}

void loadY(AddressMode address_mode) {
	Word address = getAddress(address_mode);
	y_register = readByte(address);
	setArithmeticFlags(y_register);

	debugSpecific("y = mem[" << toHex(address) << "] = " << (int)y_register);
}

void shiftRight(AddressMode address_mode) {
	Byte result = 0;
	bool carry = false;
	if (address_mode == ACCUMULATOR) {
		readByte(program_counter); // dummy read
		carry = accumulator & 1;
		accumulator >>= 1;
		result = accumulator;

		debugSpecific("acc >> 1 = " << toHex(accumulator));
	} else {
		Word address = getAddress(address_mode);
		Byte value = readByte(address);
		carry = value & 1;
		result = value >> 1;
		clockTick();
		writeByte(address, result);

		debugSpecific("mem[" << toHex(address) << "] >> 1 = " << toHex(result));
	}

	setStatusFlag(CARRY, carry);
	setArithmeticFlags(result);

	debugStatus(ZERO);
	debugStatus(CARRY);
}

void noOperation(AddressMode address_mode) {
	clockTick();
}


void bitwiseOr(AddressMode address_mode) {
	Byte value = readByte(getAddress(address_mode));
	accumulator |= value;
	setArithmeticFlags(accumulator);

	debugSpecific("acc | " << toHex(value) << " = " << toHex(accumulator));
	debugStatus(NEGATIVE);
	debugStatus(ZERO);
}

void pushAcc(AddressMode address_mode) {
	clockTick();
	pushByteToStack(accumulator);
}

void pushStatus(AddressMode address_mode) {
	clockTick();
	pushByteToStack(status | 0x30);
}

void popAcc(AddressMode address_mode) {
	readByte(program_counter); // dummy read
	clockTick();
	accumulator = popByteFromStack();
	setArithmeticFlags(accumulator);

	debugSpecific("acc = " << (int)accumulator);
	debugStatus(NEGATIVE);
	debugStatus(ZERO);

}

void popStatus(AddressMode address_mode) {
	readByte(program_counter); // dummy read
	clockTick();
	status = (popByteFromStack() & ~0x30) | (status & 0x30);

	debugStatus(NEGATIVE);
	debugStatus(OVERFLOW);
	debugStatus(BREAK);
	debugStatus(DECIMAL);
	debugStatus(DISABLE_INTERRUPTS);
	debugStatus(ZERO);
	debugStatus(CARRY);
}

void rotateLeft(AddressMode address_mode) {
	bool carry = false;
	Byte result;
	if (address_mode == ACCUMULATOR) {
		readByte(program_counter); // dummy read
		carry = accumulator & 0x80;
		accumulator = (accumulator << 1) | getStatusFlag(CARRY);
		result = accumulator;

		debugSpecific("acc <<= 1 = " << (int)accumulator);
	} else {
		Word address = getAddress(address_mode);
		result = readByte(address);
		carry = result & 0x80;
		result = (result << 1) | getStatusFlag(CARRY);
		clockTick();
		writeByte(address, result);

		debugSpecific("[" << toHex(address, 2) << "] <<= 1 = " << toHex(result));
	}
	setStatusFlag(CARRY, carry);
	setArithmeticFlags(result);

	debugStatus(NEGATIVE);
	debugStatus(ZERO);
	debugStatus(CARRY);
}

void rotateRight(AddressMode address_mode) {
	bool carry;
	Byte result;
	if (address_mode == ACCUMULATOR) {
		readByte(program_counter); // dummy read
		carry = accumulator & 1;
		accumulator = (accumulator >> 1) | (getStatusFlag(CARRY) ? 0x80 : 0);
		result = accumulator;
		
		debugSpecific("acc >>= 1 = " << toHex(accumulator));
	} else {
		Word address = getAddress(address_mode);
		result = readByte(address);
		carry = result & 1;
		result =  (result >> 1) | (getStatusFlag(CARRY) ? 0x80 : 0);
		clockTick();
		writeByte(address, result);

		debugSpecific("[" << toHex(address, 2) << "] >>= 1 = " << toHex(result));
	}
	setStatusFlag(CARRY, carry);
	setArithmeticFlags(result);

	debugStatus(NEGATIVE);
	debugStatus(ZERO);
	debugStatus(CARRY);
}

void returnFromInterrupt(AddressMode address_mode) {
	readByte(program_counter); // dummy read
	status = (popByteFromStack() & ~0x30) | (status & 0x30);
	program_counter = popWordFromStack();
	clockTick();

	debugSpecific("pc = " << toHex(program_counter));
	debugStatus(NEGATIVE);
	debugStatus(OVERFLOW);
	debugStatus(BREAK);
	debugStatus(DECIMAL);
	debugStatus(DISABLE_INTERRUPTS);
	debugStatus(ZERO);
	debugStatus(CARRY);

	in_nmi = false;
}

void returnFromSubroutine(AddressMode address_mode) {
	readByte(program_counter); // dummy read
	clockTick();
	program_counter = popWordFromStack() + 1;
	clockTick();

	debugSpecific("pc = " << toHex(program_counter));
}

void subtractFromAcc(AddressMode address_mode) {
	Byte value = readByte(getAddress(address_mode));
	addValueToAcc(~value);

	debugSpecific("acc - " << (int)value << " - " << getStatusFlag(CARRY)
		<< " = " << (int)accumulator);
}

void setCarryFlag(AddressMode address_mode) {
	setStatusFlag(CARRY, Constant<bool, true>());
	clockTick();

	debugStatus(CARRY);
}

void setDecimalFlag(AddressMode address_mode) {
	setStatusFlag(DECIMAL, Constant<bool, true>());
	clockTick();

	debugStatus(DECIMAL);
}

void setInterruptDisableFlag(AddressMode address_mode) {
	setStatusFlag(DISABLE_INTERRUPTS, Constant<bool, true>());
	clockTick();

	debugStatus(DISABLE_INTERRUPTS);
}

void storeAcc(AddressMode address_mode) {
	Word address = getAddress(address_mode);
	writeByte(address, accumulator);

	debugSpecific("mem[" << toHex(address) << "] = " << (int)accumulator);
}

void storeX(AddressMode address_mode) {
	Word address = getAddress(address_mode);
	writeByte(address, x_register);

	debugSpecific("mem[" << toHex(address) << "] = " << (int)x_register);
}

void storeY(AddressMode address_mode) {
	Word address = getAddress(address_mode);
	writeByte(address, y_register);

	debugSpecific("mem[" << toHex(address) << "] = " << (int)y_register);
}

void transferAccToX(AddressMode address_mode) {
	x_register = accumulator;
	setArithmeticFlags(x_register);
	clockTick();

	debugSpecific("x = " << (int)x_register);
}

void transferAccToY(AddressMode address_mode) {
	y_register = accumulator;
	setArithmeticFlags(y_register);
	clockTick();
	
	debugSpecific("y = " << (int)y_register);
}

void transferStackPointerToX(AddressMode address_mode) {
	x_register = stack_pointer;
	setArithmeticFlags(x_register);
	clockTick();
	
	debugSpecific("x = " << toHex(x_register) << " (" << (int)(0xff - x_register) << ')');
}

void transferXToAcc(AddressMode address_mode) {
	accumulator = x_register;
	setArithmeticFlags(accumulator);
	clockTick();
	
	debugSpecific("acc = " << (int)accumulator);
}

void transferXToStackPointer(AddressMode address_mode) {
	stack_pointer = x_register;
	clockTick();

	debugSpecific("sp = " << toHex(stack_pointer) << " (" << (int)(0xff - stack_pointer) << ')');
}

void transferYToAcc(AddressMode address_mode) {
	accumulator = y_register;
	setArithmeticFlags(accumulator);
	clockTick();
	
	debugSpecific("acc = " << (int)y_register);
}

void illegalOpcode(AddressMode address_mode) {
	dout("illegal opcode at " << toHex(program_counter-1, 2));
	dout("address mode: " << address_mode);
	halt();
}

void oamDmaTransfer(Byte high) {
	clockTick();
	if (odd_cycle) {
		clockTick();
		wait_cycles++;
	}

	wait_cycles += 513;

	Word address = high << 8;
	for(int index = 0; index < 256; index++) {
		Byte data = readByte(address + index);
		clockTick();
		PPU::writeToOAM(data);
	}
}







void dump(Word address) {
	address &= 0xfff0;

	std::cout << "      ";
	for(int bi = 0; bi < NUM_BYTE_INTERPRETATIONS; bi++) {
		std::cout << "| ";
		for(unsigned int i = 0; i < 0x10; i++) {
			std::cout << toHex(i, 0.5, "");
			std::string spaces;
			switch(bi) {
				case RAW:
					spaces = "  ";
					break;
				case OPCODE:
					spaces = "   ";
					break;
				case ASCII:
					spaces = " ";
			}
			std::cout << spaces;
		}
	}

	std::cout << '\n';
	for(int i = 0; i < 156; i++) std::cout << '-';
	std::cout << '\n';

	for(unsigned int high = 0; high < 0x10; high++) {
		std::cout << toHex(address + (high << 4), 2) << ' ';
		for(int bi = 0; bi < NUM_BYTE_INTERPRETATIONS; bi++) {
			std::cout << "| ";
			for(unsigned int low = 0; low < 0x10; low++) {
				Byte value = readByte(address + ((high << 4) + low));
				Instruction instruction;
				switch(bi) {
					case RAW:
						std::cout << toHex(value, 1, "");
						break;
					case OPCODE:
						instruction = operations[value].instruction;
						std::cout << ((instruction != ILL) ? instruction_names[instruction] : "   ");
						break;
					case ASCII:
						std::cout << (isspace(value) ? ' ' : (char)value);
						break;
				}
				std::cout << ' ';
			}
		}
		std::cout << '\n';
	}
}

void dumpStack() {
	std::cout << "Stack:\n";
	for(int i = (STACK_OFFSET | stack_pointer) + 1; i <= (STACK_OFFSET | STACK_START); i++) {
		std::cout << ' ' << toHex(readByte(i), 1) << '\n';
	}
	std::cout << "-----\n";
}

void dumpState() {
	std::cout << "Accumulator: " << toHex(accumulator) << '\n';
	std::cout << "X register: " << toHex(x_register) << '\n';
	std::cout << "Y register: " << toHex(y_register) << '\n';
	std::cout << "Program counter: " << toHex(program_counter) << '\n';
	std::cout << "Stack pointer: " << toHex(stack_pointer) << '\n';
	std::cout << "Status: NV BDIZC\n";
	std::cout << "        " << std::bitset<8>(status) << '\n';
}

void addBreakpoint(Word address, BreakpointCondition condition) {
	breakpoints.push_back({
		condition,
		address
	});
}

void clearBreakpoints() {
	breakpoints.clear();
}

void debugReadOperation(Word address, Byte value) {
	for(unsigned int n = 0; n < breakpoints.size(); n++) {
		Breakpoint op = breakpoints[n];
		if (op.condition == READ_FROM && op.address == address) {
			std::cout << "read " << toHex(value, 1) << " from " << toHex(address, 2) << '\n';
			_break = true;
		}
	}
}

void debugWriteOperation(Word address, Byte value) {
	for(unsigned int n = 0; n < breakpoints.size(); n++) {
		Breakpoint op = breakpoints[n];
		if (op.condition == WRITE_TO && op.address == address) {
			std::cout << "wrote " << toHex(value, 1) << " to " << toHex(address, 2) << '\n';
			_break = true;
		}
	}
}

void debugProgramPosition(Word address) {
	for(unsigned int n = 0; n < breakpoints.size(); n++) {
		Breakpoint op = breakpoints[n];
		if (op.condition == PROGRAM_POSITION && op.address == address) {
			std::cout << "reached PC " << toHex(address, 2) << '\n';
			_break = true;
		}
	}
}

void debugInterrupt(BreakpointCondition condition) {
	for(unsigned int n = 0; n < breakpoints.size(); n++) {
		Breakpoint op = breakpoints[n];
		if (op.condition == condition) {
			std::string name;
			switch(condition) {
				case IRQ: name = "IRQ"; break;
				case NMI: name = "NMI"; break;
				default:
					assert(false, "interrupt breakpoint not an interrupt");
					break;
			}
			std::cout << name << ", PC: " << toHex(program_counter, 2) << '\n';
			_break = true;
		}
	}
}

#define setOperation(address, instruction, mode, cycles) \
	operations[(address)] = { (instruction), (mode), (cycles) };
#define setInstructionFunction(instruction, function) instruction_functions[(instruction)] = (function);
void init() {
	for(unsigned int i = 0; i < 256; i++) {
		setOperation(i, ILL, IMPLIED, 0);
	}

	setOperation(0x69, ADC, IMMEDIATE, 2);
	setOperation(0x65, ADC, ZERO_PAGE, 3);
	setOperation(0x75, ADC, ZERO_PAGE_X, 4);
	setOperation(0x6d, ADC, ABSOLUTE, 4);
	setOperation(0x7d, ADC, ABSOLUTE_X, 4);
	setOperation(0x79, ADC, ABSOLUTE_Y, 4);
	setOperation(0x61, ADC, INDIRECT_X, 6);
	setOperation(0x71, ADC, INDIRECT_Y, 5);

	setOperation(0x29, AND, IMMEDIATE, 2);
	setOperation(0x25, AND, ZERO_PAGE, 3);
	setOperation(0x35, AND, ZERO_PAGE_X, 4);
	setOperation(0x2d, AND, ABSOLUTE, 4);
	setOperation(0x3d, AND, ABSOLUTE_X, 4);
	setOperation(0x39, AND, ABSOLUTE_Y, 4);
	setOperation(0x21, AND, INDIRECT_X, 6);
	setOperation(0x31, AND, INDIRECT_Y, 5);

	setOperation(0x0a, ASL, ACCUMULATOR, 2);
	setOperation(0x06, ASL, ZERO_PAGE, 5);
	setOperation(0x16, ASL, ZERO_PAGE_X, 6);
	setOperation(0x0e, ASL, ABSOLUTE, 6);
	setOperation(0x1e, ASL, ABSOLUTE_X_STORE, 7);

	setOperation(0x90, BCC, RELATIVE_MODE, 2);

	setOperation(0xb0, BCS, RELATIVE_MODE, 2);

	setOperation(0xf0, BEQ, RELATIVE_MODE, 2);

	setOperation(0x24, BIT, ZERO_PAGE, 3);
	setOperation(0x2c, BIT, ABSOLUTE, 4);

	setOperation(0x30, BMI, RELATIVE_MODE, 2);

	setOperation(0xd0, BNE, RELATIVE_MODE, 2);

	setOperation(0x10, BPL, RELATIVE_MODE, 2);

	setOperation(0x00, BRK, IMPLIED, 7);

	setOperation(0x50, BVC, RELATIVE_MODE, 2);

	setOperation(0x70, BVS, RELATIVE_MODE, 2);

	setOperation(0x18, CLC, IMPLIED, 2);

	setOperation(0xd8, CLD, IMPLIED, 2);

	setOperation(0x58, CLI, IMPLIED, 2);

	setOperation(0xb8, CLV, IMPLIED, 2);

	setOperation(0xc9, CMP, IMMEDIATE, 2);
	setOperation(0xc5, CMP, ZERO_PAGE, 3);
	setOperation(0xd5, CMP, ZERO_PAGE_X, 4);
	setOperation(0xcd, CMP, ABSOLUTE, 4);
	setOperation(0xdd, CMP, ABSOLUTE_X, 4);
	setOperation(0xd9, CMP, ABSOLUTE_Y, 4);
	setOperation(0xc1, CMP, INDIRECT_X, 6);
	setOperation(0xd1, CMP, INDIRECT_Y, 5);

	setOperation(0xe0, CPX, IMMEDIATE, 2);
	setOperation(0xe4, CPX, ZERO_PAGE, 3);
	setOperation(0xec, CPX, ABSOLUTE, 4);

	setOperation(0xc0, CPY, IMMEDIATE, 2);
	setOperation(0xc4, CPY, ZERO_PAGE, 3);
	setOperation(0xcc, CPY, ABSOLUTE, 4);

	setOperation(0xc6, DEC, ZERO_PAGE, 5);
	setOperation(0xd6, DEC, ZERO_PAGE_X, 6);
	setOperation(0xce, DEC, ABSOLUTE, 6);
	setOperation(0xde, DEC, ABSOLUTE_X_STORE, 7);

	setOperation(0xca, DEX, IMPLIED, 2);

	setOperation(0x88, DEY, IMPLIED, 2);

	setOperation(0x49, EOR, IMMEDIATE, 2);
	setOperation(0x45, EOR, ZERO_PAGE, 3);
	setOperation(0x55, EOR, ZERO_PAGE_X, 4);
	setOperation(0x4d, EOR, ABSOLUTE, 4);
	setOperation(0x5d, EOR, ABSOLUTE_X, 4);
	setOperation(0x59, EOR, ABSOLUTE_Y, 4);
	setOperation(0x41, EOR, INDIRECT_X, 6);
	setOperation(0x51, EOR, INDIRECT_Y, 5);

	setOperation(0xe6, INC, ZERO_PAGE, 5);
	setOperation(0xf6, INC, ZERO_PAGE_X, 6);
	setOperation(0xee, INC, ABSOLUTE, 6);
	setOperation(0xfe, INC, ABSOLUTE_X_STORE, 7);

	setOperation(0xe8, INX, IMPLIED, 2);

	setOperation(0xc8, INY, IMPLIED, 2);

	setOperation(0x4c, JMP, ABSOLUTE, 3);
	setOperation(0x6c, JMP, INDIRECT, 5);

	setOperation(0x20, JSR, ABSOLUTE, 6);

	setOperation(0xa9, LDA, IMMEDIATE, 2);
	setOperation(0xa5, LDA, ZERO_PAGE, 3);
	setOperation(0xb5, LDA, ZERO_PAGE_X, 4);
	setOperation(0xad, LDA, ABSOLUTE, 4);
	setOperation(0xbd, LDA, ABSOLUTE_X, 4);
	setOperation(0xb9, LDA, ABSOLUTE_Y, 4);
	setOperation(0xa1, LDA, INDIRECT_X, 6);
	setOperation(0xb1, LDA, INDIRECT_Y, 5);

	setOperation(0xa2, LDX, IMMEDIATE, 2);
	setOperation(0xa6, LDX, ZERO_PAGE, 3);
	setOperation(0xb6, LDX, ZERO_PAGE_Y, 4);
	setOperation(0xae, LDX, ABSOLUTE, 4);
	setOperation(0xbe, LDX, ABSOLUTE_Y, 4);

	setOperation(0xa0, LDY, IMMEDIATE, 2);
	setOperation(0xa4, LDY, ZERO_PAGE, 3);
	setOperation(0xb4, LDY, ZERO_PAGE_X, 4);
	setOperation(0xac, LDY, ABSOLUTE, 4);
	setOperation(0xbc, LDY, ABSOLUTE_X, 4);

	setOperation(0x4a, LSR, ACCUMULATOR, 2);
	setOperation(0x46, LSR, ZERO_PAGE, 5);
	setOperation(0x56, LSR, ZERO_PAGE_X, 6);
	setOperation(0x4e, LSR, ABSOLUTE, 6);
	setOperation(0x5e, LSR, ABSOLUTE_X_STORE, 7);

	setOperation(0xea, NOP, IMPLIED, 2);

	setOperation(0x09, ORA, IMMEDIATE, 2);
	setOperation(0x05, ORA, ZERO_PAGE, 3);
	setOperation(0x15, ORA, ZERO_PAGE_X, 4);
	setOperation(0x0d, ORA, ABSOLUTE, 4);
	setOperation(0x1d, ORA, ABSOLUTE_X, 4);
	setOperation(0x19, ORA, ABSOLUTE_Y, 4);
	setOperation(0x01, ORA, INDIRECT_X, 6);
	setOperation(0x11, ORA, INDIRECT_Y, 5);

	setOperation(0x48, PHA, IMPLIED, 3);

	setOperation(0x08, PHP, IMPLIED, 3);

	setOperation(0x68, PLA, IMPLIED, 4);

	setOperation(0x28, PLP, IMPLIED, 4);

	setOperation(0x2a, ROL, ACCUMULATOR, 2);
	setOperation(0x26, ROL, ZERO_PAGE, 5);
	setOperation(0x36, ROL, ZERO_PAGE_X, 6);
	setOperation(0x2e, ROL, ABSOLUTE, 6);
	setOperation(0x3e, ROL, ABSOLUTE_X_STORE, 7);

	setOperation(0x6a, ROR, ACCUMULATOR, 2);
	setOperation(0x66, ROR, ZERO_PAGE, 5);
	setOperation(0x76, ROR, ZERO_PAGE_X, 6);
	setOperation(0x6e, ROR, ABSOLUTE, 6);
	setOperation(0x7e, ROR, ABSOLUTE_X_STORE, 7);

	setOperation(0x40, RTI, IMPLIED, 6);

	setOperation(0x60, RTS, IMPLIED, 6);

	setOperation(0xe9, SBC, IMMEDIATE, 2);
	setOperation(0xe5, SBC, ZERO_PAGE, 3);
	setOperation(0xf5, SBC, ZERO_PAGE_X, 4);
	setOperation(0xed, SBC, ABSOLUTE, 4);
	setOperation(0xfd, SBC, ABSOLUTE_X, 4);
	setOperation(0xf9, SBC, ABSOLUTE_Y, 4);
	setOperation(0xe1, SBC, INDIRECT_X, 6);
	setOperation(0xf1, SBC, INDIRECT_Y, 5);

	setOperation(0x38, SEC, IMPLIED, 2);

	setOperation(0xf8, SED, IMPLIED, 2);

	setOperation(0x78, SEI, IMPLIED, 2);

	setOperation(0x85, STA, ZERO_PAGE, 3);
	setOperation(0x95, STA, ZERO_PAGE_X, 4);
	setOperation(0x8d, STA, ABSOLUTE, 4);
	setOperation(0x9d, STA, ABSOLUTE_X_STORE, 5);
	setOperation(0x99, STA, ABSOLUTE_Y_STORE, 5);
	setOperation(0x81, STA, INDIRECT_X, 6);
	setOperation(0x91, STA, INDIRECT_Y_STORE, 6);

	setOperation(0x86, STX, ZERO_PAGE, 3);
	setOperation(0x96, STX, ZERO_PAGE_Y, 4);
	setOperation(0x8e, STX, ABSOLUTE, 4);

	setOperation(0x84, STY, ZERO_PAGE, 3);
	setOperation(0x94, STY, ZERO_PAGE_X, 4);
	setOperation(0x8c, STY, ABSOLUTE, 4);

	setOperation(0xaa, TAX, IMPLIED, 2);

	setOperation(0xa8, TAY, IMPLIED, 2);

	setOperation(0xba, TSX, IMPLIED, 2);

	setOperation(0x8a, TXA, IMPLIED, 2);

	setOperation(0x9a, TXS, IMPLIED, 2);

	setOperation(0x98, TYA, IMPLIED, 2);

	setInstructionFunction(ADC, &addWithCarry);
	setInstructionFunction(AND, &bitwiseAnd);
	setInstructionFunction(ASL, &shiftLeft);
	setInstructionFunction(BCC, &branchOnCarryClear);
	setInstructionFunction(BCS, &branchOnCarrySet);
	setInstructionFunction(BEQ, &branchOnZero);
	setInstructionFunction(BIT, &testBits);
	setInstructionFunction(BMI, &branchOnNegative);
	setInstructionFunction(BNE, &branchOnNotZero);
	setInstructionFunction(BPL, &branchOnPositive);
	setInstructionFunction(BRK, &forceBreak);
	setInstructionFunction(BVC, &branchOnOverflowClear);
	setInstructionFunction(BVS, &branchOnOverflowSet);
	setInstructionFunction(CLC, &clearCarryFlag);
	setInstructionFunction(CLD, &clearDecimalFlag);
	setInstructionFunction(CLI, &clearInterruptDisableFlag);
	setInstructionFunction(CLV, &clearOverflowFlag);
	setInstructionFunction(CMP, &compareWithAcc);
	setInstructionFunction(CPX, &compareWithX);
	setInstructionFunction(CPY, &compareWithY);
	setInstructionFunction(DEC, &decrement);
	setInstructionFunction(DEX, &decrementX);
	setInstructionFunction(DEY, &decrementY);
	setInstructionFunction(EOR, &exclusiveOr);
	setInstructionFunction(INC, &increment);
	setInstructionFunction(INX, &incrementX);
	setInstructionFunction(INY, &incrementY);
	setInstructionFunction(JMP, &jump);
	setInstructionFunction(JSR, &jumpToSubroutine);
	setInstructionFunction(LDA, &loadAcc);
	setInstructionFunction(LDX, &loadX);
	setInstructionFunction(LDY, &loadY);
	setInstructionFunction(LSR, &shiftRight);
	setInstructionFunction(NOP, &noOperation);
	setInstructionFunction(ORA, &bitwiseOr);
	setInstructionFunction(PHA, &pushAcc);
	setInstructionFunction(PHP, &pushStatus);
	setInstructionFunction(PLA, &popAcc);
	setInstructionFunction(PLP, &popStatus);
	setInstructionFunction(ROL, &rotateLeft);
	setInstructionFunction(ROR, &rotateRight);
	setInstructionFunction(RTI, &returnFromInterrupt);
	setInstructionFunction(RTS, &returnFromSubroutine);
	setInstructionFunction(SBC, &subtractFromAcc);
	setInstructionFunction(SEC, &setCarryFlag);
	setInstructionFunction(SED, &setDecimalFlag);
	setInstructionFunction(SEI, &setInterruptDisableFlag);
	setInstructionFunction(STA, &storeAcc);
	setInstructionFunction(STX, &storeX);
	setInstructionFunction(STY, &storeY);
	setInstructionFunction(TAX, &transferAccToX);
	setInstructionFunction(TAY, &transferAccToY);
	setInstructionFunction(TSX, &transferStackPointerToX);
	setInstructionFunction(TXA, &transferXToAcc);
	setInstructionFunction(TXS, &transferXToStackPointer);
	setInstructionFunction(TYA, &transferYToAcc);
	setInstructionFunction(ILL, &illegalOpcode);
}

struct Registers {
	Byte accumulator;
	Byte x_register;
	Byte y_register;
	Word program_counter;
	Byte stack_pointer;
	Byte status;
	bool odd_cycle;
	bool _halt;
	int _nmi;
	int _irq;

	int wait_cycles;
	int test_ticks;
};

void saveState(std::ostream& out) {
	Registers ss;
	ss.accumulator = accumulator;
	ss.x_register = x_register;
	ss.y_register = y_register;
	ss.program_counter = program_counter;
	ss.stack_pointer = stack_pointer;
	ss.status = status;
	ss.odd_cycle = odd_cycle;
	ss._halt = _halt;
	ss._nmi = _nmi;
	ss._irq = _irq;
	ss.wait_cycles = wait_cycles;
	ss.test_ticks = test_ticks;

	out.write((const char*)ram, RAM_SIZE);
	out.write((const char*)&ss, sizeof(Registers));
}

void loadState(std::istream& in) {
	Registers ss;

	in.read((char*)ram, RAM_SIZE);
	in.read((char*)&ss, sizeof(Registers));

	accumulator = ss.accumulator;
	x_register = ss.x_register;
	y_register = ss.y_register;
	program_counter = ss.program_counter;
	stack_pointer = ss.stack_pointer;
	status = ss.status;
	odd_cycle = ss.odd_cycle;
	_halt = ss._halt;
	_nmi = ss._nmi;
	_irq = ss._irq;
	wait_cycles = ss.wait_cycles;
	test_ticks = ss.test_ticks;
}

} // end namespace