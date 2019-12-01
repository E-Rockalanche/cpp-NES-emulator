#ifndef NES_MIRRORED_HPP
#define NES_MIRRORED_HPP

namespace nes
{
	struct MemoryRegion
	{
		MemoryRegion() = default;

		constexpr MemoryRegion( size_t start_, size_t size_ )
			: start( start_ )
			, size( size_ )
			, mirrorSize( size_ )
		{}

		constexpr MemoryRegion( size_t start_, size_t size_, size_t mirrorSize_ )
			: start( start_ )
			, size( size_ )
			, mirrorSize( mirrorSize_ )
		{}

		size_t start = 0;
		size_t size = 0;
		size_t mirrorSize = 0;
	};
}

#endif