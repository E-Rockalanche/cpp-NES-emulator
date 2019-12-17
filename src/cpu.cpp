#include "Cpu.hpp"

#include "apu.hpp"
#include "Cartridge.hpp"
#include "controller.hpp"
#include "Instructions.hpp"
#include "ppu.hpp"

#include "common.hpp"

// #include "History.hpp"

using namespace nes;

namespace
{
	constexpr size_t RAM_START = 0x0000;
	constexpr size_t RAM_SIZE = 0x0800;
	constexpr size_t RAM_END = 0x1fff;
	/*
	memory from 0x0800-0x1fff mirrors internal ram memory
	*/


	constexpr size_t PPU_START = 0x2000;
	constexpr size_t PPU_SIZE = 0x0008;
	constexpr size_t PPU_END = 0x3fff;
	/*
	memory from 0x2008-0x3fff mirrors ppu registers
	*/

	constexpr size_t OAM_DMA = 0x4014;

	constexpr size_t APU_START = 0x4000;
	constexpr size_t APU_SIZE = 0x0016;
	constexpr size_t APU_END = 0x4015;

	constexpr size_t APU_STATUS = 0x4015;
	constexpr size_t APU_FRAME_COUNT = 0x4017;

	constexpr size_t JOY1 = 0x4016;
	constexpr size_t JOY2 = 0x4017;
	/*
	memory from 0x4018-0x401f is normally disabled
	*/

	constexpr size_t CARTRIDGE_START = 0x4020;
	constexpr size_t CARTRIDGE_END = 0xffff;

	constexpr size_t NMI_VECTOR = 0xfffa;
	constexpr size_t RESET_VECTOR = 0xfffc;
	constexpr size_t IRQ_VECTOR = 0xfffe;

	constexpr Byte STACK_START = 0xfd;
	constexpr Byte STATUS_START = 0x34;

	inline bool crossedPage( Word addr1, Word addr2 )
	{
		return ( addr1 ^ addr2 ) & 0x0100;
	}

	inline bool isNegative( Byte value )
	{
		return static_cast<int8_t>( value ) < 0;
	}

	template<typename T>
	inline bool didCarry( T result )
	{
		static_assert( sizeof( T ) > 1 );
		return result & 0x100;
	}

	using CpuInstruction = void( Cpu::* )( void );

	struct Operation
	{
		CpuInstruction func = nullptr;
		Instruction instr = Instruction::illegalOpcode;
		const char* instructionName = nullptr;
		const char* addressModeName = nullptr;
	};

	Operation s_cpuOperations[ 0x100 ];
}

enum class Cpu::AddressMode
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

void Cpu::power()
{
	m_cycles = 0;

	m_stackPointer = STACK_START;
	m_status = STATUS_START;

	m_accumulator = 0;
	m_xRegister = 0;
	m_yRegister = 0;

	write( APU_STATUS, 0 );
	write( APU_FRAME_COUNT, 0 );
	for ( Word i = 0; i < 16; ++i )
		write( APU_START + i, 0 );

	m_programCounter = readWordTick( RESET_VECTOR );
	dbLogWord( "PC", m_programCounter );

	m_halt = false;
	m_nmi = -1;
	m_irq = -1;
	m_oddCycle = false;
}

void Cpu::reset()
{
	m_cycles = 0;

	m_stackPointer -= 3;
	setStatus( DisableInterrupts );
	write( APU_STATUS, 0 );

	m_programCounter = readWordTick( RESET_VECTOR );
	dbLogWord( "PC", m_programCounter );

	m_halt = false;
	m_nmi = -1;
	m_irq = -1;
	m_oddCycle = false;
}

void Cpu::executeInstruction()
{
	if ( ! halted() )
	{
		if ( testStatus( DisableInterrupts ) )
			m_irq = -1;

		if ( m_nmi >= 2 )
		{
			m_nmi = -1;
			nmi();
			return;
		}

		if ( m_irq >= 2 )
		{
			m_irq = -1;
			irq();
			return;
		}

		Byte opcode = readByteTick( m_programCounter++ );
		auto operation = s_cpuOperations[ opcode ];
		( this->*operation.func )();
	}
}

void Cpu::runFrame()
{
	m_cycles = 0;
	while ( !halted() && !m_ppu->readyToDraw() )
	{
		executeInstruction();
	}

	m_apu->runFrame( m_cycles );
}

void Cpu::tick()
{
	m_oddCycle = !m_oddCycle;
	for ( int i = 0; i < 3; ++i )
	{
		m_nmi += ( m_nmi >= 0 );
		m_irq += ( m_irq >= 0 );
		m_ppu->tick();
	}
	++m_cycles;
}

void Cpu::setArithmeticFlags( Byte value )
{
	setStatus( Negative, isNegative( value ) );
	setStatus( Zero, value == 0 );
}

void Cpu::addToAccumulator( Byte value )
{
	unsigned int result = m_accumulator + value + testStatus( Carry );
	bool carry = didCarry( result );
	checkOverflow( m_accumulator, value, result );
	setStatus( Carry, carry );
	m_accumulator = static_cast<Byte>( result );
	setArithmeticFlags( m_accumulator );
}

void Cpu::oamDmaTransfer( Byte addressHigh )
{
	tick();
	if ( m_oddCycle )
		tick();

	Word address = addressHigh << 8;
	Word end = address + 256;
	for ( ; address != end; ++address )
	{
		Byte data = readByteTick( address );
		tick();
		m_ppu->writeToOAM( data );
	}
}

Byte Cpu::read( Word address )
{
	switch ( address )
	{
		case RAM_START ... RAM_END:
			return m_ram[ address - RAM_START ];

		case PPU_START ... PPU_END:
			return m_ppu->readRegister( ( address - PPU_START ) % PPU_SIZE );

		case APU_START ... APU_END:
		{
			if ( address == OAM_DMA )
				return 0;

			return m_apu->read( m_cycles, address );
		}

		case JOY1 ... JOY2:
		{
			auto* controller = m_controllerPorts[ address - JOY1 ];
			return controller ? controller->read() : 0;
		}

		case CARTRIDGE_START ... CARTRIDGE_END:
			return m_cartridge->readPRG( address );

		default:
			return 0;
	}
}

void Cpu::write( Word address, Byte value )
{
	switch ( address )
	{
		case RAM_START ... RAM_END:
			m_ram[ address - RAM_START ] = value;
			break;

		case PPU_START ... PPU_END:
			m_ppu->writeRegister( ( address - PPU_START ) % PPU_SIZE, value );
			break;

		case APU_START ... APU_END:
		case JOY2:
		{
			if ( address == OAM_DMA )
				oamDmaTransfer( value );
			else
				m_apu->write( m_cycles, address, value );
			break;
		}

		case JOY1:
		{
			for ( auto* controller : m_controllerPorts )
				controller->write( value );
			break;
		}

		case CARTRIDGE_START ... CARTRIDGE_END:
			m_cartridge->writePRG( address, value );
			break;
	}
}

