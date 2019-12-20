#ifndef RGBTEXTURE_HPP
#define RGBTEXTURE_HPP

#include "Rect.hpp"

using uint = unsigned int;

class RGBTexture
{
public:
	RGBTexture( uint width, uint height, const void* data = nullptr );
	RGBTexture( const RGBTexture& ) = delete;
	RGBTexture( RGBTexture&& other );

	~RGBTexture();

	RGBTexture& operator=( RGBTexture&& other );
	RGBTexture& operator=( const RGBTexture& ) = delete;

	void updateData( const void* data );

	void render( const Rect& destArea, const Rect& srcArea );

private:
	uint m_id = 0;
	uint m_width = 0;
	uint m_height = 0;
};

#endif