#ifndef NES_TYPES_HPP
#define NES_TYPES_HPP

#include <cstddef>
#include <cstdint>

namespace nes
{
	using Byte = uint8_t;
	using Word = uint16_t;

	constexpr size_t KB = 0x400;
}

#endif