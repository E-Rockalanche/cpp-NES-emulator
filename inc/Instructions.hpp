#ifndef CPU_INSTRUCTIONS
#define CPU_INSTRUCTIONS

namespace nes
{

enum class Instruction
{
	addWithCarry = 0,
	bitwiseAnd,
	shiftLeft,

	branchOnCarryClear,
	branchOnCarrySet,
	branchOnZero,
	testBits,
	branchOnNegative,
	branchOnNotZero,
	branchOnPositive,
	forceBreak,
	branchOnOverflowClear,
	branchOnOverflowSet,

	clearCarryFlag,
	clearDecimalFlag,
	clearInterruptDisableFlag,
	clearOverflowFlag,
	compareWithAcc,
	compareWithX,
	compareWithY,

	decrement,
	decrementX,
	decrementY,

	exclusiveOr,

	increment,
	incrementX,
	incrementY,

	jump,
	jumpToSubroutine,

	loadAcc,
	loadX,
	loadY,
	shiftRight,

	noOperation,

	bitwiseOr,

	pushAcc,
	pushStatus,
	popAcc,
	popStatus,

	rotateLeft,
	rotateRight,
	returnFromInterrupt,
	returnFromSubroutine,

	subtractFromAcc,
	setCarryFlag,
	setDecimalFlag,
	setInterruptDisableFlag,
	storeAcc,
	storeX,
	storeY,

	transferAccToX,
	transferAccToY,
	transferStackPointerToX,
	transferXToAcc,
	transferXToStackPointer,
	transferYToAcc,
	
	illegalOpcode
};

const char* getInstructionName( Instruction instr );

}

#endif