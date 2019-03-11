#ifndef CPU_HPP
#define CPU_HPP

class CPU;

#include <vector>
#include "ppu.hpp"
#include "apu.hpp"
#include "common.hpp"
#include "controller.hpp"
#include "cartridge.hpp"

class CPU {
public:
	CPU();
	~CPU();

	void setPPU(PPU* ppu);
	void setAPU(APU* apu);
	void setCartridge(Cartridge* cartridge);
	void setController(Controller* controller, unsigned int port);

	void reset();
	void power();
	void execute();
	void setNMI();
	void clearNMI();
	void setIRQ();
	bool halted();
	bool breaked();

	void dump(Word address);
	void dumpStack();
	void dumpState();

	enum BreakpointCondition {
		PROGRAM_POSITION,
		READ_FROM,
		WRITE_TO,
		NMI,
		IRQ
	};

	void addBreakpoint(Word address, BreakpointCondition condition);
	void clearBreakpoints();
	
	bool _break;
	bool debug = false;

private:
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

	enum AddressMode {
		IMMEDIATE,
		/*
		operand's value given in instruction (next byte)
		ex) LDA #$0a
		*/

		ABSOLUTE_MODE,
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

	static const int RAM_START = 0x0000;
	static const int RAM_SIZE = 0x0800;
	static const int RAM_END = 0x1fff;
	/*
	memory from 0x0800-0x1fff mirrors internal ram memory
	*/

	static const int PPU_START = 0x2000;
	static const int PPU_SIZE = 0x0008;
	static const int PPU_END = 0x3fff;
	/*
	memory from 0x2008-0x3fff mirrors ppu registers
	*/

	static const int OAM_DMA = 0x4014;

	static const int APU_START = 0x4000;
	static const int APU_SIZE = 0x0016;
	static const int APU_END = 0x4015;

	static const int APU_STATUS = 0x4015;
	static const int APU_FRAME_COUNT = 0x4017;

	static const int JOY1 = 0x4016;
	static const int JOY2 = 0x4017;
	/*
	memory from 0x4018-0x401f is normally disabled
	*/

	static const int CARTRIDGE_START = 0x4020;
	static const int CARTRIDGE_END = 0xffff;

	static const char* address_mode_names[CPU::NUM_ADDRESS_MODES];
	static const char* instruction_names[CPU::NUM_INSTRUCTIONS];

	typedef void (CPU::*InstructionFunction)(CPU::AddressMode);

	Controller* controllers[2];

	enum ByteInterpretation {
		RAW,
		OPCODE,
		ASCII,
		NUM_BYTE_INTERPRETATIONS
	};

	int wait_cycles = 0;

	PPU* ppu;
	APU* apu;
	Cartridge* cartridge;

	Byte ram[RAM_SIZE];

	Byte accumulator;
	Byte x_register;
	Byte y_register;
	Word program_counter;
	Byte stack_pointer;
	Byte status;

	bool odd_cycle;

	bool _halt;
	bool _nmi;
	bool _irq;

	static const int NMI_VECTOR = 0xfffa;
	static const int RESET_VECTOR = 0xfffc;
	static const int IRQ_VECTOR = 0xfffe;

	static const Byte STACK_START = 0xfd;
	static const int STACK_OFFSET = 0x0100;

	static const Byte STATUS_START = 0x34;

	struct Operation {
		Instruction instruction;
		AddressMode address_mode;
		short clock_cycles;
	};

	Operation operations[256];
	InstructionFunction instruction_functions[NUM_INSTRUCTIONS];



	

	int subroutine_depth = 0;

	struct Breakpoint {
		BreakpointCondition condition;
		Word address;
	};
	std::vector<Breakpoint> breakpoints;

	void debugReadOperation(Word address, Byte value);
	void debugWriteOperation(Word address, Byte value);
	void debugProgramPosition(Word address);
	void debugInterrupt(BreakpointCondition condition);






