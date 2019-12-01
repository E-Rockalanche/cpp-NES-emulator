#ifndef ADRESS_MODE_HPP
#define ADRESS_MODE_HPP

namespace nes
{

enum class AddressMode
{
	Immediate,
	/*
	operand's value given in instruction (next byte)
	ex) LDA #$0a
	*/

	Absolute,
	/*
	operands address is given
	ex) LDA $31f6
	*/

	ZeroPage,
	/*
	only low byte required
	ex) LDA $f4
	*/

	ZeroPageX,
	/*
	address given added to value in x index
	ex) LDA $20, x
	*/

	ZeroPageY,
	/*
	address given added to value in y index
	ex) LDA $20, y
	*/

	// Implied,
	/*
	no operand addresses are required
	ex) TAX
	*/

	Accumulator,
	/*
	instruction operates on data in accumulator
	ex) LSR
	*/

	AbsoluteX,
	AbsoluteXStore, // always an extra cycle regardless of page cross
	/*
	address given added to value in x index
	ex) LDA $31f6, x
	*/

	AbsoluteY,
	AbsoluteYStore, // always an extra cycle regardless of page cross
	/*
	address given added to value in y index
	ex) LDA $31f6, y
	*/

	Indirect,
	/*
	used with jump instruction
	operand is address of value where new address is stored
	ex) JMP ($215f)
	[$215f] = $76
	[$2160] = $30
	*/

	IndirectX,
	/*
	zero-page address is added to contents of x register to give the address
	of the bytes holding to address of the operand
	ex) LDA ($3e, x)
	[x-register] = $05
	[$0043] = $15
	[$0044] = $24
	[$2415] = $6e
	*/

	IndirectY,
	IndirectYStore, // always an extra cycle regardless of page cross
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

	// Relative,
	/*
	only used with branch on condition instructions. A 1 byte value is added to the
	program counter. The 1 byte is treated as a signed number
	ex) BEQ $a7
	*/
};

} // namespace nes

#endif