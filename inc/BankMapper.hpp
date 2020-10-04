#ifndef BANK_MAPPER_HPP
#define BANK_MAPPER_HPP

#include <stdx/assert.h>
#include "types.hpp"

using size_t = std::size_t;

namespace nes
{

	template <size_t NUM_SLOTS, size_t MIN_BANK_SIZE>
	class BankMapper
	{
	public:

		static_assert( NUM_SLOTS > 0 );
		static_assert( MIN_BANK_SIZE > 0 );

		BankMapper() = default;
		BankMapper( size_t memorySize ) : m_memorySize( memorySize )
		{
			dbAssert( m_memorySize >= NUM_SLOTS * MIN_BANK_SIZE );
			dbAssert( m_memorySize % MIN_BANK_SIZE == 0 );

			reset();
		}

		void setBank( size_t slot, int bank )
		{
			// wrap bank number
			if ( bank < 0 )
				bank += m_memorySize / MIN_BANK_SIZE;

			m_bankOffsets[ slot ] = ( bank * MIN_BANK_SIZE ) % m_memorySize;
		}

		// for sizes larger than min bank size
		void setBank( size_t slot, int bank, size_t bankSize )
		{
			dbAssert( bankSize >= MIN_BANK_SIZE );
			dbAssertMessage( bankSize % MIN_BANK_SIZE == 0, "bank size must be multiple of minimum bank size" );

			// wrap bank number
			if ( bank < 0 )
				bank += static_cast<int>( m_memorySize / bankSize );

			size_t numSlots = bankSize / MIN_BANK_SIZE;
			for( size_t i = 0; i < numSlots; ++i )
			{
				size_t offset = ( bank * bankSize ) + ( i * MIN_BANK_SIZE );
				m_bankOffsets[ i + slot * numSlots ] = offset % m_memorySize;
			}
		}

		constexpr size_t bankSize() const { return MIN_BANK_SIZE; }
		constexpr size_t numSlots() const { return NUM_SLOTS; }

		size_t operator[]( Word address )
		{
			size_t slot = address / MIN_BANK_SIZE;
			dbAssert( slot < NUM_SLOTS );
			return ( address % MIN_BANK_SIZE ) + m_bankOffsets[ slot ];
		}

		void reset()
		{
			for( size_t i = 0; i < NUM_SLOTS; ++i )
			{
				m_bankOffsets[ i ] = ( i * MIN_BANK_SIZE ) % m_memorySize;
			}
		}

		void setMemorySize( size_t memorySize )
		{
			m_memorySize = memorySize;
		}

	private:

		size_t m_bankOffsets[ NUM_SLOTS ];
		size_t m_memorySize = 0;
	};

}

#endif