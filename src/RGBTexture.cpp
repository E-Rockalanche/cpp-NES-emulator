#include "RGBTexture.hpp"

#include <stdx/assert.h>

#include <glad/glad.h>

#include <utility>

RGBTexture::RGBTexture( uint width, uint height, const void* data )
{
	dbAssert( width > 0 );
	dbAssert( height > 0 );

	m_width = width;
	m_height = height;

	glGenTextures( 1, &m_id );
	glBindTexture( GL_TEXTURE_2D, m_id );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glBindTexture( GL_TEXTURE_2D, 0 );
}

RGBTexture::RGBTexture( RGBTexture&& other )
{
	*this = std::move( other );
}

RGBTexture::~RGBTexture()
{
	glDeleteTextures( 1, &m_id );
	m_id = 0;
	m_width = 0;
	m_height = 0;
}

RGBTexture& RGBTexture::operator=( RGBTexture&& other )
{
	m_id = std::exchange( other.m_id, 0 );
	m_width = std::exchange( other.m_width, 0 );
	m_height = std::exchange( other.m_height, 0 );
	return *this;
}

void RGBTexture::updateData( const void* data )
{
	glBindTexture( GL_TEXTURE_2D, m_id );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, m_width, m_height, 0, GL_RGB, GL_UNSIGNED_BYTE, data );
	glBindTexture( GL_TEXTURE_2D, 0 );
}

void RGBTexture::render( const Rect& destArea, const Rect& srcArea )
{
	dbAssert( m_width > 0 );
	dbAssert( m_height > 0 );

	glLoadIdentity();
	glViewport( destArea.x, destArea.y, destArea.w, destArea.h );
	glEnable( GL_TEXTURE_2D );
	glBindTexture( GL_TEXTURE_2D, m_id );
	
	float texLeft = srcArea.x / (float)m_width;
	float texRight = ( srcArea.x + srcArea.w ) / (float)m_width;
	float texTop = srcArea.y / (float)m_height;
	float texBottom = ( srcArea.y + srcArea.h ) / (float)m_height;
	
	glBegin( GL_QUADS );
	glTexCoord2f( texLeft, texBottom ); 	glVertex2f( -1, -1 );
	glTexCoord2f( texLeft, texTop ); 		glVertex2f( -1, 1 );
	glTexCoord2f( texRight, texTop ); 		glVertex2f( 1, 1 );
	glTexCoord2f( texRight, texBottom );	glVertex2f( 1, -1 );
	glEnd();

	glBindTexture( GL_TEXTURE_2D, 0 );
}