	Byte readByte(Word address);
	Word readWord(Word address);
	Word readWordBug(Word address);
	void writeByte(Word address, Byte value);

	Word getAddress(CPU::AddressMode address_mode);

	template <class Boolean>
	void setStatusFlag(int bit, Boolean value = Constant<bool, true>());
	bool getStatusFlag(int bit);

	void conditionalBranch(bool branch);
	void compareWithValue(CPU::AddressMode address_mode, Byte value);
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

	void addWithCarry(CPU::AddressMode address_mode);
	void bitwiseAnd(CPU::AddressMode address_mode);
	void shiftLeft(CPU::AddressMode address_mode);
	void branchOnCarryClear(CPU::AddressMode address_mode);
	void branchOnCarrySet(CPU::AddressMode address_mode);
	void branchOnZero(CPU::AddressMode address_mode);
	void testBits(CPU::AddressMode address_mode);
	void branchOnNegative(CPU::AddressMode address_mode);
	void branchOnNotZero(CPU::AddressMode address_mode);
	void branchOnPositive(CPU::AddressMode address_mode);
	void forceBreak(CPU::AddressMode address_mode);
	void branchOnOverflowClear(CPU::AddressMode address_mode);
	void branchOnOverflowSet(CPU::AddressMode address_mode);
	void clearCarryFlag(CPU::AddressMode address_mode);
	void clearDecimalFlag(CPU::AddressMode address_mode);
	void clearInterruptDisableFlag(CPU::AddressMode address_mode);
	void clearOverflowFlag(CPU::AddressMode address_mode);
	void compareWithAcc(CPU::AddressMode address_mode);
	void compareWithX(CPU::AddressMode address_mode);
	void compareWithY(CPU::AddressMode address_mode);
	void decrement(CPU::AddressMode address_mode);
	void decrementX(CPU::AddressMode address_mode);
	void decrementY(CPU::AddressMode address_mode);
	void exclusiveOr(CPU::AddressMode address_mode);
	void increment(CPU::AddressMode address_mode);
	void incrementX(CPU::AddressMode address_mode);
	void incrementY(CPU::AddressMode address_mode);
	void jump(CPU::AddressMode address_mode);
	void jumpToSubroutine(CPU::AddressMode address_mode);
	void loadAcc(CPU::AddressMode address_mode);
	void loadX(CPU::AddressMode address_mode);
	void loadY(CPU::AddressMode address_mode);
	void shiftRight(CPU::AddressMode address_mode);
	void noOperation(CPU::AddressMode address_mode);
	void bitwiseOr(CPU::AddressMode address_mode);
	void pushAcc(CPU::AddressMode address_mode);
	void pushStatus(CPU::AddressMode address_mode);
	void popAcc(CPU::AddressMode address_mode);
	void popStatus(CPU::AddressMode address_mode);
	void rotateLeft(CPU::AddressMode address_mode);
	void rotateRight(CPU::AddressMode address_mode);
	void returnFromInterrupt(CPU::AddressMode address_mode);
	void returnFromSubroutine(CPU::AddressMode address_mode);
	void subtractFromAcc(CPU::AddressMode address_mode);
	void setCarryFlag(CPU::AddressMode address_mode);
	void setDecimalFlag(CPU::AddressMode address_mode);
	void setInterruptDisableFlag(CPU::AddressMode address_mode);
	void storeAcc(CPU::AddressMode address_mode);
	void storeX(CPU::AddressMode address_mode);
	void storeY(CPU::AddressMode address_mode);
	void transferAccToX(CPU::AddressMode address_mode);
	void transferAccToY(CPU::AddressMode address_mode);
	void transferStackPointerToX(CPU::AddressMode address_mode);
	void transferXToAcc(CPU::AddressMode address_mode);
	void transferXToStackPointer(CPU::AddressMode address_mode);
	void transferYToAcc(CPU::AddressMode address_mode);
	void illegalOpcode(CPU::AddressMode address_mode);
};

#endif