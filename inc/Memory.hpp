#ifndef NES_MEMORY_HPP
#define NES_MEMORY_HPP

#include <stdx/assert.h>
#include "types.hpp"

#include <memory>

namespace nes
{

	class Memory
	{
	public:

		Memory() = default;
		Memory( size_t dataSize )
			: m_data( std::make_unique<Byte[]>( dataSize ) )
			, m_size( dataSize )
		{}
		Memory( const Memory& ) = delete;
		Memory( Memory&& ) = default;

		Memory& operator=( const Memory& ) = delete;
		Memory& operator=( Memory&& ) = default;

		Byte& operator[]( size_t index )
		{
			dbAssertMessage( index < m_size, "index (%zu) out of bounds (%zu)", index, m_size );
			return m_data[ index ];
		}
		Byte operator[]( size_t index ) const
		{
			dbAssertMessage( index < m_size, "index (%zu) out of bounds (%zu)", index, m_size );
			return m_data[ index ];
		}

		size_t size() const { return m_size; }

		Byte* data() { return m_data.get(); }
		const Byte* data() const { return m_data.get(); }

		Byte* begin() { return data(); }
		const Byte* begin() const { return cbegin(); }
		const Byte* cbegin() const { return data(); }

		Byte* end() { return data() + m_size; }
		const Byte* end() const { return cend(); }
		const Byte* cend() const { return data() + m_size; }

	private:

		std::unique_ptr<Byte[]> m_data;
		size_t m_size = 0;
	};

}

#endif