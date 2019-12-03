#ifndef NES_CPU_HPP
#define NES_CPU_HPP

#include "Ram.hpp"
#include "types.hpp"

#include <iostream>

namespace nes
{

	class Apu;
	class Cartridge;
	class Controller;
	class Ppu;

	class Cpu
	{
	public:

		void setPPU( Ppu& ppu )
		{
			m_ppu = &ppu;
		}

		void setCartridge( Cartridge* cartridge )
		{
			m_cartridge = cartridge;
		}

		void setController( Controller* controller, size_t port )
		{
			m_controllerPorts[ port ] = controller;
		}

		void reset();
		void power();
		void executeInstruction();
		void runFrame();

		void setNMI( bool on = true )
		{
			m_nmi = on ? 0 : -1;
		}

		void setIRQ( bool on = true )
		{
			m_irq = on ? 0 : -1;
		}

		bool halted() const { return m_halt; }

		void saveState( std::ostream& out );
		void loadState( std::istream& in );

		// public so APU can read DMC
		Byte read( Word address );

		static void initialize();

	private:

		enum class AddressMode;

		enum StatusFlag : Byte
		{
			Carry = 1 << 0,
			Zero = 1 << 1,
			DisableInterrupts = 1 << 2,
			DecimalMode =  1 << 3,
			Break = 1 << 4,
			Unused = 1 << 5,
			Overflow = 1 << 6,
			Negative = 1 << 7,
		};

		static constexpr Word StackOffset = 0x0100;
		static constexpr size_t RamSize = 0x0800;

	private:

		void tick();

		void write( Word address, Byte value );

		void dummyRead()
		{
			readByteTick( m_programCounter );
		}

		Byte readByteTick( Word address )
		{
			tick();
			return read( address );
		}

		Word readWordTick( Word address )
		{
			Word low = readByteTick( address );
			Word high = readByteTick( address + 1 );
			return ( high << 8 ) | low;
		}

		Word readWordTickBug( Word address )
		{
			Word low = readByteTick( address );
			Word highAddress = ( ( address & 0xff ) == 0xff )
				? ( address & 0xff00 )
				: ( address + 1 );
			Word high = readByteTick( highAddress );
			return ( high << 8 ) | low;
		}

		void writeByteTick( Word address, Byte value )
		{
			tick();
			write( address, value );
		}

		void pushByte( Byte value )
		{
			writeByteTick( StackOffset | m_stackPointer--, value );
		}

		void pushWord( Word value )
		{
			pushByte( value >> 8 );
			pushByte( value & 0xff );
		}

		Byte popByte()
		{
			return readByteTick( StackOffset | ++m_stackPointer );
		}

		Word popWord()
		{
			Word low = popByte();
			Word high = popByte();
			return ( high << 8 ) | low;
		}

		void setStatus( StatusFlag flag )
		{
			m_status |= flag;
		}

		void clearStatus( StatusFlag flag )
		{
			m_status &= ~flag;
		}

		void setStatus( StatusFlag flag, bool isSet )
		{
			if ( isSet )
				setStatus( flag );
			else
				clearStatus( flag );
		}

		bool testStatus( StatusFlag flag ) const
		{
			return m_status & flag;
		}

		void setArithmeticFlags( Byte value );

		void checkOverflow( Byte reg, Byte value, Byte result )
		{
			bool overflowed = ~( reg ^ value ) & ( reg ^ result ) & 0x80;
			setStatus( Overflow, overflowed );
		}

		void nmi();
		void irq();

		void halt() { m_halt = true; }
		bool breaked() const { return testStatus( Break ); }

		void addToAccumulator( Byte value );

		void conditionalBranch( bool branch );

		template <AddressMode>
		Word getAddress();

		template <AddressMode Mode>
		void compareWithValue( Byte reg )
		{
			Byte value = readByteTick( getAddress<Mode>() );
			setStatus( Carry, reg >= value );
			setArithmeticFlags( reg - value );
		}

		void oamDmaTransfer( Byte addressHigh );

		// operations:

		#define DEF_ADDRMODE_OP( name ) template <AddressMode Mode> void name();

		DEF_ADDRMODE_OP( addWithCarry )
		DEF_ADDRMODE_OP( bitwiseAnd )
		DEF_ADDRMODE_OP( shiftLeft )
		void branchOnCarryClear();
		void branchOnCarrySet();
		void branchOnZero();
		DEF_ADDRMODE_OP( testBits )
		void branchOnNegative();
		void branchOnNotZero();
		void branchOnPositive();
		void forceBreak();
		void branchOnOverflowClear();
		void branchOnOverflowSet();
		void clearCarryFlag();
		void clearDecimalFlag();
		void clearInterruptDisableFlag();
		void clearOverflowFlag();
		DEF_ADDRMODE_OP( compareWithAcc )
		DEF_ADDRMODE_OP( compareWithX )
		DEF_ADDRMODE_OP( compareWithY )
		DEF_ADDRMODE_OP( decrement )
		void decrementX();
		void decrementY();
		DEF_ADDRMODE_OP( exclusiveOr )
		DEF_ADDRMODE_OP( increment )
		void incrementX();
		void incrementY();
		DEF_ADDRMODE_OP( jump )
		DEF_ADDRMODE_OP( jumpToSubroutine )
		DEF_ADDRMODE_OP( loadAcc )
		DEF_ADDRMODE_OP( loadX )
		DEF_ADDRMODE_OP( loadY )
		DEF_ADDRMODE_OP( shiftRight )
		void noOperation();
		DEF_ADDRMODE_OP( bitwiseOr )
		void pushAcc();
		void pushStatus();
		void popAcc();
		void popStatus();
		DEF_ADDRMODE_OP( rotateLeft )
		DEF_ADDRMODE_OP( rotateRight )
		void returnFromInterrupt();
		void returnFromSubroutine();
		DEF_ADDRMODE_OP( subtractFromAcc )
		void setCarryFlag();
		void setDecimalFlag();
		void setInterruptDisableFlag();
		DEF_ADDRMODE_OP( storeAcc )
		DEF_ADDRMODE_OP( storeX )
		DEF_ADDRMODE_OP( storeY )
		void transferAccToX();
		void transferAccToY();
		void transferStackPointerToX();
		void transferXToAcc();
		void transferXToStackPointer();
		void transferYToAcc();
		void illegalOpcode();

		#undef DEF_ADDRMODE_OP

	private:

		Apu* m_apu = nullptr;
		Ppu* m_ppu = nullptr;
		Cartridge* m_cartridge = nullptr;
		Controller* m_controllerPorts[ 2 ]{ nullptr, nullptr };

		int m_cycles = 0;
		int m_nmi = -1;
		int m_irq = -1;

		Word m_programCounter = 0;

		Ram<RamSize> m_ram;

		Byte m_accumulator = 0;
		Byte m_xRegister = 0;
		Byte m_yRegister = 0;
		Byte m_stackPointer = 0;
		Byte m_status = 0;

		bool m_oddCycle = false;
		bool m_halt = false;
	};
}

#endif