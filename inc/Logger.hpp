#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <cassert>
#include <cstdio>
#include <cstring>
#include <string_view>

class Logger
{
public:
	static constexpr size_t BufferSize = 2048;
	static constexpr size_t Capacity = BufferSize - 1;

	void log( std::string_view str )
	{
		size_t len = str.length();
		assert( len <= Capacity );

		if ( len + m_size > Capacity )
		{
			std::printf( m_buffer );
			m_size = 0;
		}

		std::memcpy( m_buffer + m_size, str.data(), len );
		m_size += len;
		m_buffer[ m_size ] = '\0';
	}

	template <typename... Args>
	void log( const char* format, Args... args )
	{
		char str[ BufferSize ];
		int len = std::snprintf( str, BufferSize, format, std::forward<Args>( args )... );
		if ( len > 0 )
			log( std::string_view( str, len ) );
	}

	void flush()
	{
		std::printf( m_buffer );
		m_size = 0;
	}

private:
	char m_buffer[ BufferSize ];
	size_t m_size = 0;
};

extern Logger logger;

#endif