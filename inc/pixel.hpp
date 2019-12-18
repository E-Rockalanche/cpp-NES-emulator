#ifndef PIXEL_HPP
#define PIXEL_HPP

#include <cstdint>

struct Pixel
{
	// little endian
	static constexpr uint32_t r_mask = 0x000000ff;
	static constexpr uint32_t g_mask = 0x0000ff00;
	static constexpr uint32_t b_mask = 0x00ff0000;
	static constexpr uint32_t a_mask = 0x00000000;

	Pixel() = default;

	constexpr Pixel( uint32_t rgb )
		: r( rgb >> 16 )
		, g( rgb >> 8 )
		, b( rgb )
	{}

	constexpr Pixel( uint8_t r_, uint8_t g_, uint8_t b_ )
		: r( r_ )
		, g( g_ )
		, b( b_ )
	{}

	uint8_t* data() { return (uint8_t*)(char*)this; }
	const uint8_t* data() const { return (const uint8_t*)(const char*)this; }
	uint8_t& operator[]( std::size_t index ) { return data()[ index ]; }
	const uint8_t& operator[]( std::size_t index ) const { return data()[ index ]; }

	uint8_t r;
	uint8_t g;
	uint8_t b;
};

#endif