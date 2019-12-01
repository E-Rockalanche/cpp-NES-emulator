#include "Cpu.hpp"

#include "AddressMode.hpp"
#include "apu.hpp"
#include "cartridge.hpp"
#include "controller.hpp"
#include "Instruction.hpp"
#include "ppu.hpp"

namespace nes
{
	Cpu cpu;
}

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

	constexpr Word STACK_OFFSET = 0x0100;

	constexpr Byte STACK_START = 0xfd;
	constexpr Byte STATUS_START = 0x34;

	inline bool crossedPage( Word addr1, Word addr2 )
	{
		return ( addr1 & 0xff00 ) != ( addr2 & 0xff00 );
	}

	inline bool isNegative( Byte value )
	{
		return value & 0x80;
	}

	using VoidCpuMemberFunction = void( Cpu::* )( void );

	VoidCpuMemberFunction s_cpuOperations[ 0x100 ];
}

void Cpu::power()
{
	m_stackPointer = STACK_START;
	m_status = STATUS_START;

	m_accumulator = 0;
	m_xRegister = 0;
	m_yRegister = 0;

	write( APU_STATUS, 0 );
	write( APU_FRAME_COUNT, 0 );
	for ( Word i = 0; i < 16; ++i )
		write( APU_START + i, 0 );

	m_cycles = 0;

	m_programCounter = readWordTick( RESET_VECTOR );
	dbLogWord( "PC", m_programCounter );

	m_halt = false;
	m_nmi = -1;
	m_irq = -1;
	m_oddCycle = false;
}

void Cpu::reset()
{
	m_stackPointer -= 3;
	setStatus( DisableInterrupts );
	write( APU_STATUS, 0 );

	m_programCounter = readWordTick( RESET_VECTOR );
	dbLogWord( "PC", m_programCounter );

	m_cycles = 0;
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
		( this->*operation )();
	}
}

void Cpu::runFrame()
{
	m_cycles = 0;
	while( !halted() && !PPU::readyToDraw() )
	{
		executeInstruction();
	}

	APU::runFrame( m_cycles );
}

void Cpu::tick()
{
	m_oddCycle = !m_oddCycle;
	for ( int i = 0; i < 3; ++i )
	{
		m_nmi += ( m_nmi >= 0 );
		m_irq += ( m_irq >= 0 );
		// m_ppu->tick();
		PPU::clockTick();
	}
	++m_cycles;
}

void Cpu::setArithmeticFlags( Byte value )
{
	setStatus( Negative, value & 0x80 );
	setStatus( Zero, value == 0 );
}

void Cpu::addToAccumulator( Byte value )
{
	unsigned int result = m_accumulator + value + testStatus( Carry );
	bool carry = result & 0x0100;
	checkOverflow( m_accumulator, value, result );
	setStatus( Carry, carry );
	m_accumulator = result & 0xff;
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
		// m_ppu->writeToOAM( data );
		PPU::writeToOAM( data );
	}
}

