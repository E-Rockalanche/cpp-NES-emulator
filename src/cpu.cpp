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
#define twosComp(byte) ((Byte)(1 + ~(byte)))
#define inRange(value, low, high) ((value) >= (low) && (value) <= (high))
#define setOverflow(reg, value, result) \
	setStatusFlag(OVERFLOW, ~((reg) ^ (value)) & ((reg) ^ (result)) & 0x80)

#define DEBUG_OPERATION true
#define DEBUG_VALUES true

#if (DEBUG_OPERATION)
	#define debugOperation(pc, opcode) if (debug) std::cout \
		<< std::string(subroutine_depth, ' ') << "[" << toHex(pc, 2) \
		<< "] op: " << toHex(opcode, 1) << ", " \
		<< instruction_names[operations[opcode].instruction] << ' ' \
		<< ((operations[opcode].address_mode != IMPLIED) \
			? address_mode_names[operations[opcode].address_mode] \
			: "") \
		<< '\n'
#else
	#define debugOperation(pc, opcode)
#endif

#if (DEBUG_VALUES)
	#define debugSpecific(message) if (debug) std::cout \
		<< std::string(subroutine_depth, ' ') << message << '\n'
	#define debugStatus(flag) if (debug) std::cout \
		<< std::string(subroutine_depth, ' ') << #flag << ": " \
		<< getStatusFlag(flag) << '\n'
#else
	#define debugSpecific(message)
	#define debugStatus(flag)
#endif

int test_ticks;

bool in_nmi = false;

#define cross(addr1, addr2) (((addr1) & 0xff00) != ((addr2) & 0xff00))

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
	"ABSOLUTE_X",
	"ABSOLUTE_X (store)",
	"ABSOLUTE_Y",
	"INDIRECT",
	"INDIRECT_X",
	"INDIRECT_Y",
	"RELATIVE",
};

