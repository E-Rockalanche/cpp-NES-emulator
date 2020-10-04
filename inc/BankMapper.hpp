#ifndef BANK_MAPPER_HPP
#define BANK_MAPPER_HPP

#include "types.hpp"

#include <stdx/assert.h>

using size_t = std::size_t;

namespace nes
{

	template <size_t NumSlots, size_t MinBankSize>
	class BankMapper
	{
	public:

		static_assert( NumSlots > 0 );
		static_assert( MinBankSize > 0 );

		BankMapper() = default;
		BankMapper( size_t memorySize ) : m_memorySize( memorySize )
		{
			dbAssert( m_memorySize >= NumSlots * MinBankSize );
			dbAssert( m_memorySize % MinBankSize == 0 );

			reset();
		}

		void setBank( size_t slot, int bank )
		{
			// wrap bank number
			if ( bank < 0 )
				bank += m_memorySize / MinBankSize;

			m_bankOffsets[ slot ] = ( bank * MinBankSize ) % m_memorySize;
		}

		// for sizes larger than min bank size
		void setBank( size_t slot, int bank, size_t bankSize )
		{
			dbAssert( bankSize >= MinBankSize );
			dbAssertMessage( ( ( bankSize % MinBankSize ) == 0 ), "bank size must be multiple of minimum bank size" );

			// wrap bank number
			if ( bank < 0 )
				bank += static_cast<int>( m_memorySize / bankSize );

			size_t numSlots = bankSize / MinBankSize;
			for( size_t i = 0; i < numSlots; ++i )
			{
				size_t offset = ( bank * bankSize ) + ( i * MinBankSize );
				m_bankOffsets[ i + slot * numSlots ] = offset % m_memorySize;
			}
		}

		constexpr size_t bankSize() const { return MinBankSize; }
		constexpr size_t numSlots() const { return NumSlots; }

		size_t operator[]( Word address )
		{
			size_t slot = address / MinBankSize;
			dbAssert( slot < NumSlots );
			return ( address % MinBankSize ) + m_bankOffsets[ slot ];
		}

		void reset()
		{
			for( size_t i = 0; i < NumSlots; ++i )
			{
				m_bankOffsets[ i ] = ( i * MinBankSize ) % m_memorySize;
			}
		}

		void setMemorySize( size_t memorySize )
		{
			m_memorySize = memorySize;
		}

	private:

		size_t m_bankOffsets[ NumSlots ];
		size_t m_memorySize = 0;
	};

}

#endif