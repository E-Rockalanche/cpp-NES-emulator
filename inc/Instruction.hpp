#ifndef INSTRUCTION_HPP
#define INSTRUCTION_HPP

namespace nes
{

enum Instruction
{
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

} // namespace nes

#endif