CPU::CPU() {
	dout("CPU()");

	for(unsigned int i = 0; i < 256; i++) {
		setOperation(i, ILL, IMPLIED, 0);
	}

	controllers[0] = NULL;
	controllers[1] = NULL;

	setOperation(0x69, ADC, IMMEDIATE, 2);
	setOperation(0x65, ADC, ZERO_PAGE, 3);
	setOperation(0x75, ADC, ZERO_PAGE_X, 4);
	setOperation(0x6d, ADC, ABSOLUTE_MODE, 4);
	setOperation(0x7d, ADC, ABSOLUTE_X, 4);
	setOperation(0x79, ADC, ABSOLUTE_Y, 4);
	setOperation(0x61, ADC, INDIRECT_X, 6);
	setOperation(0x71, ADC, INDIRECT_Y, 5);

	setOperation(0x29, AND, IMMEDIATE, 2);
	setOperation(0x25, AND, ZERO_PAGE, 3);
	setOperation(0x35, AND, ZERO_PAGE_X, 4);
	setOperation(0x2d, AND, ABSOLUTE_MODE, 4);
	setOperation(0x3d, AND, ABSOLUTE_X, 4);
	setOperation(0x39, AND, ABSOLUTE_Y, 4);
	setOperation(0x21, AND, INDIRECT_X, 6);
	setOperation(0x31, AND, INDIRECT_Y, 5);

	setOperation(0x0a, ASL, ACCUMULATOR, 2);
	setOperation(0x06, ASL, ZERO_PAGE, 5);
	setOperation(0x16, ASL, ZERO_PAGE_X, 6);
	setOperation(0x0e, ASL, ABSOLUTE_MODE, 6);
	setOperation(0x1e, ASL, ABSOLUTE_X_STORE, 7);

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
	setOperation(0xdd, CMP, ABSOLUTE_X, 4);
	setOperation(0xd9, CMP, ABSOLUTE_Y, 4);
	setOperation(0xc1, CMP, INDIRECT_X, 6);
	setOperation(0xd1, CMP, INDIRECT_Y, 5);

	setOperation(0xe0, CPX, IMMEDIATE, 2);
	setOperation(0xe4, CPX, ZERO_PAGE, 3);
	setOperation(0xec, CPX, ABSOLUTE_MODE, 4);

	setOperation(0xc0, CPY, IMMEDIATE, 2);
	setOperation(0xc4, CPY, ZERO_PAGE, 3);
	setOperation(0xcc, CPY, ABSOLUTE_MODE, 4);

	setOperation(0xc6, DEC, ZERO_PAGE, 5);
	setOperation(0xd6, DEC, ZERO_PAGE_X, 6);
	setOperation(0xce, DEC, ABSOLUTE_MODE, 6);
	setOperation(0xde, DEC, ABSOLUTE_X_STORE, 7);

	setOperation(0xca, DEX, IMPLIED, 2);

	setOperation(0x88, DEY, IMPLIED, 2);

	setOperation(0x49, EOR, IMMEDIATE, 2);
	setOperation(0x45, EOR, ZERO_PAGE, 3);
	setOperation(0x55, EOR, ZERO_PAGE_X, 4);
	setOperation(0x4d, EOR, ABSOLUTE_MODE, 4);
	setOperation(0x5d, EOR, ABSOLUTE_X, 4);
	setOperation(0x59, EOR, ABSOLUTE_Y, 4);
	setOperation(0x41, EOR, INDIRECT_X, 6);
	setOperation(0x51, EOR, INDIRECT_Y, 5);

	setOperation(0xe6, INC, ZERO_PAGE, 5);
	setOperation(0xf6, INC, ZERO_PAGE_X, 6);
	setOperation(0xee, INC, ABSOLUTE_MODE, 6);
	setOperation(0xfe, INC, ABSOLUTE_X_STORE, 7);

	setOperation(0xe8, INX, IMPLIED, 2);

	setOperation(0xc8, INY, IMPLIED, 2);

	setOperation(0x4c, JMP, ABSOLUTE_MODE, 3);
	setOperation(0x6c, JMP, INDIRECT, 5);

	setOperation(0x20, JSR, ABSOLUTE_MODE, 6);

	setOperation(0xa9, LDA, IMMEDIATE, 2);
	setOperation(0xa5, LDA, ZERO_PAGE, 3);
	setOperation(0xb5, LDA, ZERO_PAGE_X, 4);
	setOperation(0xad, LDA, ABSOLUTE_MODE, 4);
	setOperation(0xbd, LDA, ABSOLUTE_X, 4);
	setOperation(0xb9, LDA, ABSOLUTE_Y, 4);
	setOperation(0xa1, LDA, INDIRECT_X, 6);
	setOperation(0xb1, LDA, INDIRECT_Y, 5);

	setOperation(0xa2, LDX, IMMEDIATE, 2);
	setOperation(0xa6, LDX, ZERO_PAGE, 3);
	setOperation(0xb6, LDX, ZERO_PAGE_Y, 4);
	setOperation(0xae, LDX, ABSOLUTE_MODE, 4);
	setOperation(0xbe, LDX, ABSOLUTE_Y, 4);

	setOperation(0xa0, LDY, IMMEDIATE, 2);
	setOperation(0xa4, LDY, ZERO_PAGE, 3);
	setOperation(0xb4, LDY, ZERO_PAGE_X, 4);
	setOperation(0xac, LDY, ABSOLUTE_MODE, 4);
	setOperation(0xbc, LDY, ABSOLUTE_X, 4);

	setOperation(0x4a, LSR, ACCUMULATOR, 2);
	setOperation(0x46, LSR, ZERO_PAGE, 5);
	setOperation(0x56, LSR, ZERO_PAGE_X, 6);
	setOperation(0x4e, LSR, ABSOLUTE_MODE, 6);
	setOperation(0x5e, LSR, ABSOLUTE_X_STORE, 7);

	setOperation(0xea, NOP, IMPLIED, 2);

	setOperation(0x09, ORA, IMMEDIATE, 2);
	setOperation(0x05, ORA, ZERO_PAGE, 3);
	setOperation(0x15, ORA, ZERO_PAGE_X, 4);
	setOperation(0x0d, ORA, ABSOLUTE_MODE, 4);
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
	setOperation(0x2e, ROL, ABSOLUTE_MODE, 6);
	setOperation(0x3e, ROL, ABSOLUTE_X_STORE, 7);

	setOperation(0x6a, ROR, ACCUMULATOR, 2);
	setOperation(0x66, ROR, ZERO_PAGE, 5);
	setOperation(0x76, ROR, ZERO_PAGE_X, 6);
	setOperation(0x6e, ROR, ABSOLUTE_MODE, 6);
	setOperation(0x7e, ROR, ABSOLUTE_X_STORE, 7);

	setOperation(0x40, RTI, IMPLIED, 6);

	setOperation(0x60, RTS, IMPLIED, 6);

	setOperation(0xe9, SBC, IMMEDIATE, 2);
	setOperation(0xe5, SBC, ZERO_PAGE, 3);
	setOperation(0xf5, SBC, ZERO_PAGE_X, 4);
	setOperation(0xed, SBC, ABSOLUTE_MODE, 4);
	setOperation(0xfd, SBC, ABSOLUTE_X, 4);
	setOperation(0xf9, SBC, ABSOLUTE_Y, 4);
	setOperation(0xe1, SBC, INDIRECT_X, 6);
	setOperation(0xf1, SBC, INDIRECT_Y, 5);

	setOperation(0x38, SEC, IMPLIED, 2);

	setOperation(0xf8, SED, IMPLIED, 2);

	setOperation(0x78, SEI, IMPLIED, 2);

	setOperation(0x85, STA, ZERO_PAGE, 3);
	setOperation(0x95, STA, ZERO_PAGE_X, 4);
	setOperation(0x8d, STA, ABSOLUTE_MODE, 4);
	setOperation(0x9d, STA, ABSOLUTE_X_STORE, 5);
	setOperation(0x99, STA, ABSOLUTE_Y, 5);
	setOperation(0x81, STA, INDIRECT_X, 6);
	setOperation(0x91, STA, INDIRECT_Y, 6);

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

void CPU::setCartridge(Cartridge* cartridge) {
	assert(cartridge != NULL, "CPU::setCartridge() cartridge is null");
	this->cartridge = cartridge;
}

void CPU::setController(Controller* controller, unsigned int port) {
	assert(port < 2, "CPU::setController() invalid port");
	controllers[port] = controller;
}

bool CPU::halted() {
	return _halt;
}

bool CPU::breaked() {
	return status & (1 << BREAK);
}

void CPU::power() {
	assert(ppu != NULL, "CPU::reset() ppu is null");
	assert(cartridge != NULL, "CPU::reset() cartridge is null");

	stack_pointer = STACK_START;
	status = STATUS_START;
	accumulator = 0;
	x_register = 0;
	y_register = 0;

	writeByte(APU_STATUS, 0);
	writeByte(APU_FRAME_COUNT, 0);
	for(int n = 0; n < 16; n++) {
		writeByte(APU_START + n, 0);
	}

	program_counter = readWord(RESET_VECTOR);

	_halt = false;
	_nmi = -1;
	_irq = false;
	wait_cycles = 0;
	odd_cycle = false;

	for(int i = 0; i < RAM_SIZE; i++) {
		ram[i] = 0xff;
	}

	_break = false;
}

void CPU::reset() {
	stack_pointer -= 3;
	setStatusFlag(DISABLE_INTERRUPTS, true);
	writeByte(APU_STATUS, 0);
	
	program_counter = readWord(RESET_VECTOR);

	_halt = false;
	_nmi = -1;
	_irq = false;
	wait_cycles = 0;
	odd_cycle = false;

	_break = false;
}

void CPU::setNMI() {
	_nmi = 0; // time since set
}

void CPU::clearNMI() {
	_nmi = -1; // time since set
}

void CPU::setIRQ() {
	_irq = true;
}

void CPU::clockTick() {
	if (_nmi >= 0) _nmi++;

	odd_cycle = !odd_cycle;
	//apu->clockTick();
	ppu->clockTick();
	ppu->clockTick();
	ppu->clockTick();


	test_ticks++; // debug
}
	
Byte CPU::readByte(Word address) {
	Byte value = 0;
	clockTick();
	switch(address) {
		case RAM_START ... RAM_END:
			value = ram[(address - RAM_START) % RAM_SIZE];
			break;
		
		case PPU_START ... PPU_END:
			value = ppu->readByte((address - PPU_START) % PPU_SIZE);
			break;

		case APU_START ... APU_END:
			if (address == PPU::OAM_DMA) {
				dout("trying to read from OAM_DMA");
				break;
			}

			value = apu->readByte(address - APU_START);
			dout("apu->readByte(" << toHex(address, 2) << ")");
			break;

		case JOY1 ... JOY2:
			{
				Controller* controller = controllers[address - JOY1];
				if (controller) value = controller->read();
			}
			break;

		case 0x4018 ... 0x401f:
			dout("read from disabled");
			break;

		case CARTRIDGE_START ... CARTRIDGE_END:
			value = cartridge->readPRG(address);
			break;

		default:
			assert(false, "invalid address");
			break;
	}

	debugReadOperation(address, value);

	return value;
}

void CPU::writeByte(Word address, Byte value) {
	clockTick();

	debugWriteOperation(address, value);

	switch(address) {
		case RAM_START ... RAM_END:
			ram[(address - RAM_START) % RAM_SIZE] = value;
			break;

		case PPU_START ... PPU_END:
			ppu->writeByte((address - PPU_START) % PPU_SIZE, value);
			break;

		case APU_START ... APU_END:
		case JOY2:
			if (address == OAM_DMA) {
				oamDmaTransfer(value);
			} else {
				apu->writeByte(address - APU_START, value);
			}
			break;

		case JOY1:
			{
				Controller* controller = controllers[address - JOY1];
				if (controller) controller->write(value);
			}
			break;

		case 0x4018 ... 0x401f:
			// disabled
			break;

		case CARTRIDGE_START ... CARTRIDGE_END:
			cartridge->writePRG(address, value);
			break;

		default:
			assert(false, "invalid address");
			break;
	}
}

void CPU::execute() {
	wait_cycles = 0;
	test_ticks = 0;

	if (!_halt) {
		bool do_interrupts = !getStatusFlag(DISABLE_INTERRUPTS);
		_irq = _irq && do_interrupts;

		if (_nmi > 0) {
			_nmi = -1;
			nmi();

			if (test_ticks != wait_cycles) {
				std::cout << '\n';
				dout("NMI");
				dout("wait cycles: " << wait_cycles);
				dout("test ticks: " << test_ticks);
			}
		} else if (_irq) {
			_irq = false;
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
			(this->*function)(op.address_mode);

			if (_halt) dout("opcode: " << toHex(opcode));

			if (test_ticks != wait_cycles) {
				std::cout << '\n';
				dout("instruction: " << instruction_names[op.instruction]);
				dout("address mode: " << address_mode_names[op.address_mode]);
				dout("wait cycles: " << wait_cycles);
				dout("test ticks: " << test_ticks);
			}
		}

		debugProgramPosition(program_counter);
	}
}

void CPU::nmi() {
	wait_cycles = 7;
	clockTick();
	clockTick();
	pushWordToStack(program_counter);
	pushByteToStack(status);
	setStatusFlag(DISABLE_INTERRUPTS, Constant<bool, true>());
	program_counter = readWord(NMI_VECTOR);

	debugInterrupt(CPU::NMI);
	in_nmi = true;
}

void CPU::irq() {
	wait_cycles = 7;
	clockTick();
	clockTick();
	setStatusFlag(BREAK, Constant<bool, false>());
	pushWordToStack(program_counter);
	pushByteToStack(status);
	setStatusFlag(DISABLE_INTERRUPTS, Constant<bool, true>());
	program_counter = readWord(IRQ_VECTOR);

	debugInterrupt(CPU::IRQ);
}

Word CPU::getAddress(CPU::AddressMode address_mode) {
	Word address = 0;
	Word page1 = 0;
	Byte zp = 0;
	switch(address_mode) {
		case IMMEDIATE:
			address = program_counter++;
			break;

		case ABSOLUTE_MODE:
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
			if (cross(page1, address)) {
				wait_cycles++;
				clockTick();
			}
			break;

		case ABSOLUTE_X_STORE:
			page1 = readWord(program_counter);
			program_counter += 2;
			address = page1 + x_register;
			clockTick();
			break;

		case ABSOLUTE_Y:
			page1 = readWord(program_counter);
			program_counter += 2;
			address = page1 + y_register;
			if (cross(page1, address)) {
				wait_cycles++;
				clockTick();
			}
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
			if (cross(address - y_register, address)) {
				clockTick();
				wait_cycles++;
			}
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
	int offset = (signed char)readByte(program_counter++);
	int page1 = program_counter;
	if (branch) {
		clockTick();
		wait_cycles++;
		program_counter += offset;

		if (cross(page1, program_counter)) {
			wait_cycles++;
			clockTick();
		}
		debugSpecific("branch to " << toHex(program_counter));
	} else {
		debugSpecific("no branch");
	}
}

void CPU::compareWithValue(CPU::AddressMode address_mode, Byte reg) {
	Byte value = readByte(getAddress(address_mode));
	setStatusFlag(CARRY, reg >= value);
	setArithmeticFlags(reg - value);

	debugSpecific("compare " << (int)value << " with " << (int)reg);
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
	unsigned int result = accumulator + value + getStatusFlag(CARRY);
	bool carry = result > 0xff;
	setOverflow(accumulator, value, result);
	setStatusFlag(CARRY, carry);
	accumulator = result & 0xff;
	setArithmeticFlags(accumulator);
}

void CPU::addWithCarry(CPU::AddressMode address_mode) {
	Byte value = readByte(getAddress(address_mode));
	addValueToAcc(value);

	debugSpecific("acc + " << (int)value << " + " << getStatusFlag(CARRY)
		<< " = " << (int)accumulator);
	debugStatus(NEGATIVE);
	debugStatus(OVERFLOW);
	debugStatus(ZERO);
	debugStatus(CARRY);
}

void CPU::bitwiseAnd(CPU::AddressMode address_mode) {
	Byte value = readByte(getAddress(address_mode));
	accumulator &= value;
	setArithmeticFlags(accumulator);

	debugSpecific("acc & " << (int)value << " = " << (int)accumulator);
	debugStatus(NEGATIVE);
	debugStatus(OVERFLOW);
	debugStatus(ZERO);
	debugStatus(CARRY);
}

void CPU::shiftLeft(CPU::AddressMode address_mode) {
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

void CPU::branchOnCarryClear(CPU::AddressMode address_mode) {
	conditionalBranch(!getStatusFlag(CARRY));
}

void CPU::branchOnCarrySet(CPU::AddressMode address_mode) {
	conditionalBranch(getStatusFlag(CARRY));
}

void CPU::branchOnZero(CPU::AddressMode address_mode) {
	conditionalBranch(getStatusFlag(ZERO));
}

void CPU::testBits(CPU::AddressMode address_mode) {
	Byte value = readByte(getAddress(address_mode));
	setStatusFlag(NEGATIVE, value & (1 << NEGATIVE));
	setStatusFlag(OVERFLOW, value & (1 << OVERFLOW));
	setStatusFlag(ZERO, (value & accumulator) == 0);

	debugSpecific("test " << (int)value);
	debugStatus(NEGATIVE);
	debugStatus(OVERFLOW);
	debugStatus(ZERO);
}

void CPU::branchOnNegative(CPU::AddressMode address_mode) {
	conditionalBranch(getStatusFlag(NEGATIVE));
}

void CPU::branchOnNotZero(CPU::AddressMode address_mode) {
	conditionalBranch(!getStatusFlag(ZERO));
}

void CPU::branchOnPositive(CPU::AddressMode address_mode) {
	conditionalBranch(!getStatusFlag(NEGATIVE));
}

void CPU::forceBreak(CPU::AddressMode address_mode) {
	readByte(program_counter); // dummy read
	setStatusFlag(BREAK, Constant<bool, true>());
	pushWordToStack(program_counter+1);
	pushByteToStack(status);
	setStatusFlag(DISABLE_INTERRUPTS, Constant<bool, true>());
	program_counter = readWord(IRQ_VECTOR);
	dout("Forced IRQ, pc = " << toHex(program_counter));
}

void CPU::branchOnOverflowClear(CPU::AddressMode address_mode) {
	conditionalBranch(!getStatusFlag(OVERFLOW));
}

void CPU::branchOnOverflowSet(CPU::AddressMode address_mode) {
	conditionalBranch(getStatusFlag(OVERFLOW));
}

void CPU::clearCarryFlag(CPU::AddressMode address_mode) {
	clockTick();
	setStatusFlag(CARRY, Constant<bool, false>());

	debugStatus(CARRY);
}

void CPU::clearDecimalFlag(CPU::AddressMode address_mode) {
	clockTick();
	setStatusFlag(DECIMAL, Constant<bool, false>());

	debugStatus(DECIMAL);
}

void CPU::clearInterruptDisableFlag(CPU::AddressMode address_mode) {
	clockTick();
	setStatusFlag(DISABLE_INTERRUPTS, Constant<bool, false>());

	debugStatus(DISABLE_INTERRUPTS);
}

void CPU::clearOverflowFlag(CPU::AddressMode address_mode) {
	clockTick();
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
	clockTick();
	setArithmeticFlags(result);
	writeByte(address, result);

	debugSpecific("mem[" << toHex(address) << "] - 1 = " << (int)result);
}

void CPU::decrementX(CPU::AddressMode address_mode) {
	clockTick();
	setArithmeticFlags(--x_register);

	debugSpecific("x - 1 = " << (int)x_register);
}

void CPU::decrementY(CPU::AddressMode address_mode) {
	clockTick();
	setArithmeticFlags(--y_register);

	debugSpecific("y - 1 = " << (int)y_register);
}

void CPU::exclusiveOr(CPU::AddressMode address_mode) {
	Byte value = readByte(getAddress(address_mode));
	accumulator ^= value;
	setArithmeticFlags(accumulator);

	debugSpecific("acc ^ " << (int)value << " = " << (int)accumulator);
}

void CPU::increment(CPU::AddressMode address_mode) {
	Word address = getAddress(address_mode);
	Byte result = readByte(address) + 1;
	clockTick();
	setArithmeticFlags(result);

	// indirect x dummy read here

	writeByte(address, result);

	debugSpecific("mem[" << toHex(address) << "] + 1 = " << (int)result);
}

void CPU::incrementX(CPU::AddressMode address_mode) {
	clockTick();
	setArithmeticFlags(++x_register);

	debugSpecific("x + 1 = " << (int)x_register);
}

void CPU::incrementY(CPU::AddressMode address_mode) {
	clockTick();
	setArithmeticFlags(++y_register);

	debugSpecific("y + 1 = " << (int)y_register);
}

void CPU::jump(CPU::AddressMode address_mode) {
	program_counter = getAddress(address_mode);
	debugSpecific("pc = " << toHex(program_counter));
}

void CPU::jumpToSubroutine(CPU::AddressMode address_mode) {
	clockTick();
	pushWordToStack(program_counter + 1);

	debugSpecific("return addr = " << toHex(program_counter + 2, 2));

	program_counter = readWord(program_counter);

	debugSpecific("pc = " << toHex(program_counter));
	subroutine_depth++;
}

void CPU::loadAcc(CPU::AddressMode address_mode) {
	Word address = getAddress(address_mode);
	accumulator = readByte(address);
	setArithmeticFlags(accumulator);

	debugSpecific("acc = mem[" << toHex(address) << "] = " << (int)accumulator);
}

void CPU::loadX(CPU::AddressMode address_mode) {
	Word address = getAddress(address_mode);
	x_register = readByte(address);
	setArithmeticFlags(x_register);

	debugSpecific("x = mem[" << toHex(address) << "] = " << (int)x_register);
}

void CPU::loadY(CPU::AddressMode address_mode) {
	Word address = getAddress(address_mode);
	y_register = readByte(address);
	setArithmeticFlags(y_register);

	debugSpecific("y = mem[" << toHex(address) << "] = " << (int)y_register);
}

void CPU::shiftRight(CPU::AddressMode address_mode) {
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

void CPU::noOperation(CPU::AddressMode address_mode) {
	clockTick();
}


void CPU::bitwiseOr(CPU::AddressMode address_mode) {
	Byte value = readByte(getAddress(address_mode));
	accumulator |= value;
	setArithmeticFlags(accumulator);

	debugSpecific("acc | " << toHex(value) << " = " << toHex(accumulator));
	debugStatus(NEGATIVE);
	debugStatus(ZERO);
}

void CPU::pushAcc(CPU::AddressMode address_mode) {
	clockTick();
	pushByteToStack(accumulator);
}

void CPU::pushStatus(CPU::AddressMode address_mode) {
	clockTick();
	pushByteToStack(status | 0x30);
}

void CPU::popAcc(CPU::AddressMode address_mode) {
	readByte(program_counter); // dummy read
	clockTick();
	accumulator = popByteFromStack();
	setArithmeticFlags(accumulator);

	debugSpecific("acc = " << (int)accumulator);
	debugStatus(NEGATIVE);
	debugStatus(ZERO);

}

void CPU::popStatus(CPU::AddressMode address_mode) {
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

void CPU::rotateLeft(CPU::AddressMode address_mode) {
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

void CPU::rotateRight(CPU::AddressMode address_mode) {
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

void CPU::returnFromInterrupt(CPU::AddressMode address_mode) {
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

void CPU::returnFromSubroutine(CPU::AddressMode address_mode) {
	readByte(program_counter); // dummy read
	program_counter = popWordFromStack() + 1;
	clockTick();
	clockTick();

	debugSpecific("pc = " << toHex(program_counter));
	subroutine_depth--;
}

void CPU::subtractFromAcc(CPU::AddressMode address_mode) {
	Byte value = readByte(getAddress(address_mode));
	addValueToAcc(~value);

	debugSpecific("acc - " << (int)value << " - " << getStatusFlag(CARRY)
		<< " = " << (int)accumulator);
}

void CPU::setCarryFlag(CPU::AddressMode address_mode) {
	clockTick();
	setStatusFlag(CARRY, Constant<bool, true>());

	debugStatus(CARRY);
}

void CPU::setDecimalFlag(CPU::AddressMode address_mode) {
	clockTick();
	setStatusFlag(DECIMAL, Constant<bool, true>());

	debugStatus(DECIMAL);
}

void CPU::setInterruptDisableFlag(CPU::AddressMode address_mode) {
	clockTick();
	setStatusFlag(DISABLE_INTERRUPTS, Constant<bool, true>());

	debugStatus(DISABLE_INTERRUPTS);
}

void CPU::storeAcc(CPU::AddressMode address_mode) {
	switch(address_mode) {
		case INDIRECT_Y:
		case ABSOLUTE_X:
		case ABSOLUTE_Y:
			clockTick();
			break;
	}

	Word address = getAddress(address_mode);
	writeByte(address, accumulator);

	debugSpecific("mem[" << toHex(address) << "] = " << (int)accumulator);
}

void CPU::storeX(CPU::AddressMode address_mode) {
	Word address = getAddress(address_mode);
	writeByte(address, x_register);

	debugSpecific("mem[" << toHex(address) << "] = " << (int)x_register);
}

void CPU::storeY(CPU::AddressMode address_mode) {
	Word address = getAddress(address_mode);
	writeByte(address, y_register);

	debugSpecific("mem[" << toHex(address) << "] = " << (int)y_register);
}

void CPU::transferAccToX(CPU::AddressMode address_mode) {
	x_register = accumulator;
	setArithmeticFlags(x_register);
	clockTick();

	debugSpecific("x = " << (int)x_register);
}

void CPU::transferAccToY(CPU::AddressMode address_mode) {
	y_register = accumulator;
	setArithmeticFlags(y_register);
	clockTick();
	
	debugSpecific("y = " << (int)y_register);
}

void CPU::transferStackPointerToX(CPU::AddressMode address_mode) {
	x_register = stack_pointer;
	setArithmeticFlags(x_register);
	clockTick();
	
	debugSpecific("x = " << toHex(x_register) << " (" << (int)(0xff - x_register) << ')');
}

void CPU::transferXToAcc(CPU::AddressMode address_mode) {
	accumulator = x_register;
	setArithmeticFlags(accumulator);
	clockTick();
	
	debugSpecific("acc = " << (int)accumulator);
}

void CPU::transferXToStackPointer(CPU::AddressMode address_mode) {
	stack_pointer = x_register;
	clockTick();

	debugSpecific("sp = " << toHex(stack_pointer) << " (" << (int)(0xff - stack_pointer) << ')');
}

void CPU::transferYToAcc(CPU::AddressMode address_mode) {
	accumulator = y_register;
	setArithmeticFlags(accumulator);
	clockTick();
	
	debugSpecific("acc = " << (int)y_register);
}

void CPU::illegalOpcode(CPU::AddressMode address_mode) {
	dout("illegal opcode at " << toHex(program_counter-1, 2));
	dout("address mode: " << address_mode);
	halt();
}

void CPU::oamDmaTransfer(Byte high) {
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
		ppu->writeToOAM(data);
	}
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

void CPU::addBreakpoint(Word address, CPU::BreakpointCondition condition) {
	breakpoints.push_back({
		condition,
		address
	});
}

void CPU::clearBreakpoints() {
	breakpoints.clear();
}

void CPU::debugReadOperation(Word address, Byte value) {
	for(unsigned int n = 0; n < breakpoints.size(); n++) {
		Breakpoint op = breakpoints[n];
		if (op.condition == READ_FROM && op.address == address) {
			std::cout << "read " << toHex(value, 1) << " from " << toHex(address, 2) << '\n';
			_break = true;
		}
	}
}

void CPU::debugWriteOperation(Word address, Byte value) {
	for(unsigned int n = 0; n < breakpoints.size(); n++) {
		Breakpoint op = breakpoints[n];
		if (op.condition == WRITE_TO && op.address == address) {
			std::cout << "wrote " << toHex(value, 1) << " to " << toHex(address, 2) << '\n';
			_break = true;
		}
	}
}

void CPU::debugProgramPosition(Word address) {
	for(unsigned int n = 0; n < breakpoints.size(); n++) {
		Breakpoint op = breakpoints[n];
		if (op.condition == PROGRAM_POSITION && op.address == address) {
			std::cout << "reached PC " << toHex(address, 2) << '\n';
			_break = true;
		}
	}
}

void CPU::debugInterrupt(BreakpointCondition condition) {
	for(unsigned int n = 0; n < breakpoints.size(); n++) {
		Breakpoint op = breakpoints[n];
		if (op.condition == condition) {
			std::string name;
			switch(condition) {
				case CPU::IRQ: name = "IRQ"; break;
				case CPU::NMI: name = "NMI"; break;
				default:
					assert(false, "interrupt breakpoint not an interrupt");
					break;
			}
			std::cout << name << ", PC: " << toHex(program_counter, 2) << '\n';
			_break = true;
		}
	}
}