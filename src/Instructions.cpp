#include "Instructions.hpp"

#include "debug.hpp"

#include <iterator>

namespace nes
{

const char* s_instructionNames[] = {
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

const char* getInstructionName( Instruction instr )
{
	auto index = static_cast<size_t>( instr );
	dbAssert( index < std::size( s_instructionNames ) );
	return s_instructionNames[ index ];
}

} // namespace nes