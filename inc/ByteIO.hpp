#ifndef BYTE_IO_HPP
#define BYTE_IO_HPP

#include "debug.hpp"

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string_view>

namespace ByteIO
{

class Exception : public std::runtime_error
{
	using std::runtime_error::runtime_error;
};

class Writer
{
public:

	Writer( std::ostream& stream ) : m_stream( stream ) {}

	template <typename T>
	std::enable_if_t<!std::is_pointer_v<T>, void>
	write( const T& value )
	{
		m_stream.write( reinterpret_cast<const char*>( std::addressof( value ) ), sizeof( value ) );
	}

	template <typename T, size_t N>
	void write( const T array[ N ] )
	{
		m_stream.write( &array, sizeof( T ) * N );
	}

	template <typename T>
	void write( const T* data, size_t size )
	{
		m_stream.write( reinterpret_cast<const char*>( data ), size );
	}

	void write( const char* str )
	{
		write( std::string_view( str ) );
	}

	void write( std::string_view str )
	{
		write( str.data(), str.size() );
	}

private:

	std::ostream& m_stream;
};

class Reader
{
public:

	Reader( std::istream& stream ) : m_stream( stream ) {}

	template <typename T>
	std::enable_if_t<!std::is_pointer_v<T>, void>
	read( T& value )
	{
		m_stream.read( reinterpret_cast<char*>( std::addressof( value ) ), sizeof( value ) );
	}

	template <typename T, size_t N>
	void read( const T array[ N ] )
	{
		m_stream.read( &array, sizeof( T ) * N );
	}

	template <typename T>
	void read( T* data, size_t size )
	{
		m_stream.read( reinterpret_cast<char*>( data ), size );
	}

	void readHeader( std::string_view str )
	{
		for( char c : str )
		{
			if ( c != m_stream.get() )
			{
				dbBreakMessage( "Incorrect header [%s]", str.data() );
				throw Exception( "Incorrect header" );
			}
		}
	}

private:
	
	std::istream& m_stream;
};

}

#endif