Byte Cpu::read( Word address )
{
	switch ( address )
	{
		case RAM_START ... RAM_END:
			return m_ram[ address - RAM_START ];

		case PPU_START ... PPU_END:
			// return m_ppu->read( address - PPU_START );
			return PPU::readByte( ( address - PPU_START ) % PPU_SIZE );

		case APU_START ... APU_END:
			if ( address == OAM_DMA )
				return 0;

			return APU::readByte( m_cycles, address );
			// return m_apu->read( m_cycles, address );

		case JOY1 ... JOY2:
		{
			// auto* controller = m_controllerPorts[ address - JOY1 ];
			auto controller = controller_ports[ address - JOY1 ];
			return controller ? controller->read() : 0;
		}

		case CARTRIDGE_START ... CARTRIDGE_END:
			// return m_cartridge->readPRG( address );
			return cartridge->readPRG( address );

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
			// m_ppu->write( address - PPU_START, value );
			PPU::writeByte( address - PPU_START, value );
			break;

		case APU_START ... APU_END:
		case JOY2:
		{
			if ( address == OAM_DMA )
				oamDmaTransfer( value );
			else
				// m_apu->write( m_cycles, address, value );
				APU::writeByte( m_cycles, address, value );
			break;
		}

		case JOY1:
		{
			// for ( auto* controller : m_controllerPorts )
			for( auto* controller : controller_ports )
				controller->write( value );
			break;
		}

		case CARTRIDGE_START ... CARTRIDGE_END:
			// m_cartridge->writePRG( address, value );
			cartridge->writePRG( address, value );
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

template <AddressMode Mode>
Word Cpu::getAddress()
{
	switch ( Mode )
	{
		case AddressMode::Immediate:
			return m_programCounter++;

		case AddressMode::Absolute:
		{
			auto address = readWordTick( m_programCounter );
			m_programCounter += 2;
			return address;
		}

		case AddressMode::ZeroPage:
			return readByteTick( m_programCounter++ );

		case AddressMode::ZeroPageX:
		{
			auto address = ( readByteTick( m_programCounter++ ) + m_xRegister ) & 0xff;
			tick();
			return address;
		}

		case AddressMode::ZeroPageY:
		{
			auto address = ( readByteTick( m_programCounter++ ) + m_yRegister ) & 0xff;
			tick();
			return address;
		}

		case AddressMode::AbsoluteX:
		{
			auto page1 = readWordTick( m_programCounter );
			m_programCounter += 2;
			auto address = page1 + m_xRegister;
			if ( crossedPage( page1, address ) )
				dummyRead();
			return address;
		}

		case AddressMode::AbsoluteXStore:
		{
			auto page1 = readWordTick( m_programCounter );
			m_programCounter += 2;
			auto address = page1 + m_xRegister;
			dummyRead();
			return address;
		}

		case AddressMode::AbsoluteY:
		{
			auto page1 = readWordTick( m_programCounter );
			m_programCounter += 2;
			auto address = page1 + m_yRegister;
			if ( crossedPage( page1, address ) )
				dummyRead();
			return address;
		}

		case AddressMode::AbsoluteYStore:
		{
			auto page1 = readWordTick( m_programCounter );
			m_programCounter += 2;
			auto address = page1 + m_yRegister;
			dummyRead();
			return address;
		}

		case AddressMode::Indirect:
		{
			auto address = readWordTickBug( readWordTick( m_programCounter ) );
			m_programCounter += 2;
			return address;
		}

		case AddressMode::IndirectX:
		{
			auto zp = ( readByteTick( m_programCounter++ ) + m_xRegister ) & 0xff;
			tick();
			return readWordTickBug( zp );
		}

		case AddressMode::IndirectY:
		{
			auto zp = readByteTick( m_programCounter++ );
			auto address = readWordTickBug( zp ) + m_yRegister;
			if ( crossedPage( address - m_yRegister, address ) )
				dummyRead();
			return address;
		}

		case AddressMode::IndirectYStore:
		{
			auto zp = readByteTick( m_programCounter++ );
			auto address = readWordTickBug( zp ) + m_yRegister;
			dummyRead();
			return address;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
//                                 OPERATIONS                                 //
////////////////////////////////////////////////////////////////////////////////

template <AddressMode Mode>
void Cpu::addWithCarry()
{
	addToAccumulator( readByteTick( getAddress<Mode>() ) );
}

template <AddressMode Mode>
void Cpu::bitwiseAnd()
{
	Byte value = readByteTick( getAddress<Mode>() );
	m_accumulator &= value;
	setArithmeticFlags( m_accumulator );
}

template <AddressMode Mode>
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
	setStatus( Carry, result & 0x100 );
}

void Cpu::branchOnCarryClear()
{
	conditionalBranch( !testStatus( Carry ) );
}

void Cpu::branchOnCarrySet()
{
	conditionalBranch( testStatus( Carry ) );
}

void Cpu::branchOnZero()
{
	conditionalBranch( testStatus( Zero ) );
}

template <AddressMode Mode>
void Cpu::testBits()
{
	Byte value = readByteTick( getAddress<Mode>() );
	setStatus( Negative, value & Negative );
	setStatus( Overflow, value & Overflow );
	setStatus( Zero, ( value & m_accumulator ) == 0 );
}

void Cpu::branchOnNegative()
{
	conditionalBranch( testStatus( Negative ) );
}

void Cpu::branchOnNotZero()
{
	conditionalBranch( !testStatus( Zero ) );
}

void Cpu::branchOnPositive()
{
	conditionalBranch( !testStatus( Negative ) );
}

void Cpu::forceBreak()
{
	dummyRead();
	setStatus( Break );
	pushWord( m_programCounter + 1 );
	pushByte( m_status );
	setStatus( DisableInterrupts );
	m_programCounter = readWordTick( IRQ_VECTOR );
}

void Cpu::branchOnOverflowClear()
{
	conditionalBranch( !testStatus( Overflow ) );
}

void Cpu::branchOnOverflowSet()
{
	conditionalBranch( testStatus( Overflow ) );
}

void Cpu::clearCarryFlag()
{
	clearStatus( Carry );
	tick();
}

void Cpu::clearDecimalFlag()
{
	clearStatus( DecimalMode );
	tick();
}

void Cpu::clearInterruptDisableFlag()
{
	clearStatus( DisableInterrupts );
	tick();
}

void Cpu::clearOverflowFlag()
{
	clearStatus( Overflow );
	tick();
}

template <AddressMode Mode>
void Cpu::compareWithAcc()
{
	compareWithValue<Mode>( m_accumulator );
}

template <AddressMode Mode>
void Cpu::compareWithX()
{
	compareWithValue<Mode>( m_xRegister );
}

template <AddressMode Mode>
void Cpu::compareWithY()
{
	compareWithValue<Mode>( m_yRegister );
}

template <AddressMode Mode>
void Cpu::decrement()
{
	Word address = getAddress<Mode>();
	Byte result = readByteTick( address ) - 1;
	tick();
	setArithmeticFlags( result );
	writeByteTick( address, result );
}

void Cpu::decrementX()
{
	setArithmeticFlags( --m_xRegister );
	tick();
}

void Cpu::decrementY()
{
	setArithmeticFlags( --m_yRegister );
	tick();
}

template <AddressMode Mode>
void Cpu::exclusiveOr()
{
	m_accumulator ^= readByteTick( getAddress<Mode>() );
	tick();
}

template <AddressMode Mode>
void Cpu::increment()
{
	Word address = getAddress<Mode>();
	Byte result = readByteTick( address ) + 1;
	tick();
	setArithmeticFlags( result );
	writeByteTick( address, result );
}

void Cpu::incrementX()
{
	setArithmeticFlags( ++m_xRegister );
	tick();
}

void Cpu::incrementY()
{
	setArithmeticFlags( ++m_yRegister );
	tick();
}

template <AddressMode Mode>
void Cpu::jump()
{
	m_programCounter = getAddress<Mode>();
}

template <AddressMode Mode>
void Cpu::jumpToSubroutine()
{
	tick();
	pushWord( m_programCounter + 1 );
	m_programCounter = readWordTick( m_programCounter );
}

template <AddressMode Mode>
void Cpu::loadAcc()
{
	m_accumulator = readByteTick( getAddress<Mode>() );
	setArithmeticFlags( m_accumulator );
}

template <AddressMode Mode>
void Cpu::loadX()
{
	m_xRegister = readByteTick( getAddress<Mode>() );
	setArithmeticFlags( m_xRegister );
}

template <AddressMode Mode>
void Cpu::loadY()
{
	m_yRegister = readByteTick( getAddress<Mode>() );
	setArithmeticFlags( m_yRegister );
}

template <AddressMode Mode>
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

void Cpu::noOperation()
{
	tick();
}

template <AddressMode Mode>
void Cpu::bitwiseOr()
{
	m_accumulator |= readByteTick( getAddress<Mode>() );
	setArithmeticFlags( m_accumulator );
}

void Cpu::pushAcc()
{
	tick();
	pushByte( m_accumulator );
}

void Cpu::pushStatus()
{
	tick();
	pushByte( m_status | 0x30 );
}

void Cpu::popAcc()
{
	dummyRead();
	tick();
	m_accumulator = popByte();
	setArithmeticFlags( m_accumulator );
}

void Cpu::popStatus()
{
	dummyRead();
	tick();
	m_status = ( popByte() & ~0x30 ) | ( m_status & 0x30 );
}

template <AddressMode Mode>
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

template <AddressMode Mode>
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

void Cpu::returnFromInterrupt()
{
	dummyRead();
	m_status = ( popByte() & ~0x30 ) | ( m_status & 0x30 );
	m_programCounter = popWord();
	tick();
}

void Cpu::returnFromSubroutine()
{
	readByteTick( m_programCounter );
	tick();
	m_programCounter = popWord() + 1;
	tick();
}

template <AddressMode Mode>
void Cpu::subtractFromAcc()
{
	addToAccumulator( 1 + ~readByteTick( getAddress<Mode>() ) );
}

void Cpu::setCarryFlag()
{
	setStatus( Carry );
	tick();
}

void Cpu::setDecimalFlag()
{
	setStatus( DecimalMode );
	tick();
}

void Cpu::setInterruptDisableFlag()
{
	setStatus( DisableInterrupts );
	tick();
}

template <AddressMode Mode>
void Cpu::storeAcc()
{
	writeByteTick( getAddress<Mode>(), m_accumulator );
}

template <AddressMode Mode>
void Cpu::storeX()
{
	writeByteTick( getAddress<Mode>(), m_xRegister );
}

template <AddressMode Mode>
void Cpu::storeY()
{
	writeByteTick( getAddress<Mode>(), m_yRegister );
}

void Cpu::transferAccToX()
{
	m_xRegister = m_accumulator;
	setArithmeticFlags( m_xRegister );
	tick();
}

void Cpu::transferAccToY()
{
	m_yRegister = m_accumulator;
	setArithmeticFlags( m_yRegister );
	tick();
}

void Cpu::transferStackPointerToX()
{
	m_xRegister = m_stackPointer;
	setArithmeticFlags( m_xRegister );
	tick();
}

void Cpu::transferXToAcc()
{
	m_accumulator = m_xRegister;
	setArithmeticFlags( m_accumulator );
	tick();
}

void Cpu::transferXToStackPointer()
{
	m_stackPointer = m_xRegister;
	tick();
}

void Cpu::transferYToAcc()
{
	m_accumulator = m_yRegister;
	setArithmeticFlags( m_accumulator );
	tick();
}

void Cpu::illegalOpcode()
{
	halt();
}

#define SET_ADDRMODE_OP( opcode, instr, addrmode ) dbAssert( s_cpuOperations[ opcode ] == &Cpu::illegalOpcode ); s_cpuOperations[ opcode ] = &Cpu::instr< AddressMode::addrmode >;
#define SET_OP( opcode, instr ) dbAssert( s_cpuOperations[ opcode ] == &Cpu::illegalOpcode ); s_cpuOperations[ opcode ] = &Cpu::instr;

void Cpu::initialize()
{
	for ( auto& ptr : s_cpuOperations )
		ptr = &Cpu::illegalOpcode;

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

	SET_OP( 0x90, branchOnCarryClear )
	SET_OP( 0xb0, branchOnCarrySet )
	SET_OP( 0xf0, branchOnZero )

	SET_ADDRMODE_OP( 0x24, testBits, ZeroPage )
	SET_ADDRMODE_OP( 0x2c, testBits, Absolute )

	SET_OP( 0x30, branchOnNegative )
	SET_OP( 0xd0, branchOnNotZero )
	SET_OP( 0x10, branchOnPositive )

	SET_OP( 0x00, forceBreak )

	SET_OP( 0x50, branchOnOverflowClear )
	SET_OP( 0x70, branchOnOverflowSet )

	SET_OP( 0x18, clearCarryFlag )
	SET_OP( 0xd8, clearDecimalFlag )
	SET_OP( 0x58, clearInterruptDisableFlag )
	SET_OP( 0xb8, clearOverflowFlag )

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

	SET_OP( 0xca, decrementX )
	SET_OP( 0x88, decrementY )

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

	SET_OP( 0xe8, incrementX )
	SET_OP( 0xc8, incrementY )

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

	SET_OP( 0xea, noOperation );

	SET_ADDRMODE_OP( 0x09, bitwiseOr, Immediate );
	SET_ADDRMODE_OP( 0x05, bitwiseOr, ZeroPage );
	SET_ADDRMODE_OP( 0x15, bitwiseOr, ZeroPageX );
	SET_ADDRMODE_OP( 0x0d, bitwiseOr, Absolute );
	SET_ADDRMODE_OP( 0x1d, bitwiseOr, AbsoluteX );
	SET_ADDRMODE_OP( 0x19, bitwiseOr, AbsoluteY );
	SET_ADDRMODE_OP( 0x01, bitwiseOr, IndirectX );
	SET_ADDRMODE_OP( 0x11, bitwiseOr, IndirectY );

	SET_OP( 0x48, pushAcc );
	SET_OP( 0x08, pushStatus );
	SET_OP( 0x68, popAcc );
	SET_OP( 0x28, popStatus );

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

	SET_OP( 0x40, returnFromInterrupt );

	SET_OP( 0x60, returnFromSubroutine );

	SET_ADDRMODE_OP( 0xe9, subtractFromAcc, Immediate );
	SET_ADDRMODE_OP( 0xe5, subtractFromAcc, ZeroPage );
	SET_ADDRMODE_OP( 0xf5, subtractFromAcc, ZeroPageX );
	SET_ADDRMODE_OP( 0xed, subtractFromAcc, Absolute );
	SET_ADDRMODE_OP( 0xfd, subtractFromAcc, AbsoluteX );
	SET_ADDRMODE_OP( 0xf9, subtractFromAcc, AbsoluteY );
	SET_ADDRMODE_OP( 0xe1, subtractFromAcc, IndirectX );
	SET_ADDRMODE_OP( 0xf1, subtractFromAcc, IndirectY );

	SET_OP( 0x38, setCarryFlag );
	SET_OP( 0xf8, setDecimalFlag );
	SET_OP( 0x78, setInterruptDisableFlag );

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

	SET_OP( 0xaa, transferAccToX );
	SET_OP( 0xa8, transferAccToY );
	SET_OP( 0xba, transferStackPointerToX );
	SET_OP( 0x8a, transferXToAcc );
	SET_OP( 0x9a, transferXToStackPointer );
	SET_OP( 0x98, transferYToAcc );
}

#undef SET_ADDRMODE_OP
#undef SET_OP

#define writeBytes( var ) out.write( ( const char* )&var, sizeof( var ) );
#define readBytes( var ) in.read( ( char* )&var, sizeof( var ) );

void Cpu::saveState( std::ostream& out )
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