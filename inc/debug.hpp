#ifndef NES_DEBUG_HPP
#define NES_DEBUG_HPP

#if defined( SHIPPING )

	#define dbAbort()
	#define dbPrintFileLine()
	#define dbLogError( ... )
	#define dbBreak()
	#define dbBreakMessage( ... )
	#define dbAssert( condition )
	#define dbAssertMessage( condition, ... )

#else

	#include <cstdio>
	#include <cstdlib>

	#define dbAbort() { *(int*)nullptr = 0; }

	#define dbPrintFileLine()	\
		std::printf( "\n\tin %s at line %i\n", __FILE__, __LINE__ );

	#define dbLog( ... ) { std::printf( __VA_ARGS__ ); std::printf( "\n" ); }

	#define dbLogByte( varname, value )	\
		std::printf( "%s: 0x%02x", varname, (int)value );

	#define dbLogWord( varname, value )	\
		std::printf( "%s: 0x%04x", varname, (int)value );

	#define dbLogError( ... ) {		\
		std::printf( "Error: " );	\
		std::printf( __VA_ARGS__ );	\
		dbPrintFileLine(); }

	#define dbBreak() {			\
		std::printf( "Break" );	\
		dbPrintFileLine();		\
		dbAbort(); }

	#define dbBreakMessage( ... ) {	\
		std::printf( "Break: " );	\
		std::printf( __VA_ARGS__ );	\
		dbPrintFileLine();			\
		dbAbort(); }

	#define dbAssert( condition )	\
		if ( !( condition ) ) {		\
			dbLogError( "failed assertion: %s", #condition );	\
			dbAbort(); }

	#define dbAssertMessage( condition, ... )	\
		if ( !( condition ) ) {					\
			std::printf( "Error: failed assertion: %s\n\t", #condition );	\
			std::printf( __VA_ARGS__ );	\
			dbPrintFileLine();			\
			dbAbort(); }

#endif

#endif