void Cpu::nmi()
{
	tick();
	tick();
	pushWord( m_programCounter );
	pushByte( m_status );
	setStatus( DisableInterrupts );
	m_programCounter = readWordTick( NMI_VECTOR );
}

void Cpu::irq()
{
	tick();
	tick();
	setStatus( Break );
	pushWord( m_programCounter );
	pushByte( m_status );
	setStatus( DisableInterrupts );
	m_programCounter = readWordTick( IRQ_VECTOR );
}

void Cpu::conditionalBranch( bool branch )
{
	int8_t offset = static_cast<int8_t>( readByteTick( m_programCounter++ ) );
	if ( branch )
	{
		tick();
		Word page1 = m_programCounter;
		m_programCounter += offset;

		if ( crossedPage( page1, m_programCounter ) )
		{
			tick();
		}
	}
}

template <Cpu::AddressMode Mode>
Word Cpu::getAddress()
{
	switch ( Mode )
	{
		case AddressMode::Immediate:
			return m_programCounter++;

		case AddressMode::Absolute:
		{
			Word address = readWordTick( m_programCounter );
			m_programCounter += 2;
			return address;
		}

		case AddressMode::ZeroPage:
			return readByteTick( m_programCounter++ );

		case AddressMode::ZeroPageX:
		{
			Word address = ( readByteTick( m_programCounter++ ) + m_xRegister ) & 0xff;
			tick();
			return address;
		}

		case AddressMode::ZeroPageY:
		{
			Word address = ( readByteTick( m_programCounter++ ) + m_yRegister ) & 0xff;
			tick();
			return address;
		}

		case AddressMode::AbsoluteX:
		{
			Word page1 = readWordTick( m_programCounter );
			auto address = page1 + m_xRegister;
			if ( crossedPage( page1, address ) )
				dummyRead( address - 0x100 );
			m_programCounter += 2;
			return address;
		}

		case AddressMode::AbsoluteXStore:
		{
			Word page1 = readWordTick( m_programCounter );
			m_programCounter += 2;
			Word address = page1 + m_xRegister;
			dummyRead( address - ( crossedPage( page1, address ) ? 0x100 : 0 ) );
			return address;
		}

		case AddressMode::AbsoluteY:
		{
			Word page1 = readWordTick( m_programCounter );
			Word address = page1 + m_yRegister;
			if ( crossedPage( page1, address ) )
				dummyRead( address - 0x100 );
			m_programCounter += 2;
			return address;
		}

		case AddressMode::AbsoluteYStore:
		{
			Word page1 = readWordTick( m_programCounter );
			m_programCounter += 2;
			Word address = page1 + m_yRegister;
			dummyRead( address - ( crossedPage( page1, address ) ? 0x100 : 0 ) );
			return address;
		}

		case AddressMode::Indirect:
		{
			Word address = readWordTickBug( readWordTick( m_programCounter ) );
			m_programCounter += 2;
			return address;
		}

		case AddressMode::IndirectX:
		{
			Word zp = ( readByteTick( m_programCounter++ ) + m_xRegister ) & 0xff;
			tick();
			return readWordTickBug( zp );
		}

		case AddressMode::IndirectY:
		{
			Word zp = readByteTick( m_programCounter++ );
			Word address = readWordTickBug( zp ) + m_yRegister;
			if ( crossedPage( address - m_yRegister, address ) )
				dummyRead( address - 0x100 );
			return address;
		}

		case AddressMode::IndirectYStore:
		{
			Word page1 = readWordTickBug( readByteTick( m_programCounter++ ) );
			Word address = page1 + m_yRegister;
			dummyRead( address - ( crossedPage( page1, address ) ? 0x100 : 0 ) );
			return address;
		}
	}
}

void Cpu::buggyIndexWrite( Byte index, Byte value )
{
	Word page1 = getAddress<AddressMode::Absolute>();
	Word address = page1 + index;
	dummyRead( ( page1 & 0xff00 ) | ( address & 0x00ff ) );
	if ( crossedPage( page1, address ) )
	{
		address &= ( value << 8 );
	}
	writeByteTick( address, value & ( ( page1 >> 8 ) + 1 ) );
}

////////////////////////////////////////////////////////////////////////////////
//                                 OPERATIONS                                 //
////////////////////////////////////////////////////////////////////////////////

// ADC
template <Cpu::AddressMode Mode>
void Cpu::addWithCarry()
{
	addToAccumulator( readByteTick( getAddress<Mode>() ) );
}

// AND
template <Cpu::AddressMode Mode>
void Cpu::bitwiseAnd()
{
	Byte value = readByteTick( getAddress<Mode>() );
	m_accumulator &= value;
	setArithmeticFlags( m_accumulator );
}

// ASL
template <Cpu::AddressMode Mode>
void Cpu::shiftLeft()
{
	int result;
	if constexpr ( Mode == AddressMode::Accumulator )
	{
		dummyRead();
		result = m_accumulator << 1;
		m_accumulator = result;
	}
	else
	{
		Word address = getAddress<Mode>();
		result = readByteTick( address ) << 1;
		tick();
		writeByteTick( address, result );
	}
	setArithmeticFlags( result );
	setStatus( Carry, didCarry( result ) );
}

// BCC
void Cpu::branchOnCarryClear()
{
	conditionalBranch( !testStatus( Carry ) );
}

// BCS
void Cpu::branchOnCarrySet()
{
	conditionalBranch( testStatus( Carry ) );
}

// BEQ
void Cpu::branchOnZero()
{
	conditionalBranch( testStatus( Zero ) );
}

// BIT
template <Cpu::AddressMode Mode>
void Cpu::testBits()
{
	Byte value = readByteTick( getAddress<Mode>() );
	setStatus( Negative, value & Negative );
	setStatus( Overflow, value & Overflow );
	setStatus( Zero, ( value & m_accumulator ) == 0 );
}

// BMI
void Cpu::branchOnNegative()
{
	conditionalBranch( testStatus( Negative ) );
}

// BNE
void Cpu::branchOnNotZero()
{
	conditionalBranch( !testStatus( Zero ) );
}

// BPL
void Cpu::branchOnPositive()
{
	conditionalBranch( !testStatus( Negative ) );
}

// BRK
void Cpu::forceBreak()
{
	dummyRead();
	setStatus( Break );
	pushWord( m_programCounter + 1 );
	pushByte( m_status );
	setStatus( DisableInterrupts );
	m_programCounter = readWordTick( IRQ_VECTOR );
}

