#ifndef NES_RAM_HPP
#define NES_RAM_HPP

#include "debug.hpp"
#include "types.hpp"

namespace nes
{
	template <size_t Size, bool Mirror = true>
	class Ram
	{
	public:

		static_assert( Size > 0 );

		Byte& operator[]( Word index )
		{
			if constexpr ( Mirror )
				return m_data[ index % Size ];
			else
			{
				dbAssert( index < Size );
				return m_data[ index ];
			}
		}

		Byte operator[]( Word index ) const
		{
			if constexpr ( Mirror )
				return m_data[ index % Size ];
			else
			{
				dbAssert( index < Size );
				return m_data[ index ];
			}
		}

		Byte* data() { return m_data; }
		const Byte* data() const { return m_data; }

		Byte* begin() { return m_data; }
		const Byte* begin() const { return m_data; }
		const Byte* cbegin() const { return m_data; }

		Byte* end() { return m_data + Size; }
		const Byte* end() const { return m_data + Size; }
		const Byte* cend() const { return m_data + Size; }

		size_t size() const { return Size; }

		void fill( Byte value )
		{
			for( Byte& b : m_data )
				b = value;
		}

		Byte* operator&() { return data(); }
		const Byte* operator&() const { return data(); }

	private:

		Byte m_data[ Size ];
	};
}

#endif