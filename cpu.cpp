#include <iomanip>
#include <iostream>
#include <bitset>
#include "cpu.hpp"
#include "debugging.hpp"
#include "common.hpp"

#define setOperation(address, instruction, mode, cycles) \
	operations[(address)] = { (instruction), (mode), (cycles) };
#define setInstructionFunction(instruction, function) instruction_functions[(instruction)] = (function);
#define isNegative(byte) ((bool)((byte) & 0x80))
#define twosComp(byte) (1 + ~(byte))
#define inRange(value, low, high) ((value) >= (low) && (value) <= (high))
#define mirrorAddress(value, start, size) (((value) - (start)) % (size))

#define DEBUG_OPERATION true
#define DEBUG_VALUES true

#if (DEBUG_OPERATION)
	#define debugOperation(pc, opcode) if (display_ops_cycles > 0) std::cout << "[" << toHex(pc, 2) \
		<< "] opcode: " << toHex(opcode, 1) << ", " \
		<< instruction_names[operations[opcode].instruction] << ' ' \
		<< ((operations[opcode].address_mode != IMPLIED) \
			? address_mode_names[operations[opcode].address_mode] \
			: "") \
		<< '\n'
#else
	#define debugOperation(pc, opcode)
#endif

#if (DEBUG_VALUES)
	#define debugSpecific(message) if (display_ops_cycles > 0) std::cout << message << '\n'
	#define debugStatus(flag) if (display_ops_cycles > 0) std::cout << #flag << ": " << getStatusFlag(flag) << '\n'
#else
	#define debugSpecific(message)
	#define debugStatus(flag)
#endif