// BVC
void Cpu::branchOnOverflowClear()
{
	conditionalBranch( !testStatus( Overflow ) );
}

// BVS
void Cpu::branchOnOverflowSet()
{
	conditionalBranch( testStatus( Overflow ) );
}

// CLC
void Cpu::clearCarryFlag()
{
	clearStatus( Carry );
	tick();
}

// CLD
void Cpu::clearDecimalFlag()
{
	clearStatus( DecimalMode );
	tick();
}

// CLI
void Cpu::clearInterruptDisableFlag()
{
	clearStatus( DisableInterrupts );
	tick();
}

// CLV
void Cpu::clearOverflowFlag()
{
	clearStatus( Overflow );
	tick();
}

// CMP
template <Cpu::AddressMode Mode>
void Cpu::compareWithAcc()
{
	compareWithValue<Mode>( m_accumulator );
}

// CPX
template <Cpu::AddressMode Mode>
void Cpu::compareWithX()
{
	compareWithValue<Mode>( m_xRegister );
}

// CPY
template <Cpu::AddressMode Mode>
void Cpu::compareWithY()
{
	compareWithValue<Mode>( m_yRegister );
}

// DEC
template <Cpu::AddressMode Mode>
void Cpu::decrement()
{
	Word address = getAddress<Mode>();
	Byte result = readByteTick( address ) - 1;
	tick();
	setArithmeticFlags( result );
	writeByteTick( address, result );
}

// DEX
void Cpu::decrementX()
{
	setArithmeticFlags( --m_xRegister );
	tick();
}

// DEY
void Cpu::decrementY()
{
	setArithmeticFlags( --m_yRegister );
	tick();
}

// EOR
template <Cpu::AddressMode Mode>
void Cpu::exclusiveOr()
{
	m_accumulator ^= readByteTick( getAddress<Mode>() );
	setArithmeticFlags( m_accumulator );
}

// INC
template <Cpu::AddressMode Mode>
void Cpu::increment()
{
	Word address = getAddress<Mode>();
	Byte result = readByteTick( address ) + 1;
	tick();
	setArithmeticFlags( result );
	writeByteTick( address, result );
}

// INX
void Cpu::incrementX()
{
	setArithmeticFlags( ++m_xRegister );
	tick();
}

// INY
void Cpu::incrementY()
{
	setArithmeticFlags( ++m_yRegister );
	tick();
}

// JMP
template <Cpu::AddressMode Mode>
void Cpu::jump()
{
	m_programCounter = getAddress<Mode>();
}

// JSR
template <Cpu::AddressMode Mode>
void Cpu::jumpToSubroutine()
{
	tick();
	pushWord( m_programCounter + 1 );
	m_programCounter = readWordTick( m_programCounter );
}

// LDA
template <Cpu::AddressMode Mode>
void Cpu::loadAcc()
{
	m_accumulator = readByteTick( getAddress<Mode>() );
	setArithmeticFlags( m_accumulator );
}

// LDX
template <Cpu::AddressMode Mode>
void Cpu::loadX()
{
	m_xRegister = readByteTick( getAddress<Mode>() );
	setArithmeticFlags( m_xRegister );
}

// LDY
template <Cpu::AddressMode Mode>
void Cpu::loadY()
{
	m_yRegister = readByteTick( getAddress<Mode>() );
	setArithmeticFlags( m_yRegister );
}

// LSR
template <Cpu::AddressMode Mode>
void Cpu::shiftRight()
{
	Byte result;
	bool carry;

	if constexpr ( Mode == AddressMode::Accumulator )
	{
		dummyRead();
		carry = m_accumulator & 1;
		m_accumulator >>= 1;
		result = m_accumulator;
	}
	else
	{
		Word address = getAddress<Mode>();
		Byte value = readByteTick( address );
		carry = value & 1;
		result = value >> 1;
		tick();
		writeByteTick( address, result );
	}

	setStatus( Carry, carry );
	setArithmeticFlags( result );
}

// NOP
void Cpu::noOperation()
{
	tick();
}

// ORA
template <Cpu::AddressMode Mode>
void Cpu::bitwiseOr()
{
	m_accumulator |= readByteTick( getAddress<Mode>() );
	setArithmeticFlags( m_accumulator );
}

// PHA
void Cpu::pushAcc()
{
	tick();
	pushByte( m_accumulator );
}

// PHP
void Cpu::pushStatus()
{
	tick();
	pushByte( m_status | 0x30 );
}

// PLA
void Cpu::popAcc()
{
	dummyRead();
	tick();
	m_accumulator = popByte();
	setArithmeticFlags( m_accumulator );
}

// PLP
void Cpu::popStatus()
{
	dummyRead();
	tick();
	m_status = ( popByte() & ~0x30 ) | ( m_status & 0x30 );
}

// ROL
template <Cpu::AddressMode Mode>
void Cpu::rotateLeft()
{
	bool carry;
	Byte result;

	if constexpr ( Mode == AddressMode::Accumulator )
	{
		dummyRead();
		carry = m_accumulator & 0x80;
		m_accumulator = ( m_accumulator << 1 ) | testStatus( Carry );
		result = m_accumulator;
	}
	else
	{
		Word address = getAddress<Mode>();
		result = readByteTick( address );
		carry = result & 0x80;
		result = ( result << 1 ) | testStatus( Carry );
		tick();
		writeByteTick( address, result );
	}

	setStatus( Carry, carry );
	setArithmeticFlags( result );
}

// ROR
template <Cpu::AddressMode Mode>
void Cpu::rotateRight()
{
	bool carry;
	Byte result;

	if constexpr ( Mode == AddressMode::Accumulator )
	{
		dummyRead();
		carry = m_accumulator & 1;
		m_accumulator = ( m_accumulator >> 1 ) | ( testStatus( Carry ) ? 0x80 : 0 );
		result = m_accumulator;
	}
	else
	{
		Word address = getAddress<Mode>();
		result = readByteTick( address );
		carry = result & 1;
		result = ( result >> 1 ) | ( testStatus( Carry ) ? 0x80 : 0 );
		tick();
		writeByteTick( address, result );
	}
	setStatus( Carry, carry );
	setArithmeticFlags( result );
}

// RTI
void Cpu::returnFromInterrupt()
{
	dummyRead();
	m_status = ( popByte() & ~0x30 ) | ( m_status & 0x30 );
	m_programCounter = popWord();
	tick();
}

// RTS
void Cpu::returnFromSubroutine()
{
	readByteTick( m_programCounter );
	tick();
	m_programCounter = popWord() + 1;
	tick();
}

// SBS
template <Cpu::AddressMode Mode>
void Cpu::subtractFromAcc()
{
	addToAccumulator( ~readByteTick( getAddress<Mode>() ) );
}

