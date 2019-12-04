#ifndef HISTORY_HPP
#define HISTORY_HPP

#include <cassert>
#include <cstdio>

class History
{
public:
	static constexpr size_t Size = 256;
	static constexpr size_t StringLength = 64;

	template <typename... Args>
	void add( const char* format, Args... args )
	{
		auto len = std::snprintf( m_history[ m_next ], StringLength, format, std::forward<Args>( args )... );
		assert( len >= 0 );
		m_next = ( m_next + 1 ) % Size;
		m_size += ( m_size < Size );
	}

	void print()
	{
		std::printf( "===== HISTORY =====\n" );
		for( size_t i = 0; i < m_size; ++i )
		{
			auto index = ( m_next - m_size + i ) % Size;
			std::printf( m_history[ index ] );
			std::printf( "\n" );
		}
		std::printf( "===================\n" );
	}

private:
	char m_history[ Size ][ StringLength ];
	size_t m_next = 0;
	size_t m_size = 0;
};

#endif