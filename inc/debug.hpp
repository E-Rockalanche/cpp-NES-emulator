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

	#include "Logger.hpp"

	#include <cstdlib>

	#define dbAbort() { *(int*)nullptr = 0; }

	#define dbPrintFileLine()	\
		logger.log( "\n\tin %s at line %i\n", __FILE__, __LINE__ );

	#define dbLog( ... ) { logger.log( __VA_ARGS__ ); logger.log( "\n" ); }

	#define dbLogByte( varname, value )	\
		logger.log( "%s: 0x%02x", varname, (int)value );

	#define dbLogWord( varname, value )	\
		logger.log( "%s: 0x%04x", varname, (int)value );

	#define dbLogError( ... ) {		\
		logger.log( "Error: " );	\
		logger.log( __VA_ARGS__ );	\
		dbPrintFileLine(); }

	#define dbBreak() {			\
		logger.log( "Break" );	\
		dbPrintFileLine();		\
		dbAbort(); }

	#define dbBreakMessage( ... ) {	\
		logger.log( "Break: " );	\
		logger.log( __VA_ARGS__ );	\
		dbPrintFileLine();			\
		dbAbort(); }

	#define dbAssert( condition )	\
		if ( !( condition ) ) {		\
			dbLogError( "failed assertion: %s", #condition );	\
			dbAbort(); }

	#define dbAssertMessage( condition, ... )	\
		if ( !( condition ) ) {					\
			logger.log( "Error: failed assertion: %s\n\t", #condition );	\
			logger.log( __VA_ARGS__ );	\
			dbPrintFileLine();			\
			dbAbort(); }

#endif

#endif