// SEC
void Cpu::setCarryFlag()
{
	setStatus( Carry );
	tick();
}

// SED
void Cpu::setDecimalFlag()
{
	setStatus( DecimalMode );
	tick();
}

// SEI
void Cpu::setInterruptDisableFlag()
{
	setStatus( DisableInterrupts );
	tick();
}

// STA
template <Cpu::AddressMode Mode>
void Cpu::storeAcc()
{
	writeByteTick( getAddress<Mode>(), m_accumulator );
}

// STX
template <Cpu::AddressMode Mode>
void Cpu::storeX()
{
	writeByteTick( getAddress<Mode>(), m_xRegister );
}

// STY
template <Cpu::AddressMode Mode>
void Cpu::storeY()
{
	writeByteTick( getAddress<Mode>(), m_yRegister );
}

// TAX
void Cpu::transferAccToX()
{
	m_xRegister = m_accumulator;
	setArithmeticFlags( m_xRegister );
	tick();
}

// TAY
void Cpu::transferAccToY()
{
	m_yRegister = m_accumulator;
	setArithmeticFlags( m_yRegister );
	tick();
}

// TSX
void Cpu::transferStackPointerToX()
{
	m_xRegister = m_stackPointer;
	setArithmeticFlags( m_xRegister );
	tick();
}

// TXA
void Cpu::transferXToAcc()
{
	m_accumulator = m_xRegister;
	setArithmeticFlags( m_accumulator );
	tick();
}

// TXS
void Cpu::transferXToStackPointer()
{
	m_stackPointer = m_xRegister;
	tick();
}

// TYA
void Cpu::transferYToAcc()
{
	m_accumulator = m_yRegister;
	setArithmeticFlags( m_accumulator );
	tick();
}

// ILL
void Cpu::illegalOpcode()
{
	halt();
}

// unofficial opcodes

// ALR
template <Cpu::AddressMode Mode>
void Cpu::andShiftRight()
{
	m_accumulator &= readByteTick( getAddress<Mode>() );
	setStatus( Carry, m_accumulator & 1 );
	m_accumulator >>= 1;
	setArithmeticFlags( m_accumulator );
}

// ANC
template <Cpu::AddressMode Mode>
void Cpu::andSetCarry()
{
	m_accumulator &= readByteTick( getAddress<Mode>() );
	setArithmeticFlags( m_accumulator );
	setStatus( Carry, m_accumulator & 0x80 );
}

// ARR
template <Cpu::AddressMode Mode>
void Cpu::andRotateRight()
{
	m_accumulator &= readByteTick( getAddress<Mode>() );
	m_accumulator = ( m_accumulator >> 1 ) | ( testStatus( Carry ) ? 0x80 : 0 );
	setArithmeticFlags( m_accumulator );
	setStatus( Carry, 0x40 & m_accumulator );
	setStatus( Overflow, (bool)( 0x40 & m_accumulator) ^ (bool)( 0x20 & m_accumulator ) );
}

// AXS
template <Cpu::AddressMode Mode>
void Cpu::subtractFromAccAndX()
{
	Byte value = readByteTick( getAddress<Mode>() );
	int result = ( m_accumulator & m_xRegister ) - value;
	setStatus( Carry, ( m_accumulator & m_xRegister ) >= value );
	m_xRegister = result & 0xff;
	setArithmeticFlags( m_xRegister );
}

// LAX
template <Cpu::AddressMode Mode>
void Cpu::loadAccTransferToX()
{
	m_xRegister = m_accumulator = readByteTick( getAddress<Mode>() );
	setArithmeticFlags( m_accumulator );
}

// SAX, AXA
template <Cpu::AddressMode Mode>
void Cpu::storeAccAndX()
{
	writeByteTick( getAddress<Mode>(), m_accumulator & m_xRegister );
}

// DCP
template <Cpu::AddressMode Mode>
void Cpu::decrementCompare()
{
	Word address = getAddress<Mode>();
	Byte value = readByteTick( address ) - 1;
	writeByteTick( address, value );

	setStatus( Carry, m_accumulator >= value );
	setArithmeticFlags( m_accumulator - value );
	tick();
}

// ISC
template <Cpu::AddressMode Mode>
void Cpu::incrementSubtract()
{
	Word address = getAddress<Mode>();
	Byte value = readByteTick( address ) + 1;
	tick();
	writeByteTick( address, value );

	addToAccumulator( ~value );
}

// RLA
template <Cpu::AddressMode Mode>
void Cpu::rotateLeftAnd()
{
	Word address = getAddress<Mode>();
	Byte value = readByteTick( address );
	tick();
	bool carry = value & 0x80;
	value = ( value << 1 ) | testStatus( Carry );
	setStatus( Carry, carry );
	writeByteTick( address, value );
	m_accumulator &= value;
	setArithmeticFlags( m_accumulator );
}

// RRA
template <Cpu::AddressMode Mode>
void Cpu::rotateRightAdd()
{
	Word address = getAddress<Mode>();
	Byte value = readByteTick( address );
	tick();
	bool carry = value & 1;
	value = ( value >> 1 ) | ( testStatus( Carry ) ? 0x80 : 0 );
	setStatus( Carry, carry );

	writeByteTick( address, value );
	addToAccumulator( value );
}

// SLO
template <Cpu::AddressMode Mode>
void Cpu::shiftLeftOrAcc()
{
	Word address = getAddress<Mode>();
	Byte value = readByteTick( address );
	tick();
	setStatus( Carry, value & 0x80 );
	value <<= 1;
	writeByteTick( address, value );

	m_accumulator |= value;
	setArithmeticFlags( m_accumulator );
}

// SRE
template <Cpu::AddressMode Mode>
void Cpu::shiftRightExclusiveOr()
{
	Word address = getAddress<Mode>();
	Byte value = readByteTick( address );
	tick();
	setStatus( Carry, value & 1 );
	value >>= 1;
	writeByteTick( address, value );

	m_accumulator ^= value;
	setArithmeticFlags( m_accumulator );
}

// XAA
template <Cpu::AddressMode Mode>
void Cpu::transferXToAccAnd()
{
	m_accumulator = m_xRegister;
	m_accumulator &= readByteTick( getAddress<Mode>() );
}

// OAL
template <Cpu::AddressMode Mode>
void Cpu::orAndAccSetAccX()
{
	m_accumulator |= 0xee;
	m_accumulator &= readByteTick( getAddress<Mode>() );
	m_xRegister = m_accumulator;
}

// SXA
void Cpu::andXAddrHigh()
{
	buggyIndexWrite( m_yRegister, m_xRegister );
}

// SYA
void Cpu::andYAddrHigh()
{
	buggyIndexWrite( m_xRegister, m_yRegister );
}

