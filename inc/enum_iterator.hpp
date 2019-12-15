#ifndef ENUM_ITERATOR_HPP
#define ENUM_ITERATOR_HPP

#include "debug.hpp"
#include <type_traits>

template <typename T>
struct is_bitset_enum
{
	static_assert( std::is_enum_v<T> );

	using Yes = char;
	using No = int;

	template<typename C> static Yes test( decltype( C::is_bitset_enum ) );
	template<typename C> static No test( ... );

	static constexpr bool value = sizeof(test<T>(T{})) == sizeof(Yes);
};
template <typename T>
constexpr bool is_bitset_enum_v = is_bitset_enum<T>::value;

template <typename T>
constexpr auto to_underlying( T value )
{
	static_assert( std::is_enum_v<T> );
	return static_cast< std::underlying_type_t<T> >( value );
}

template <typename T>
constexpr auto to_flag( T value )
{
	static_assert( std::is_enum_v<T> );

	if constexpr ( is_bitset_enum_v<T> )
		return to_underlying( value );
	else
		return 1 << to_underlying( value );
}

template <typename T>
class enum_iterator
{
public:
	static_assert( std::is_enum_v<T> );

	using underlying_type = std::underlying_type_t<T>;
	using value_type = T;
	using size_type = size_t;
	using difference_type = int;

	constexpr enum_iterator() = default;
	constexpr enum_iterator( T value ) : m_value( to_underlying( value ) ) {}

	constexpr T value() const { return static_cast<T>( m_value ); }
	constexpr operator T() const { return value(); }
	constexpr T operator*() const { return static_cast<const T&>( m_value ); }
	constexpr T operator[]( size_type index ) const { return static_cast<T>( m_value + index ); }

	constexpr enum_iterator& operator=( const enum_iterator& ) = default;
	constexpr enum_iterator& operator=( T value ) { m_value = to_underlying( value ); }

	constexpr enum_iterator& operator++() { ++m_value; return *this; }
	constexpr enum_iterator& operator--() { --m_value; return *this; }
	constexpr enum_iterator operator++( int ) { return enum_iterator( m_value++ ); }
	constexpr enum_iterator operator--( int ) { return enum_iterator( m_value-- ); }

	constexpr bool operator==( const enum_iterator& other ) const { return m_value == other.m_value; }
	constexpr bool operator!=( const enum_iterator& other ) const { return m_value != other.m_value; }
	constexpr bool operator>( const enum_iterator& other ) const { return m_value > other.m_value; }
	constexpr bool operator<( const enum_iterator& other ) const { return m_value < other.m_value; }
	constexpr bool operator>=( const enum_iterator& other ) const { return m_value >= other.m_value; }
	constexpr bool operator<=( const enum_iterator& other ) const { return m_value <= other.m_value; }

	friend constexpr enum_iterator operator+( const enum_iterator& it, difference_type num ) { return enum_iterator( it.m_value + num ); }
	friend constexpr enum_iterator operator+( difference_type num, const enum_iterator& it ) { return enum_iterator( it.m_value + num ); }
	friend constexpr enum_iterator operator-( const enum_iterator& it, difference_type num ) { return enum_iterator( it.m_value - num ); }
	friend constexpr difference_type operator-( const enum_iterator& lhs, const enum_iterator& rhs ) { return lhs.m_value - rhs.m_value; }

	constexpr enum_iterator& operator+=( difference_type num ) { m_value += num; return *this; }
	constexpr enum_iterator& operator-=( difference_type num ) { m_value -= num; return *this; }

private:
	constexpr enum_iterator( underlying_type value ) : m_value( value ) {}

private:
	underlying_type m_value = 0;
};

template <typename T>
constexpr enum_iterator<T> enum_begin()
{
	return enum_iterator<T>( static_cast<T>( 0 ) );
}

// T enum_back( T ) must be defined for each enum
template <typename T>
constexpr enum_iterator<T> enum_end()
{
	return enum_iterator<T>( enum_back( T{} ) ) + 1;
}

template <typename T>
constexpr size_t to_index( T value )
{
	enum_iterator<T> it( value );
	dbAssert( it < enum_end<T>() );
	return static_cast<size_t>( it - enum_begin<T>() );
}

template <typename T>
constexpr size_t enum_size() { return static_cast<size_t>( enum_end<T>() - enum_begin<T>() ); }

template <typename T>
constexpr size_t enum_size_v = enum_size<T>();

template <typename T>
class enum_sequence
{
public:
	using iterator = enum_iterator<T>;
	using value_type = T;
	using size_type = size_t;

	constexpr iterator begin() const { return enum_begin<T>(); }
	constexpr iterator end() const { return enum_end<T>(); }

	constexpr size_type size() const { return end() - begin(); }

	constexpr T operator[]( size_type index ) const { return begin()[ index ]; }

	constexpr T front() const { return *begin(); }
	constexpr T back() const { return *--end(); }

	constexpr bool empty() const { return size() == 0; }
};

#endif