const char* CPU::instruction_names[CPU::NUM_INSTRUCTIONS] = {
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

const char* CPU::address_mode_names[CPU::NUM_ADDRESS_MODES] = {
	"IMMEDIATE",
	"ABSOLUTE",
	"ZERO_PAGE",
	"ZERO_PAGE_X",
	"ZERO_PAGE_Y",
	"IMPLIED",
	"ACCUMULATOR",
	"INDEXED_X",
	"INDEXED_Y",
	"INDIRECT",
	"PRE_INDEXED",
	"POST_INDEXED",
	"RELATIVE",
};

CPU::CPU() {
	dout("CPU()");

	for(unsigned int i = 0; i < 256; i++) {
		setOperation(i, ILL, IMPLIED, 0);
	}

	setOperation(0x69, ADC, IMMEDIATE, 2);
	setOperation(0x65, ADC, ZERO_PAGE, 3);
	setOperation(0x75, ADC, ZERO_PAGE_X, 4);
	setOperation(0x6d, ADC, ABSOLUTE_MODE, 4);
	setOperation(0x7d, ADC, INDEXED_X, 4);
	setOperation(0x79, ADC, INDEXED_Y, 4);
	setOperation(0x61, ADC, PRE_INDEXED, 6);
	setOperation(0x71, ADC, POST_INDEXED, 5);

	setOperation(0x29, AND, IMMEDIATE, 2);
	setOperation(0x25, AND, ZERO_PAGE, 3);
	setOperation(0x35, AND, ZERO_PAGE_X, 4);
	setOperation(0x2d, AND, ABSOLUTE_MODE, 4);
	setOperation(0x3d, AND, INDEXED_X, 4);
	setOperation(0x39, AND, INDEXED_Y, 4);
	setOperation(0x21, AND, PRE_INDEXED, 6);
	setOperation(0x31, AND, POST_INDEXED, 5);

	setOperation(0x0a, ASL, ACCUMULATOR, 2);
	setOperation(0x06, ASL, ZERO_PAGE, 5);
	setOperation(0x16, ASL, ZERO_PAGE_X, 6);
	setOperation(0x0e, ASL, ABSOLUTE_MODE, 6);
	setOperation(0x1e, ASL, INDEXED_X, 7);

	setOperation(0x90, BCC, RELATIVE_MODE, 2);

	setOperation(0xb0, BCS, RELATIVE_MODE, 2);

	setOperation(0xf0, BEQ, RELATIVE_MODE, 2);

	setOperation(0x24, BIT, ZERO_PAGE, 3);
	setOperation(0x2c, BIT, ABSOLUTE_MODE, 4);

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
	setOperation(0xcd, CMP, ABSOLUTE_MODE, 4);
	setOperation(0xdd, CMP, INDEXED_X, 4);
	setOperation(0xd9, CMP, INDEXED_Y, 4);
	setOperation(0xc1, CMP, PRE_INDEXED, 6);
	setOperation(0xd1, CMP, POST_INDEXED, 5);

	setOperation(0xe0, CPX, IMMEDIATE, 2);
	setOperation(0xe4, CPX, ZERO_PAGE, 3);
	setOperation(0xec, CPX, ABSOLUTE_MODE, 4);

	setOperation(0xc0, CPY, IMMEDIATE, 2);
	setOperation(0xc4, CPY, ZERO_PAGE, 3);
	setOperation(0xcc, CPY, ABSOLUTE_MODE, 4);

	setOperation(0xc6, DEC, ZERO_PAGE, 5);
	setOperation(0xd6, DEC, ZERO_PAGE_X, 6);
	setOperation(0xce, DEC, ABSOLUTE_MODE, 3);
	setOperation(0xde, DEC, INDEXED_X, 7);

	setOperation(0xca, DEX, IMPLIED, 2);

	setOperation(0x88, DEY, IMPLIED, 2);

	setOperation(0x49, EOR, IMMEDIATE, 2);
	setOperation(0x45, EOR, ZERO_PAGE, 3);
	setOperation(0x55, EOR, ZERO_PAGE_X, 4);
	setOperation(0x4d, EOR, ABSOLUTE_MODE, 4);
	setOperation(0x5d, EOR, INDEXED_X, 4);
	setOperation(0x59, EOR, INDEXED_Y, 4);
	setOperation(0x41, EOR, PRE_INDEXED, 6);
	setOperation(0x51, EOR, POST_INDEXED, 5);

	setOperation(0xe6, INC, ZERO_PAGE, 5);
	setOperation(0xf6, INC, ZERO_PAGE_X, 6);
	setOperation(0xee, INC, ABSOLUTE_MODE, 6);
	setOperation(0xfe, INC, INDEXED_X, 7);

	setOperation(0xe8, INX, IMPLIED, 2);

	setOperation(0xc8, INY, IMPLIED, 2);

	setOperation(0x4c, JMP, ABSOLUTE_MODE, 3);
	setOperation(0x6c, JMP, INDIRECT, 5);

	setOperation(0x20, JSR, ABSOLUTE_MODE, 6);

	setOperation(0xa9, LDA, IMMEDIATE, 2);
	setOperation(0xa5, LDA, ZERO_PAGE, 3);
	setOperation(0xb5, LDA, ZERO_PAGE_X, 4);
	setOperation(0xad, LDA, ABSOLUTE_MODE, 4);
	setOperation(0xbd, LDA, INDEXED_X, 4);
	setOperation(0xb9, LDA, INDEXED_Y, 4);
	setOperation(0xa1, LDA, PRE_INDEXED, 6);
	setOperation(0xb1, LDA, POST_INDEXED, 5);

	setOperation(0xa2, LDX, IMMEDIATE, 2);
	setOperation(0xa6, LDX, ZERO_PAGE, 3);
	setOperation(0xb6, LDX, ZERO_PAGE_Y, 4);
	setOperation(0xae, LDX, ABSOLUTE_MODE, 4);
	setOperation(0xbe, LDX, INDEXED_Y, 4);

	setOperation(0xa0, LDY, IMMEDIATE, 2);
	setOperation(0xa4, LDY, ZERO_PAGE, 3);
	setOperation(0xb4, LDY, ZERO_PAGE_X, 4);
	setOperation(0xac, LDY, ABSOLUTE_MODE, 4);
	setOperation(0xbc, LDY, INDEXED_X, 4);

	setOperation(0x4a, LSR, ACCUMULATOR, 2);
	setOperation(0x46, LSR, ZERO_PAGE, 5);
	setOperation(0x56, LSR, ZERO_PAGE_X, 6);
	setOperation(0x4e, LSR, ABSOLUTE_MODE, 6);
	setOperation(0x5e, LSR, INDEXED_X, 7);

	setOperation(0xea, NOP, IMPLIED, 2);

	setOperation(0x09, ORA, IMMEDIATE, 2);
	setOperation(0x05, ORA, ZERO_PAGE, 3);
	setOperation(0x15, ORA, ZERO_PAGE_X, 4);
	setOperation(0x0d, ORA, ABSOLUTE_MODE, 4);
	setOperation(0x1d, ORA, INDEXED_X, 4);
	setOperation(0x19, ORA, INDEXED_Y, 4);
	setOperation(0x01, ORA, PRE_INDEXED, 6);
	setOperation(0x11, ORA, POST_INDEXED, 5);

	setOperation(0x48, PHA, IMPLIED, 3);

	setOperation(0x08, PHP, IMPLIED, 3);

	setOperation(0x68, PLA, IMPLIED, 4);

	setOperation(0x28, PLP, IMPLIED, 4);

	setOperation(0x2a, ROL, ACCUMULATOR, 2);
	setOperation(0x26, ROL, ZERO_PAGE, 5);
	setOperation(0x36, ROL, ZERO_PAGE_X, 6);
	setOperation(0x2e, ROL, ABSOLUTE_MODE, 6);
	setOperation(0x3e, ROL, INDEXED_X, 7);

	setOperation(0x6a, ROR, ACCUMULATOR, 2);
	setOperation(0x66, ROR, ZERO_PAGE, 5);
	setOperation(0x76, ROR, ZERO_PAGE_X, 6);
	setOperation(0x6e, ROR, ABSOLUTE_MODE, 6);
	setOperation(0x7e, ROR, INDEXED_X, 7);

	setOperation(0x40, RTI, IMPLIED, 6);

	setOperation(0x60, RTS, IMPLIED, 6);

	setOperation(0xe9, SBC, IMMEDIATE, 2);
	setOperation(0xe5, SBC, ZERO_PAGE, 3);
	setOperation(0xf5, SBC, ZERO_PAGE_X, 4);
	setOperation(0xed, SBC, ABSOLUTE_MODE, 4);
	setOperation(0xfd, SBC, INDEXED_X, 4);
	setOperation(0xf9, SBC, INDEXED_Y, 4);
	setOperation(0xe1, SBC, PRE_INDEXED, 6);
	setOperation(0xf1, SBC, POST_INDEXED, 5);

	setOperation(0x38, SEC, IMPLIED, 2);

	setOperation(0xf8, SED, IMPLIED, 2);

	setOperation(0x78, SEI, IMPLIED, 2);

	setOperation(0x85, STA, ZERO_PAGE, 3);
	setOperation(0x95, STA, ZERO_PAGE_X, 4);
	setOperation(0x8d, STA, ABSOLUTE_MODE, 4);
	setOperation(0x9d, STA, INDEXED_X, 5);
	setOperation(0x99, STA, INDEXED_Y, 5);
	setOperation(0x81, STA, PRE_INDEXED, 6);
	setOperation(0x91, STA, POST_INDEXED, 6);

	setOperation(0x86, STX, ZERO_PAGE, 3);
	setOperation(0x96, STX, ZERO_PAGE_Y, 4);
	setOperation(0x8e, STX, ABSOLUTE_MODE, 4);

	setOperation(0x84, STY, ZERO_PAGE, 3);
	setOperation(0x94, STY, ZERO_PAGE_X, 4);
	setOperation(0x8c, STY, ABSOLUTE_MODE, 4);

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

CPU::~CPU() {}

void CPU::setPPU(PPU* ppu) {
	assert(ppu != NULL, "CPU::setPPU() ppu is null");
	this->ppu = ppu;
}

void CPU::setAPU(APU* apu) {
	assert(apu != NULL, "CPU::setAPU() apu is null");
	this->apu = apu;
}

void CPU::setROM(Byte* data, unsigned int size) {
	assert(data != NULL, "CPU::setROM() data is null");
	rom = data;
	rom_size = size;
}

bool CPU::halted() {
	return _halt;
}

bool CPU::breaked() {
	return status & (1 << BREAK);
}

void CPU::reset() {
	dout("CPU::reset()");
	assert(ppu != NULL, "CPU::reset() ppu is null");
	program_counter = readWord(RESET_VECTOR);
	stack_pointer = STACK_START;
	status = STATUS_START;
	accumulator = 0;
	x_register = 0;
	y_register = 0;
	_halt = false;
	_nmi = false;
	_irq = false;
	wait_cycles = 0;
	odd_cycle = false;
	for(int i = 0; i < RAM_SIZE; i++) {
		ram[i] = 0xff;
	}

	_break = false;
	display_ops_cycles = 0;
}

void CPU::setNMI() {
	// dout("CPU::setNMI()");
	_nmi = true;
}

void CPU::setIRQ() {
	_irq = true;
}
	
Byte CPU::readByte(Word address) {
	Byte value = 0;
	Word actual_address;
	switch(address) {
		case RAM_START ... RAM_END:
			actual_address = mirrorAddress(address, RAM_START, RAM_SIZE);
			assert(actual_address < RAM_SIZE, "READ ram address " << toHex(actual_address, 2) << " out of bounds " << toHex(RAM_SIZE));
			value = ram[actual_address];
			break;
		case PPU_START ... PPU_END:
			value = ppu->readByte(address);
			break;
		case APU_IO_START ... APU_IO_END:
			value = apu->readByte(address);
			dout("apu->readByte(" << toHex(address, 2) << ")");
			break;
		case SRAM_START ... SRAM_END:
			dout("sram->readByte(" << toHex(address, 2) << ")");
			break;
		case ROM_START ... ROM_END:
			actual_address = (address - ROM_START) % rom_size;
			assert(actual_address < rom_size, "READ rom address " << toHex(actual_address, 2) << " out of bounds " << toHex(rom_size));
			value = rom[actual_address];
			break;
		default:
			dout("read from " << toHex(address, 2) << " did not map");
			break;
	}

	debugReadOperation(address, value);

	return value;
}

void CPU::writeByte(Word address, Byte value) {

	debugWriteOperation(address, value);

	Word index;
	switch(address) {
		case RAM_START ... RAM_END:
			index = mirrorAddress(address, RAM_START, RAM_SIZE);
			assert(index < RAM_SIZE, "WRITE ram address " << toHex(index, 2) << " out of bounds " << toHex(RAM_SIZE));
			ram[index] = value;
			break;
		case PPU_START ... PPU_END:
			ppu->writeByte(address, value);
			break;
		case APU_IO_START ... APU_IO_END:
			if (address == OAM_DMA) {
				dout("OAM DMA transfer");
				oamDmaTransfer(value);
			} else {
				dout("apu->writeByte(" << toHex(address, 2) << ")");
				index = mirrorAddress(address, APU_IO_START, APU_IO_SIZE);
				apu->writeByte(index, value);
			}
			break;
		case SRAM_START ... SRAM_END:
			dout("sram->writeByte(" << toHex(address, 2) << ")");
			break;
		default:
			dout("write to " << toHex(address, 2) << " did not map");
	}
}

void CPU::clockTick() {
	if (_halt) return;

	display_ops_cycles -= (display_ops_cycles > 0);
	
	assert(wait_cycles >= 0, "wait cycles below 0");

	if (wait_cycles <= 0) {
		bool do_interrupts = !getStatusFlag(DISABLE_INTERRUPTS);

		// if (_nmi && !do_interrupts) dout("skipping nmi");

		_nmi = _nmi && do_interrupts;
		_irq = _irq && do_interrupts;

		if (_nmi) {
			_nmi = false;
			nmi();
		} else if (_irq) {
			_irq = false;
			irq();
		} else {
			debugSpecific("\npc: " << toHex(program_counter, 2));

			debugProgramPosition(program_counter);

			if (!_break) {
				Byte opcode = readByte(program_counter++);
				Operation op = operations[opcode];
				wait_cycles = op.clock_cycles;

				debugOperation(program_counter-1, opcode);

				InstructionFunction function = instruction_functions[op.instruction];
				(this->*function)(op.address_mode);
			}
		}
	}
	wait_cycles--;
	odd_cycle = !odd_cycle;
}

void CPU::nmi() {
	dout("CPU::nmi()");
	wait_cycles = 7;
	pushWordToStack(program_counter);
	pushByteToStack(status);
	setStatusFlag(DISABLE_INTERRUPTS, Constant<bool, true>());
	program_counter = readWord(NMI_VECTOR);
	dout("pc = " << toHex(program_counter));
}

void CPU::irq() {
	dout("CPU::irq()");
	wait_cycles = 7;
	setStatusFlag(BREAK, Constant<bool, false>());
	pushWordToStack(program_counter);
	pushByteToStack(status);
	setStatusFlag(DISABLE_INTERRUPTS, Constant<bool, true>());
	program_counter = readWord(IRQ_VECTOR);
	dout("pc = " << toHex(program_counter));
}

Word CPU::getAddress(CPU::AddressMode address_mode) {
	Word address = 0;
	Word page1 = 0;
	switch(address_mode) {
		case IMMEDIATE:
			address = program_counter;
			break;

		case ABSOLUTE_MODE:
			address = readWord(program_counter);
			break;

		case ZERO_PAGE:
			address = readByte(program_counter);
			break;

		case ZERO_PAGE_X:
			address = (readByte(program_counter) + x_register) & 255;
			break;

		case ZERO_PAGE_Y:
			address = (readByte(program_counter) + y_register) & 255;
			break;

		case INDEXED_X:
			page1 = readWord(program_counter);
			address = page1 + x_register;
			if ((page1 & 0xff00) != (address & 0xff00)) {
				wait_cycles++;
			}
			break;

		case INDEXED_Y:
			page1 = readWord(program_counter);
			address = page1 + y_register;
			if ((page1 & 0xff00) != (address & 0xff00)) {
				wait_cycles++;
			}
			break;

		case INDIRECT:
			address = readWordBug(readWord(program_counter));
			break;

		case PRE_INDEXED:
			address = readWordBug((readByte(program_counter) + x_register) & 255);
			break;

		case POST_INDEXED:
			address = readWordBug(readByte(program_counter)) + y_register;
			break;

		case RELATIVE_MODE:
			address = program_counter + 1 + (signed char)readByte(program_counter);
			break;

		default:
			dout("Invalid address mode: " << address_mode);
			halt();
	}
	return address;
}

void CPU::incrementProgramCounter(CPU::AddressMode address_mode) {
	int increment = 0;
	switch(address_mode) {
		case IMPLIED:
		case ACCUMULATOR:
			increment = 0;
			break;

		case IMMEDIATE:
		case ZERO_PAGE:
		case ZERO_PAGE_X:
		case ZERO_PAGE_Y:
		case PRE_INDEXED:
		case POST_INDEXED:
		case RELATIVE_MODE:
			increment = 1;
			break;

		case ABSOLUTE_MODE:
		case INDEXED_X:
		case INDEXED_Y:
		case INDIRECT:
			increment = 2;
			break;

		default:
			dout("Invalid address mode: " << address_mode);
			halt();
	}

	program_counter += increment;
}

void CPU::halt() {
	dout("halt");
	_halt = true;
}

Word CPU::readWord(Word address) {
	Word low = readByte(address);
	Word high = readByte(address + 1);
	return (high << 8) | low;
}

Word CPU::readWordBug(Word address) {
	Word low = readByte(address);
	Word high = readByte(((address & 0xff) == 0xff)
		? (address & 0xff00)
		: (address + 1));
	return (high << 8) | low;
}

bool CPU::getStatusFlag(int bit) {
	return status & (1 << bit);
}

template <class Boolean>
void CPU::setStatusFlag(int bit, Boolean value) {
	if (bit == DISABLE_INTERRUPTS) dout("Setting interrupt disable to " << value);

	if (value) {
		status |= (1 << bit);
	} else {
		status &= ~(1 << bit);
	}
}

void CPU::setArithmeticFlags(Byte value) {
	setStatusFlag(NEGATIVE, value & 0x80);
	setStatusFlag(ZERO, value == 0);
}

void CPU::conditionalBranch(bool branch) {
	if (branch) {
		Word page1 = program_counter - 1;
		program_counter = getAddress(RELATIVE_MODE);
		if ((page1 & 0xff00) != (program_counter & 0xff00)) {
			wait_cycles += 2;
		} else {
			wait_cycles++;
		}
		debugSpecific("branch to " << toHex(program_counter));
	} else {
		incrementProgramCounter(RELATIVE_MODE);
		debugSpecific("no branch");
	}
}

void CPU::compareWithValue(CPU::AddressMode address_mode, Byte value) {
	Byte memory_value = readByte(getAddress(address_mode));
	int result = value + twosComp(memory_value);
	setStatusFlag(CARRY, result >= 256);
	setArithmeticFlags(result & 255);
	incrementProgramCounter(address_mode);
	debugSpecific("compare " << (int)memory_value << " with " << (int)value);
	debugStatus(NEGATIVE);
	debugStatus(OVERFLOW);
	debugStatus(ZERO);
}

void CPU::pushByteToStack(Byte value) {
	debugSpecific("push " << toHex(value));
	writeByte(STACK_OFFSET | stack_pointer--, value);
}

void CPU::pushWordToStack(Word value) {
	pushByteToStack(value >> 8);
	pushByteToStack(value & 255);
}

Byte CPU::popByteFromStack() {
	Byte value;
	value = readByte(STACK_OFFSET | ++stack_pointer);
	debugSpecific("pop " << toHex(value));
	return value;
}

Word CPU::popWordFromStack() {
	Word low = popByteFromStack();
	Word high = popByteFromStack();
	return (high << 8) | low;
}

void CPU::addValueToAcc(Byte value) {
	int result = accumulator + value + getStatusFlag(CARRY);

	bool carry = result & 0x100;
	bool overflow = (isNegative(value) == isNegative(accumulator))
		&& (isNegative(value) != isNegative(result));
	setStatusFlag(CARRY, carry);
	setStatusFlag(OVERFLOW, overflow);

	accumulator = result & 255;
	setArithmeticFlags(accumulator);
}

void CPU::addWithCarry(CPU::AddressMode address_mode) {
	Byte value = readByte(getAddress(address_mode));
	addValueToAcc(value);
	incrementProgramCounter(address_mode);
	debugSpecific("acc + " << (int)value << " + " << getStatusFlag(CARRY) << " = " << (int)accumulator);
	debugStatus(NEGATIVE);
	debugStatus(OVERFLOW);
	debugStatus(ZERO);
	debugStatus(CARRY);
}

void CPU::bitwiseAnd(CPU::AddressMode address_mode) {
	Byte value = readByte(getAddress(address_mode));
	accumulator &= value;
	setArithmeticFlags(accumulator);
	incrementProgramCounter(address_mode);
	debugSpecific("acc & " << (int)value << " = " << (int)accumulator);
	debugStatus(NEGATIVE);
	debugStatus(OVERFLOW);
	debugStatus(ZERO);
	debugStatus(CARRY);
}

void CPU::shiftLeft(CPU::AddressMode address_mode) {
	int result;
	if (address_mode == ACCUMULATOR) {
		result = (int)accumulator << 1;
		accumulator = result & 255;
		debugSpecific("acc << 1 = " << (int)accumulator);
	} else {
		Word address = getAddress(address_mode);
		result = (int)readByte(address) << 1;
		writeByte(address, result & 255);
		debugSpecific("mem[" << toHex(address) << "] << 1 = " << (int)result);
	}
	setStatusFlag(CARRY, result >= 256);
	setArithmeticFlags(result & 255);

	incrementProgramCounter(address_mode);
}

void CPU::branchOnCarryClear(CPU::AddressMode address_mode) {
	if (!(address_mode == RELATIVE_MODE)) {
		dout("addressing mode not RELATIVE_MODE at " << toHex(program_counter-1, 2));
		halt();
	}
	conditionalBranch(!getStatusFlag(CARRY));
}

void CPU::branchOnCarrySet(CPU::AddressMode address_mode) {
	if (!(address_mode == RELATIVE_MODE)) {
		dout("addressing mode not RELATIVE_MODE at " << toHex(program_counter-1, 2));
		halt();
	}
	conditionalBranch(getStatusFlag(CARRY));
}

void CPU::branchOnZero(CPU::AddressMode address_mode) {
	if (!(address_mode == RELATIVE_MODE)) {
		dout("addressing mode not RELATIVE_MODE at " << toHex(program_counter-1, 2));
		halt();
	}
	conditionalBranch(getStatusFlag(ZERO));
}

void CPU::testBits(CPU::AddressMode address_mode) {
	Byte value = readByte(getAddress(address_mode));
	setStatusFlag(NEGATIVE, value & 0x80);
	setStatusFlag(OVERFLOW, value & 0x40);
	setStatusFlag(ZERO, value & accumulator);
	incrementProgramCounter(address_mode);
	debugSpecific("test " << (int)value);
	debugStatus(NEGATIVE);
	debugStatus(OVERFLOW);
	debugStatus(ZERO);
}

void CPU::branchOnNegative(CPU::AddressMode address_mode) {
	if (!(address_mode == RELATIVE_MODE)) {
		dout("addressing mode not RELATIVE_MODE at " << toHex(program_counter-1, 2));
		halt();
	}
	conditionalBranch(getStatusFlag(NEGATIVE));
}

void CPU::branchOnNotZero(CPU::AddressMode address_mode) {
	if (!(address_mode == RELATIVE_MODE)) {
		dout("addressing mode not RELATIVE_MODE at " << toHex(program_counter-1, 2));
		halt();
	}
	conditionalBranch(!getStatusFlag(ZERO));
}

void CPU::branchOnPositive(CPU::AddressMode address_mode) {
	if (!(address_mode == RELATIVE_MODE)) {
		dout("addressing mode not RELATIVE_MODE at " << toHex(program_counter-1, 2));
		halt();
	}
	conditionalBranch(!getStatusFlag(NEGATIVE));
}

void CPU::forceBreak(CPU::AddressMode address_mode) {
	if (!(address_mode == IMPLIED)) {
		dout("addressing mode not implied at " << toHex(program_counter-1, 2));
		halt();
	}
	setStatusFlag(BREAK, Constant<bool, true>());
	pushWordToStack(program_counter+1);
	pushByteToStack(status);
	setStatusFlag(DISABLE_INTERRUPTS, Constant<bool, true>());
	program_counter = readWord(IRQ_VECTOR);
	dout("Forced IRQ, pc = " << toHex(program_counter));
}

void CPU::branchOnOverflowClear(CPU::AddressMode address_mode) {
	if (!(address_mode == RELATIVE_MODE)) {
		dout("addressing mode not RELATIVE_MODE at " << toHex(program_counter-1, 2));
		halt();
	}
	conditionalBranch(!getStatusFlag(OVERFLOW));
}

void CPU::branchOnOverflowSet(CPU::AddressMode address_mode) {
	if (!(address_mode == RELATIVE_MODE)) {
		dout("addressing mode not RELATIVE_MODE at " << toHex(program_counter-1, 2));
		halt();
	}
	conditionalBranch(getStatusFlag(OVERFLOW));
}

void CPU::clearCarryFlag(CPU::AddressMode address_mode) {
	if (!(address_mode == IMPLIED)) {
		dout("addressing mode not implied at " << toHex(program_counter-1, 2));
		halt();
	}
	setStatusFlag(CARRY, Constant<bool, false>());
	debugStatus(CARRY);
}

void CPU::clearDecimalFlag(CPU::AddressMode address_mode) {
	if (!(address_mode == IMPLIED)) {
		dout("addressing mode not implied at " << toHex(program_counter-1, 2));
		halt();
	}
	setStatusFlag(DECIMAL, Constant<bool, false>());
	debugStatus(DECIMAL);
}

void CPU::clearInterruptDisableFlag(CPU::AddressMode address_mode) {
	if (!(address_mode == IMPLIED)) {
		dout("addressing mode not implied at " << toHex(program_counter-1, 2));
		halt();
	}
	setStatusFlag(DISABLE_INTERRUPTS, Constant<bool, false>());
	debugStatus(DISABLE_INTERRUPTS);
}

void CPU::clearOverflowFlag(CPU::AddressMode address_mode) {
	if (!(address_mode == IMPLIED)) {
		dout("addressing mode not implied at " << toHex(program_counter-1, 2));
		halt();
	}
	setStatusFlag(OVERFLOW, Constant<bool, false>());
	debugStatus(OVERFLOW);
}

void CPU::compareWithAcc(CPU::AddressMode address_mode) {
	compareWithValue(address_mode, accumulator);
}

void CPU::compareWithX(CPU::AddressMode address_mode) {
	compareWithValue(address_mode, x_register);
}

void CPU::compareWithY(CPU::AddressMode address_mode) {
	compareWithValue(address_mode, y_register);
}

void CPU::decrement(CPU::AddressMode address_mode) {
	Word address = getAddress(address_mode);
	Byte result = readByte(address) - 1;
	setArithmeticFlags(result);
	writeByte(address, result);
	incrementProgramCounter(address_mode);
	debugSpecific("mem[" << toHex(address) << "] - 1 = " << (int)result);
}

void CPU::decrementX(CPU::AddressMode address_mode) {
	if (!(address_mode == IMPLIED)) {
		dout("addressing mode not implied at " << toHex(program_counter-1, 2));
		halt();
	}
	Byte result = x_register - 1;
	setArithmeticFlags(result);
	x_register = result;
	debugSpecific("x - 1 = " << (int)x_register);
}

void CPU::decrementY(CPU::AddressMode address_mode) {
	if (!(address_mode == IMPLIED)) {
		dout("addressing mode not implied at " << toHex(program_counter-1, 2));
		halt();
	}
	Byte result = y_register - 1;
	setArithmeticFlags(result);
	y_register = result;
	debugSpecific("y - 1 = " << (int)y_register);
}

void CPU::exclusiveOr(CPU::AddressMode address_mode) {
	Byte value = readByte(getAddress(address_mode));
	accumulator ^= value;
	setArithmeticFlags(accumulator);
	incrementProgramCounter(address_mode);
	debugSpecific("acc ^ " << (int)value << " = " << (int)accumulator);
}

void CPU::increment(CPU::AddressMode address_mode) {
	Word address = getAddress(address_mode);
	Byte result = readByte(address) + 1;
	setArithmeticFlags(result);
	writeByte(address, result);
	incrementProgramCounter(address_mode);
	debugSpecific("mem[" << toHex(address) << "] + 1 = " << (int)result);
}

void CPU::incrementX(CPU::AddressMode address_mode) {
	if (!(address_mode == IMPLIED)) {
		dout("addressing mode not implied at " << toHex(program_counter-1, 2));
		halt();
	}
	Byte result = x_register + 1;
	setArithmeticFlags(result);
	x_register = result;
	debugSpecific("x + 1 = " << (int)x_register);
}

void CPU::incrementY(CPU::AddressMode address_mode) {
	if (!(address_mode == IMPLIED)) {
		dout("addressing mode not implied at " << toHex(program_counter-1, 2));
		halt();
	}
	Byte result = y_register + 1;
	setArithmeticFlags(result);
	y_register = result;
	debugSpecific("y + 1 = " << (int)y_register);
}

void CPU::jump(CPU::AddressMode address_mode) {
	if (!(address_mode == ABSOLUTE_MODE)) {
		dout("addressing mode not ABSOLUTE_MODE at " << toHex(program_counter-1, 2));
		halt();
	}
	program_counter = readWord(program_counter);
	debugSpecific("pc = " << toHex(program_counter));
}

void CPU::jumpToSubroutine(CPU::AddressMode address_mode) {
	if (!(address_mode == ABSOLUTE_MODE)) {
		dout("addressing mode not ABSOLUTE_MODE at " << toHex(program_counter-1, 2));
		halt();
	}
	pushWordToStack(program_counter + 1);
	debugSpecific("return addr-1 = " << toHex(program_counter + 1, 2));
	program_counter = readWord(program_counter);
	debugSpecific("pc = " << toHex(program_counter));
}

void CPU::loadAcc(CPU::AddressMode address_mode) {
	Word address = getAddress(address_mode);
	accumulator = readByte(address);
	setArithmeticFlags(accumulator);
	incrementProgramCounter(address_mode);
	debugSpecific("acc = mem[" << toHex(address) << "] = " << (int)accumulator);
}

void CPU::loadX(CPU::AddressMode address_mode) {
	Word address = getAddress(address_mode);
	x_register = readByte(address);
	setArithmeticFlags(x_register);
	incrementProgramCounter(address_mode);
	debugSpecific("x = mem[" << toHex(address) << "] = " << (int)x_register);
}

void CPU::loadY(CPU::AddressMode address_mode) {
	Word address = getAddress(address_mode);
	y_register = readByte(address);
	setArithmeticFlags(y_register);
	incrementProgramCounter(address_mode);
	debugSpecific("y = mem[" << toHex(address) << "] = " << (int)y_register);
}

void CPU::shiftRight(CPU::AddressMode address_mode) {
	Byte before;
	Byte after;
	if (address_mode == ACCUMULATOR) {
		before = accumulator;
		accumulator = accumulator >> 1;
		after = accumulator;
		debugSpecific("acc >> 1 = " << (int)accumulator);
	} else {
		Word address = getAddress(address_mode);
		before = readByte(address);
		after = before >> 1;
		writeByte(address, after);
		debugSpecific("mem[" << toHex(address) << "] >> 1 = " << (int)after);
	}
	setStatusFlag(CARRY, before & 1);
	setStatusFlag(ZERO, after == 0);
	incrementProgramCounter(address_mode);
	debugStatus(ZERO);
	debugStatus(CARRY);
}

void CPU::noOperation(CPU::AddressMode address_mode) {
	if (!(address_mode == IMPLIED)) {
		dout("addressing mode not implied at " << toHex(program_counter-1, 2));
		halt();
	}
}


void CPU::bitwiseOr(CPU::AddressMode address_mode) {
	Byte value = readByte(getAddress(address_mode));
	accumulator |= value;
	setArithmeticFlags(accumulator);
	incrementProgramCounter(address_mode);
	debugSpecific("acc | " << toHex(value) << " = " << toHex(accumulator));
	debugStatus(NEGATIVE);
	debugStatus(ZERO);
}

void CPU::pushAcc(CPU::AddressMode address_mode) {
	if (!(address_mode == IMPLIED)) {
		dout("addressing mode not implied at " << toHex(program_counter-1, 2));
		halt();
	}
	pushByteToStack(accumulator);
}

void CPU::pushStatus(CPU::AddressMode address_mode) {
	if (!(address_mode == IMPLIED)) {
		dout("addressing mode not implied at " << toHex(program_counter-1, 2));
		halt();
	}
	pushByteToStack(status);
}

void CPU::popAcc(CPU::AddressMode address_mode) {
	if (!(address_mode == IMPLIED)) {
		dout("addressing mode not implied at " << toHex(program_counter-1, 2));
		halt();
	}
	accumulator = popByteFromStack();
	setArithmeticFlags(accumulator);
	debugSpecific("acc = " << (int)accumulator);
	debugStatus(NEGATIVE);
	debugStatus(ZERO);

}

void CPU::popStatus(CPU::AddressMode address_mode) {
	if (!(address_mode == IMPLIED)) {
		dout("addressing mode not implied at " << toHex(program_counter-1, 2));
		halt();
	}
	status = popByteFromStack();
	debugStatus(NEGATIVE);
	debugStatus(OVERFLOW);
	debugStatus(BREAK);
	debugStatus(DECIMAL);
	debugStatus(DISABLE_INTERRUPTS);
	debugStatus(ZERO);
	debugStatus(CARRY);
}

void CPU::rotateLeft(CPU::AddressMode address_mode) {
	Byte before;
	Byte after;
	if (address_mode == ACCUMULATOR) {
		before = accumulator;
		accumulator = (accumulator << 1) | getStatusFlag(CARRY);
		after = accumulator;
	} else {
		Word address = getAddress(address_mode);
		before = readByte(address);
		after = (before << 1) | getStatusFlag(CARRY);
		writeByte(address, after);
	}
	setStatusFlag(CARRY, before & 0x80);
	setArithmeticFlags(after);
	incrementProgramCounter(address_mode);
	debugStatus(NEGATIVE);
	debugStatus(ZERO);
	debugStatus(CARRY);
}

void CPU::rotateRight(CPU::AddressMode address_mode) {
	Byte before;
	Byte after;
	if (address_mode == ACCUMULATOR) {
		before = accumulator;
		accumulator = (accumulator >> 1) | (getStatusFlag(CARRY) ? 0x80 : 0);
		after = accumulator;
	} else {
		Word address = getAddress(address_mode);
		before = readByte(address);
		after =  (before >> 1) | (getStatusFlag(CARRY) ? 0x80 : 0);
		writeByte(address, after);
	}
	setStatusFlag(CARRY, before & 1);
	setArithmeticFlags(after);
	incrementProgramCounter(address_mode);
	debugStatus(NEGATIVE);
	debugStatus(ZERO);
	debugStatus(CARRY);
}

void CPU::returnFromInterrupt(CPU::AddressMode address_mode) {
	if (!(address_mode == IMPLIED)) {
		dout("addressing mode not implied at " << toHex(program_counter-1, 2));
		halt();
	}
	status = popByteFromStack();
	program_counter = popWordFromStack();
	debugSpecific("pc = " << toHex(program_counter));
	debugStatus(NEGATIVE);
	debugStatus(OVERFLOW);
	debugStatus(BREAK);
	debugStatus(DECIMAL);
	debugStatus(DISABLE_INTERRUPTS);
	debugStatus(ZERO);
	debugStatus(CARRY);
}

void CPU::returnFromSubroutine(CPU::AddressMode address_mode) {
	if (!(address_mode == IMPLIED)) {
		dout("addressing mode not implied at " << toHex(program_counter-1, 2));
		halt();
	}
	program_counter = popWordFromStack() + 1;
	debugSpecific("pc = " << toHex(program_counter));
}

void CPU::subtractFromAcc(CPU::AddressMode address_mode) {
	Byte value = readByte(getAddress(address_mode));
	addValueToAcc(~value + 1);

	debugSpecific("acc - " << (int)value << " - " << getStatusFlag(CARRY) << " = " << (int)accumulator);

	incrementProgramCounter(address_mode);
}

void CPU::setCarryFlag(CPU::AddressMode address_mode) {
	if (!(address_mode == IMPLIED)) {
		dout("addressing mode not implied at " << toHex(program_counter-1, 2));
		halt();
	}
	setStatusFlag(CARRY, Constant<bool, true>());
	debugStatus(CARRY);
}

void CPU::setDecimalFlag(CPU::AddressMode address_mode) {
	if (!(address_mode == IMPLIED)) {
		dout("addressing mode not implied at " << toHex(program_counter-1, 2));
		halt();
	}
	setStatusFlag(DECIMAL, Constant<bool, true>());
	debugStatus(DECIMAL);
}

void CPU::setInterruptDisableFlag(CPU::AddressMode address_mode) {
	if (!(address_mode == IMPLIED)) {
		dout("addressing mode not implied at " << toHex(program_counter-1, 2));
		halt();
	}
	setStatusFlag(DISABLE_INTERRUPTS, Constant<bool, true>());
	debugStatus(DISABLE_INTERRUPTS);
}

void CPU::storeAcc(CPU::AddressMode address_mode) {
	Word address = getAddress(address_mode);
	writeByte(address, accumulator);
	incrementProgramCounter(address_mode);
	debugSpecific("mem[" << toHex(address) << "] = " << (int)accumulator);
}

void CPU::storeX(CPU::AddressMode address_mode) {
	Word address = getAddress(address_mode);
	writeByte(address, x_register);
	incrementProgramCounter(address_mode);
	debugSpecific("mem[" << toHex(address) << "] = " << (int)x_register);
}

void CPU::storeY(CPU::AddressMode address_mode) {
	Word address = getAddress(address_mode);
	writeByte(address, y_register);
	incrementProgramCounter(address_mode);
	debugSpecific("mem[" << toHex(address) << "] = " << (int)y_register);
}

void CPU::transferAccToX(CPU::AddressMode address_mode) {
	if (!(address_mode == IMPLIED)) {
		dout("addressing mode not implied at " << toHex(program_counter-1, 2));
		halt();
	}
	x_register = accumulator;
	setArithmeticFlags(x_register);
	debugSpecific("x = " << (int)x_register);
}

void CPU::transferAccToY(CPU::AddressMode address_mode) {
	if (!(address_mode == IMPLIED)) {
		dout("addressing mode not implied at " << toHex(program_counter-1, 2));
		halt();
	}
	y_register = accumulator;
	setArithmeticFlags(y_register);
	debugSpecific("y = " << (int)y_register);
}

void CPU::transferStackPointerToX(CPU::AddressMode address_mode) {
	if (!(address_mode == IMPLIED)) {
		dout("addressing mode not implied at " << toHex(program_counter-1, 2));
		halt();
	}
	x_register = stack_pointer;
	setArithmeticFlags(x_register);
	debugSpecific("x = " << toHex(x_register) << " (" << (int)(0xff - x_register) << ')');
}

void CPU::transferXToAcc(CPU::AddressMode address_mode) {
	if (!(address_mode == IMPLIED)) {
		dout("addressing mode not implied at " << toHex(program_counter-1, 2));
		halt();
	}
	accumulator = x_register;
	setArithmeticFlags(accumulator);
	debugSpecific("acc = " << (int)accumulator);
}

void CPU::transferXToStackPointer(CPU::AddressMode address_mode) {
	if (!(address_mode == IMPLIED)) {
		dout("addressing mode not implied at " << toHex(program_counter-1, 2));
		halt();
	}
	stack_pointer = x_register;
	debugSpecific("sp = " << toHex(stack_pointer) << " (" << (int)(0xff - stack_pointer) << ')');
}

void CPU::transferYToAcc(CPU::AddressMode address_mode) {
	if (!(address_mode == IMPLIED)) {
		dout("addressing mode not implied at " << toHex(program_counter-1, 2));
		halt();
	}
	accumulator = y_register;
	setArithmeticFlags(accumulator);
	debugSpecific("acc = " << (int)y_register);
}

void CPU::illegalOpcode(CPU::AddressMode address_mode) {
	dout("illegal opcode at " << toHex(program_counter-1, 2));
	halt();
}

void CPU::dump(Word address) {
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

void CPU::dumpStack() {
	std::cout << "Stack:\n";
	for(int i = (STACK_OFFSET | stack_pointer) + 1; i <= (STACK_OFFSET | STACK_START); i++) {
		std::cout << ' ' << toHex(readByte(i), 1) << '\n';
	}
	std::cout << "-----\n";
}

void CPU::dumpState() {
	std::cout << "Accumulator: " << toHex(accumulator) << '\n';
	std::cout << "X register: " << toHex(x_register) << '\n';
	std::cout << "Y register: " << toHex(y_register) << '\n';
	std::cout << "Program counter: " << toHex(program_counter) << '\n';
	std::cout << "Stack pointer: " << toHex(stack_pointer) << '\n';
	std::cout << "Status: NV BDIZC\n";
	std::cout << "        " << std::bitset<8>(status) << '\n';
}

void CPU::addDebugOperation(CPU::DebugOperationType type, CPU::DebugOperationCondition condition, Word address) {
	debug_operations.push_back({
		type,
		condition,
		address
	});
}

void CPU::clearDebugOperations() {
	debug_operations.clear();
}

void CPU::debugReadOperation(Word address, Byte value) {
	for(unsigned int n = 0; n < debug_operations.size(); n++) {
		DebugOperation op = debug_operations[n];
		if (op.condition == READ_FROM && op.address == address) {
			std::cout << "read " << toHex(value, 1) << " from " << toHex(address, 2) << '\n';
			executeDebugOperation(op.type);
		}
	}
}

void CPU::debugWriteOperation(Word address, Byte value) {
	for(unsigned int n = 0; n < debug_operations.size(); n++) {
		DebugOperation op = debug_operations[n];
		if (op.condition == WRITE_TO && op.address == address) {
			std::cout << "wrote " << toHex(value, 1) << " to " << toHex(address, 2) << '\n';
			executeDebugOperation(op.type);
		}
	}
}

void CPU::debugProgramPosition(Word address) {
	for(unsigned int n = 0; n < debug_operations.size(); n++) {
		DebugOperation op = debug_operations[n];
		if (op.condition == PROGRAM_POSITION && op.address == address) {
			std::cout << "reached PC " << toHex(address, 2) << '\n';
			executeDebugOperation(op.type);
		}
	}
}

void CPU::executeDebugOperation(CPU::DebugOperationType type) {
	switch(type) {
		case BREAKPOINT:
			_break = true;
			break;

		case DISPLAY_OPS:
			display_ops_cycles = 10;
			break;

		case DUMP:
			dumpState();
			dumpStack();
			dump(program_counter);
			break;
	}
}

void CPU::oamDmaTransfer(Byte high) {
	dout("OAM DMA TRANSFER");
	Word address = high << 8;
	for(int index = 0; index < 256; index++) {
		ppu->writeToOAM(index, readByte(address + index));
	}
	wait_cycles += 513 + odd_cycle;
}