// XAS
template <Cpu::AddressMode Mode>
void Cpu::andXAccStoreStackPointer()
{
	Word address = getAddress<Mode>();
	m_stackPointer = m_xRegister & m_accumulator;
	writeByteTick( address, m_stackPointer & ( ( address >> 8 ) + 1 ) );
}

//SHA
template <Cpu::AddressMode Mode>
void Cpu::andXAccSeven()
{
	Word address = getAddress<Mode>();
	writeByteTick( address, m_xRegister & m_accumulator & 0x07 );
}

// LAR
template <Cpu::AddressMode Mode>
void Cpu::andSPTransferToAcXSP()
{
	m_accumulator = m_xRegister = m_stackPointer = readByteTick( getAddress<Mode>() ) & m_stackPointer;
	setArithmeticFlags( m_accumulator );
}

// IGN
template <Cpu::AddressMode Mode>
void Cpu::ignoreByte()
{
	Word address = getAddress<Mode>();
	readByteTick( address );
}


#define CHECK_NULL_OR_ILL( opcode )	do {		\
	auto func = s_cpuOperations[ opcode ].func;	\
	dbAssert( func == nullptr || func == &Cpu::illegalOpcode ); } while( false )

#define SET_ADDRMODE_OP( opcode, instr, addrmode ) 	\
	CHECK_NULL_OR_ILL( opcode );					\
	s_cpuOperations[ opcode ] = { &Cpu::instr< Cpu::AddressMode::addrmode >, Instruction::instr, #instr, #addrmode };

#define SET_IMPLIED( opcode, instr )	\
	CHECK_NULL_OR_ILL( opcode );		\
	s_cpuOperations[ opcode ] = { &Cpu::instr, Instruction::instr, #instr, "Implied" };

#define SET_BRANCH( opcode, instr )	\
	CHECK_NULL_OR_ILL( opcode );	\
	s_cpuOperations[ opcode ] = { &Cpu::instr, Instruction::instr, #instr, "Relative" };

void Cpu::initialize()
{
	for ( size_t i = 0; i < 0x100; ++i )
	{
		SET_IMPLIED( i, illegalOpcode )
	}

	SET_ADDRMODE_OP( 0x69, addWithCarry, Immediate )
	SET_ADDRMODE_OP( 0x65, addWithCarry, ZeroPage )
	SET_ADDRMODE_OP( 0x75, addWithCarry, ZeroPageX )
	SET_ADDRMODE_OP( 0x6d, addWithCarry, Absolute )
	SET_ADDRMODE_OP( 0x7d, addWithCarry, AbsoluteX )
	SET_ADDRMODE_OP( 0x79, addWithCarry, AbsoluteY )
	SET_ADDRMODE_OP( 0x61, addWithCarry, IndirectX )
	SET_ADDRMODE_OP( 0x71, addWithCarry, IndirectY )

	SET_ADDRMODE_OP( 0x29, bitwiseAnd, Immediate )
	SET_ADDRMODE_OP( 0x25, bitwiseAnd, ZeroPage )
	SET_ADDRMODE_OP( 0x35, bitwiseAnd, ZeroPageX )
	SET_ADDRMODE_OP( 0x2d, bitwiseAnd, Absolute )
	SET_ADDRMODE_OP( 0x3d, bitwiseAnd, AbsoluteX )
	SET_ADDRMODE_OP( 0x39, bitwiseAnd, AbsoluteY )
	SET_ADDRMODE_OP( 0x21, bitwiseAnd, IndirectX )
	SET_ADDRMODE_OP( 0x31, bitwiseAnd, IndirectY )

	SET_ADDRMODE_OP( 0x0a, shiftLeft, Accumulator )
	SET_ADDRMODE_OP( 0x06, shiftLeft, ZeroPage )
	SET_ADDRMODE_OP( 0x16, shiftLeft, ZeroPageX )
	SET_ADDRMODE_OP( 0x0e, shiftLeft, Absolute )
	SET_ADDRMODE_OP( 0x1e, shiftLeft, AbsoluteXStore )

	SET_BRANCH( 0x90, branchOnCarryClear )
	SET_BRANCH( 0xb0, branchOnCarrySet )
	SET_BRANCH( 0xf0, branchOnZero )

	SET_ADDRMODE_OP( 0x24, testBits, ZeroPage )
	SET_ADDRMODE_OP( 0x2c, testBits, Absolute )

	SET_BRANCH( 0x30, branchOnNegative )
	SET_BRANCH( 0xd0, branchOnNotZero )
	SET_BRANCH( 0x10, branchOnPositive )

	SET_IMPLIED( 0x00, forceBreak )

	SET_BRANCH( 0x50, branchOnOverflowClear )
	SET_BRANCH( 0x70, branchOnOverflowSet )

	SET_IMPLIED( 0x18, clearCarryFlag )
	SET_IMPLIED( 0xd8, clearDecimalFlag )
	SET_IMPLIED( 0x58, clearInterruptDisableFlag )
	SET_IMPLIED( 0xb8, clearOverflowFlag )

	SET_ADDRMODE_OP( 0xc9, compareWithAcc, Immediate )
	SET_ADDRMODE_OP( 0xc5, compareWithAcc, ZeroPage )
	SET_ADDRMODE_OP( 0xd5, compareWithAcc, ZeroPageX )
	SET_ADDRMODE_OP( 0xcd, compareWithAcc, Absolute )
	SET_ADDRMODE_OP( 0xdd, compareWithAcc, AbsoluteX )
	SET_ADDRMODE_OP( 0xd9, compareWithAcc, AbsoluteY )
	SET_ADDRMODE_OP( 0xc1, compareWithAcc, IndirectX )
	SET_ADDRMODE_OP( 0xd1, compareWithAcc, IndirectY )

	SET_ADDRMODE_OP( 0xe0, compareWithX, Immediate )
	SET_ADDRMODE_OP( 0xe4, compareWithX, ZeroPage )
	SET_ADDRMODE_OP( 0xec, compareWithX, Absolute )

	SET_ADDRMODE_OP( 0xc0, compareWithY, Immediate )
	SET_ADDRMODE_OP( 0xc4, compareWithY, ZeroPage )
	SET_ADDRMODE_OP( 0xcc, compareWithY, Absolute )

	SET_ADDRMODE_OP( 0xc6, decrement, ZeroPage )
	SET_ADDRMODE_OP( 0xd6, decrement, ZeroPageX )
	SET_ADDRMODE_OP( 0xce, decrement, Absolute )
	SET_ADDRMODE_OP( 0xde, decrement, AbsoluteXStore )

	SET_IMPLIED( 0xca, decrementX )
	SET_IMPLIED( 0x88, decrementY )

	SET_ADDRMODE_OP( 0x49, exclusiveOr, Immediate )
	SET_ADDRMODE_OP( 0x45, exclusiveOr, ZeroPage )
	SET_ADDRMODE_OP( 0x55, exclusiveOr, ZeroPageX )
	SET_ADDRMODE_OP( 0x4d, exclusiveOr, Absolute )
	SET_ADDRMODE_OP( 0x5d, exclusiveOr, AbsoluteX )
	SET_ADDRMODE_OP( 0x59, exclusiveOr, AbsoluteY )
	SET_ADDRMODE_OP( 0x41, exclusiveOr, IndirectX )
	SET_ADDRMODE_OP( 0x51, exclusiveOr, IndirectY )

	SET_ADDRMODE_OP( 0xe6, increment, ZeroPage )
	SET_ADDRMODE_OP( 0xf6, increment, ZeroPageX )
	SET_ADDRMODE_OP( 0xee, increment, Absolute )
	SET_ADDRMODE_OP( 0xfe, increment, AbsoluteXStore )

	SET_IMPLIED( 0xe8, incrementX )
	SET_IMPLIED( 0xc8, incrementY )

	SET_ADDRMODE_OP( 0x4c, jump, Absolute )
	SET_ADDRMODE_OP( 0x6c, jump, Indirect )

	SET_ADDRMODE_OP( 0x20, jumpToSubroutine, Absolute )

	SET_ADDRMODE_OP( 0xa9, loadAcc, Immediate )
	SET_ADDRMODE_OP( 0xa5, loadAcc, ZeroPage )
	SET_ADDRMODE_OP( 0xb5, loadAcc, ZeroPageX )
	SET_ADDRMODE_OP( 0xad, loadAcc, Absolute )
	SET_ADDRMODE_OP( 0xbd, loadAcc, AbsoluteX )
	SET_ADDRMODE_OP( 0xb9, loadAcc, AbsoluteY )
	SET_ADDRMODE_OP( 0xa1, loadAcc, IndirectX )
	SET_ADDRMODE_OP( 0xb1, loadAcc, IndirectY )

	SET_ADDRMODE_OP( 0xa2, loadX, Immediate )
	SET_ADDRMODE_OP( 0xa6, loadX, ZeroPage )
	SET_ADDRMODE_OP( 0xb6, loadX, ZeroPageY )
	SET_ADDRMODE_OP( 0xae, loadX, Absolute )
	SET_ADDRMODE_OP( 0xbe, loadX, AbsoluteY )

	SET_ADDRMODE_OP( 0xa0, loadY, Immediate );
	SET_ADDRMODE_OP( 0xa4, loadY, ZeroPage );
	SET_ADDRMODE_OP( 0xb4, loadY, ZeroPageX );
	SET_ADDRMODE_OP( 0xac, loadY, Absolute );
	SET_ADDRMODE_OP( 0xbc, loadY, AbsoluteX );

	SET_ADDRMODE_OP( 0x4a, shiftRight, Accumulator );
	SET_ADDRMODE_OP( 0x46, shiftRight, ZeroPage );
	SET_ADDRMODE_OP( 0x56, shiftRight, ZeroPageX );
	SET_ADDRMODE_OP( 0x4e, shiftRight, Absolute );
	SET_ADDRMODE_OP( 0x5e, shiftRight, AbsoluteXStore );

	SET_IMPLIED( 0xea, noOperation );

	SET_ADDRMODE_OP( 0x09, bitwiseOr, Immediate );
	SET_ADDRMODE_OP( 0x05, bitwiseOr, ZeroPage );
	SET_ADDRMODE_OP( 0x15, bitwiseOr, ZeroPageX );
	SET_ADDRMODE_OP( 0x0d, bitwiseOr, Absolute );
	SET_ADDRMODE_OP( 0x1d, bitwiseOr, AbsoluteX );
	SET_ADDRMODE_OP( 0x19, bitwiseOr, AbsoluteY );
	SET_ADDRMODE_OP( 0x01, bitwiseOr, IndirectX );
	SET_ADDRMODE_OP( 0x11, bitwiseOr, IndirectY );

	SET_IMPLIED( 0x48, pushAcc );
	SET_IMPLIED( 0x08, pushStatus );
	SET_IMPLIED( 0x68, popAcc );
	SET_IMPLIED( 0x28, popStatus );

	SET_ADDRMODE_OP( 0x2a, rotateLeft, Accumulator );
	SET_ADDRMODE_OP( 0x26, rotateLeft, ZeroPage );
	SET_ADDRMODE_OP( 0x36, rotateLeft, ZeroPageX );
	SET_ADDRMODE_OP( 0x2e, rotateLeft, Absolute );
	SET_ADDRMODE_OP( 0x3e, rotateLeft, AbsoluteXStore );

	SET_ADDRMODE_OP( 0x6a, rotateRight, Accumulator );
	SET_ADDRMODE_OP( 0x66, rotateRight, ZeroPage );
	SET_ADDRMODE_OP( 0x76, rotateRight, ZeroPageX );
	SET_ADDRMODE_OP( 0x6e, rotateRight, Absolute );
	SET_ADDRMODE_OP( 0x7e, rotateRight, AbsoluteXStore );

	SET_IMPLIED( 0x40, returnFromInterrupt );

	SET_IMPLIED( 0x60, returnFromSubroutine );

	SET_ADDRMODE_OP( 0xe9, subtractFromAcc, Immediate );
	SET_ADDRMODE_OP( 0xe5, subtractFromAcc, ZeroPage );
	SET_ADDRMODE_OP( 0xf5, subtractFromAcc, ZeroPageX );
	SET_ADDRMODE_OP( 0xed, subtractFromAcc, Absolute );
	SET_ADDRMODE_OP( 0xfd, subtractFromAcc, AbsoluteX );
	SET_ADDRMODE_OP( 0xf9, subtractFromAcc, AbsoluteY );
	SET_ADDRMODE_OP( 0xe1, subtractFromAcc, IndirectX );
	SET_ADDRMODE_OP( 0xf1, subtractFromAcc, IndirectY );

	SET_IMPLIED( 0x38, setCarryFlag );
	SET_IMPLIED( 0xf8, setDecimalFlag );
	SET_IMPLIED( 0x78, setInterruptDisableFlag );

	SET_ADDRMODE_OP( 0x85, storeAcc, ZeroPage );
	SET_ADDRMODE_OP( 0x95, storeAcc, ZeroPageX );
	SET_ADDRMODE_OP( 0x8d, storeAcc, Absolute );
	SET_ADDRMODE_OP( 0x9d, storeAcc, AbsoluteXStore );
	SET_ADDRMODE_OP( 0x99, storeAcc, AbsoluteYStore );
	SET_ADDRMODE_OP( 0x81, storeAcc, IndirectX );
	SET_ADDRMODE_OP( 0x91, storeAcc, IndirectYStore );

	SET_ADDRMODE_OP( 0x86, storeX, ZeroPage );
	SET_ADDRMODE_OP( 0x96, storeX, ZeroPageY );
	SET_ADDRMODE_OP( 0x8e, storeX, Absolute );

	SET_ADDRMODE_OP( 0x84, storeY, ZeroPage );
	SET_ADDRMODE_OP( 0x94, storeY, ZeroPageX );
	SET_ADDRMODE_OP( 0x8c, storeY, Absolute );

	SET_IMPLIED( 0xaa, transferAccToX );
	SET_IMPLIED( 0xa8, transferAccToY );
	SET_IMPLIED( 0xba, transferStackPointerToX );
	SET_IMPLIED( 0x8a, transferXToAcc );
	SET_IMPLIED( 0x9a, transferXToStackPointer );
	SET_IMPLIED( 0x98, transferYToAcc );

	// unofficial:

	SET_ADDRMODE_OP( 0x4b, andShiftRight, Immediate );

	SET_ADDRMODE_OP( 0x0b, andSetCarry, Immediate );
	SET_ADDRMODE_OP( 0x2b, andSetCarry, Immediate );

	SET_ADDRMODE_OP( 0x6b, andRotateRight, Immediate );

	SET_ADDRMODE_OP( 0xcb, subtractFromAccAndX, Immediate );

	SET_ADDRMODE_OP( 0xa3, loadAccTransferToX, IndirectX )
	SET_ADDRMODE_OP( 0xa7, loadAccTransferToX, ZeroPage )
	SET_ADDRMODE_OP( 0xab, loadAccTransferToX, Immediate )
	SET_ADDRMODE_OP( 0xaf, loadAccTransferToX, Absolute )
	SET_ADDRMODE_OP( 0xb3, loadAccTransferToX, IndirectY )
	SET_ADDRMODE_OP( 0xb7, loadAccTransferToX, ZeroPageY )
	SET_ADDRMODE_OP( 0xbf, loadAccTransferToX, AbsoluteY )

	SET_ADDRMODE_OP( 0x83, storeAccAndX, IndirectX )
	SET_ADDRMODE_OP( 0x87, storeAccAndX, ZeroPage )
	SET_ADDRMODE_OP( 0x8f, storeAccAndX, Absolute )
	SET_ADDRMODE_OP( 0x97, storeAccAndX, ZeroPageY )

	SET_ADDRMODE_OP( 0xc3, decrementCompare, IndirectX )
	SET_ADDRMODE_OP( 0xc7, decrementCompare, ZeroPage )
	SET_ADDRMODE_OP( 0xcf, decrementCompare, Absolute )
	SET_ADDRMODE_OP( 0xd3, decrementCompare, IndirectYStore )
	SET_ADDRMODE_OP( 0xd7, decrementCompare, ZeroPageX )
	SET_ADDRMODE_OP( 0xdb, decrementCompare, AbsoluteYStore )
	SET_ADDRMODE_OP( 0xdf, decrementCompare, AbsoluteXStore )

	SET_ADDRMODE_OP( 0xe3, incrementSubtract, IndirectX )
	SET_ADDRMODE_OP( 0xe7, incrementSubtract, ZeroPage )
	SET_ADDRMODE_OP( 0xef, incrementSubtract, Absolute )
	SET_ADDRMODE_OP( 0xf3, incrementSubtract, IndirectYStore )
	SET_ADDRMODE_OP( 0xf7, incrementSubtract, ZeroPageX )
	SET_ADDRMODE_OP( 0xfb, incrementSubtract, AbsoluteYStore )
	SET_ADDRMODE_OP( 0xff, incrementSubtract, AbsoluteXStore )

	SET_ADDRMODE_OP( 0x23, rotateLeftAnd, IndirectX )
	SET_ADDRMODE_OP( 0x27, rotateLeftAnd, ZeroPage )
	SET_ADDRMODE_OP( 0x2f, rotateLeftAnd, Absolute )
	SET_ADDRMODE_OP( 0x33, rotateLeftAnd, IndirectYStore )
	SET_ADDRMODE_OP( 0x37, rotateLeftAnd, ZeroPageX )
	SET_ADDRMODE_OP( 0x3b, rotateLeftAnd, AbsoluteYStore )
	SET_ADDRMODE_OP( 0x3f, rotateLeftAnd, AbsoluteXStore )

	SET_ADDRMODE_OP( 0x63, rotateRightAdd, IndirectX )
	SET_ADDRMODE_OP( 0x67, rotateRightAdd, ZeroPage )
	SET_ADDRMODE_OP( 0x6f, rotateRightAdd, Absolute )
	SET_ADDRMODE_OP( 0x73, rotateRightAdd, IndirectYStore )
	SET_ADDRMODE_OP( 0x77, rotateRightAdd, ZeroPageX )
	SET_ADDRMODE_OP( 0x7b, rotateRightAdd, AbsoluteYStore )
	SET_ADDRMODE_OP( 0x7f, rotateRightAdd, AbsoluteXStore )

	SET_ADDRMODE_OP( 0x03, shiftLeftOrAcc, IndirectX )
	SET_ADDRMODE_OP( 0x07, shiftLeftOrAcc, ZeroPage )
	SET_ADDRMODE_OP( 0x0f, shiftLeftOrAcc, Absolute )
	SET_ADDRMODE_OP( 0x13, shiftLeftOrAcc, IndirectYStore )
	SET_ADDRMODE_OP( 0x17, shiftLeftOrAcc, ZeroPageX )
	SET_ADDRMODE_OP( 0x1b, shiftLeftOrAcc, AbsoluteYStore )
	SET_ADDRMODE_OP( 0x1f, shiftLeftOrAcc, AbsoluteXStore )

	SET_ADDRMODE_OP( 0x43, shiftRightExclusiveOr, IndirectX )
	SET_ADDRMODE_OP( 0x47, shiftRightExclusiveOr, ZeroPage )
	SET_ADDRMODE_OP( 0x4f, shiftRightExclusiveOr, Absolute )
	SET_ADDRMODE_OP( 0x53, shiftRightExclusiveOr, IndirectYStore )
	SET_ADDRMODE_OP( 0x57, shiftRightExclusiveOr, ZeroPageX )
	SET_ADDRMODE_OP( 0x5b, shiftRightExclusiveOr, AbsoluteYStore )
	SET_ADDRMODE_OP( 0x5f, shiftRightExclusiveOr, AbsoluteXStore )

	SET_ADDRMODE_OP( 0xeb, subtractFromAcc, Immediate )

	SET_ADDRMODE_OP( 0x8b, transferXToAccAnd, Immediate )

	SET_IMPLIED( 0x9e, andXAddrHigh )
	SET_IMPLIED( 0x9c, andYAddrHigh )

	SET_ADDRMODE_OP( 0x9b, andXAccStoreStackPointer, AbsoluteYStore )

	SET_ADDRMODE_OP( 0x9f, andXAccSeven, AbsoluteYStore )
	SET_ADDRMODE_OP( 0x93, andXAccSeven, IndirectYStore )

	SET_ADDRMODE_OP( 0xbb, andSPTransferToAcXSP, AbsoluteY );

	SET_IMPLIED( 0x1a, noOperation )
	SET_IMPLIED( 0x3a, noOperation )
	SET_IMPLIED( 0x5a, noOperation )
	SET_IMPLIED( 0x7a, noOperation )
	SET_IMPLIED( 0xda, noOperation )
	SET_IMPLIED( 0xfa, noOperation )

	SET_ADDRMODE_OP( 0x0c, ignoreByte, Absolute )
	SET_ADDRMODE_OP( 0x1c, ignoreByte, AbsoluteX )
	SET_ADDRMODE_OP( 0x3c, ignoreByte, AbsoluteX )
	SET_ADDRMODE_OP( 0x5c, ignoreByte, AbsoluteX )
	SET_ADDRMODE_OP( 0x7c, ignoreByte, AbsoluteX )
	SET_ADDRMODE_OP( 0xdc, ignoreByte, AbsoluteX )
	SET_ADDRMODE_OP( 0xfc, ignoreByte, AbsoluteX )
	SET_ADDRMODE_OP( 0x04, ignoreByte, ZeroPage )
	SET_ADDRMODE_OP( 0x14, ignoreByte, ZeroPageX )
	SET_ADDRMODE_OP( 0x34, ignoreByte, ZeroPageX )
	SET_ADDRMODE_OP( 0x44, ignoreByte, ZeroPage )
	SET_ADDRMODE_OP( 0x54, ignoreByte, ZeroPageX )
	SET_ADDRMODE_OP( 0x64, ignoreByte, ZeroPage )
	SET_ADDRMODE_OP( 0x74, ignoreByte, ZeroPageX )
	SET_ADDRMODE_OP( 0x80, ignoreByte, Immediate )
	SET_ADDRMODE_OP( 0x82, ignoreByte, Immediate )
	SET_ADDRMODE_OP( 0x89, ignoreByte, Immediate )
	SET_ADDRMODE_OP( 0xc2, ignoreByte, Immediate )
	SET_ADDRMODE_OP( 0xd4, ignoreByte, ZeroPageX )
	SET_ADDRMODE_OP( 0xe2, ignoreByte, Immediate )
	SET_ADDRMODE_OP( 0xf4, ignoreByte, ZeroPageX )
}

#undef SET_ADDRMODE_OP
#undef SET_IMPLIED
#undef SET_BRANCH

#define writeBytes( var ) out.write( ( const char* )&var, sizeof( var ) );
#define readBytes( var ) in.read( ( char* )&var, sizeof( var ) );

void Cpu::saveState( std::ostream& out ) const
{
	writeBytes( m_ram );
	writeBytes( m_cycles );
	writeBytes( m_nmi );
	writeBytes( m_irq );
	writeBytes( m_programCounter );
	writeBytes( m_accumulator );
	writeBytes( m_xRegister );
	writeBytes( m_yRegister );
	writeBytes( m_stackPointer );
	writeBytes( m_status );
	writeBytes( m_oddCycle );
	writeBytes( m_halt );
}

void Cpu::loadState( std::istream& in )
{
	readBytes( m_ram );
	readBytes( m_cycles );
	readBytes( m_nmi );
	readBytes( m_irq );
	readBytes( m_programCounter );
	readBytes( m_accumulator );
	readBytes( m_xRegister );
	readBytes( m_yRegister );
	readBytes( m_stackPointer );
	readBytes( m_status );
	readBytes( m_oddCycle );
	readBytes( m_halt );
}

#undef writeBytes
#undef readBytes

enum ByteInterpretation
{
	RAW,
	OPCODE,
	ASCII,
	NUM_BYTE_INTERPRETATIONS
};

void Cpu::dump( Word address )
{
	std::cout << "DUMP OF " << toHex( address, 2 ) << '\n';

	// center view of memory on address
	if ( address >= 128 )
		address -= 128;

	// align memory for output
	address &= 0xfff0;

	std::cout << "      ";
	for ( int bi = 0; bi < NUM_BYTE_INTERPRETATIONS; bi++ )
	{
		std::cout << "| ";
		for ( unsigned int i = 0; i < 0x10; i++ )
		{
			std::cout << toHex( i, 0.5, "" );
			std::string spaces;
			switch ( bi )
			{
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
	for ( int i = 0; i < 156; i++ ) std::cout << '-';
	std::cout << '\n';

	for ( unsigned int high = 0; high < 0x10; high++ )
	{
		std::cout << toHex( address + ( high << 4 ), 2 ) << ' ';
		for ( int bi = 0; bi < NUM_BYTE_INTERPRETATIONS; bi++ )
		{
			std::cout << "| ";
			for ( unsigned int low = 0; low < 0x10; low++ )
			{
				Byte value = read( address + ( ( high << 4 ) + low ) );
				switch ( bi )
				{
					case RAW:
					{
						std::cout << toHex( value, 1, "" );
						break;
					}

					case OPCODE:
					{
						auto operation = s_cpuOperations[ value ];
						std::cout << ( ( operation.instr != Instruction::illegalOpcode )
									   ? nes::getInstructionName( operation.instr )
									   : "   " );
						break;
					}

					case ASCII:
					{
						std::cout << ( isspace( value ) ? ' ' : ( char )value );
						break;
					}
				}
				std::cout << ' ';
			}
		}
		std::cout << '\n';
	}
}

void Cpu::dumpStack()
{
	std::cout << "Stack:\n";
	for ( int i = ( StackOffset | m_stackPointer ) + 1; i <= ( StackOffset | STACK_START ); i++ )
	{
		std::cout << ' ' << toHex( read( i ), 1 ) << '\n';
	}
	std::cout << "-----\n";
}

void Cpu::dumpState()
{
	std::cout << "Accumulator: " << toHex( m_accumulator ) << '\n';
	std::cout << "X register: " << toHex( m_xRegister ) << '\n';
	std::cout << "Y register: " << toHex( m_yRegister ) << '\n';
	std::cout << "Program counter: " << toHex( m_programCounter ) << '\n';
	std::cout << "Stack pointer: " << toHex( m_stackPointer ) << '\n';
	std::cout << "Status: NV BDIZC\n";
	std::cout << "        " << std::bitset<8>( m_status ) << '\n';
}