
#include "movie.hpp"

#include <stdx/assert.h>
#include "globals.hpp"
#include "menu_bar.hpp"

#include <fstream>
#include <iostream>
#include <vector>
#include <string>

namespace Movie
{
	struct ButtonPress
	{
		int frame;
		int joypad;
		nes::Joypad::Button button;
		bool pressed;
	};

	State state = NONE;
	std::vector<ButtonPress> button_presses;
	int index = 0;

	bool empty()
	{
		return button_presses.empty();
	}

	void clear()
	{
		button_presses.clear();
		save_movie_button.disable();
		play_movie_button.disable();

		auto cartridge = s_nes.getCartridge();
		load_movie_button.enable( cartridge != nullptr );
		record_movie_button.enable( cartridge != nullptr );
		state = NONE;
		index = 0;
	}

	bool save( std::string filename )
	{
		if ( isRecording() )
		{
			stopRecording();
		}

		std::ofstream fout( filename.c_str(), std::ios::binary );
		if ( !fout.is_open() )
		{
			dbLogError( "cannot save move to %s", filename.c_str() );
			return false;
		}

		unsigned int checksum = s_nes.getCartridge()->getChecksum();
		writeBinary(fout, checksum);

		int size = (int)button_presses.size();
		fout.write( ( char* )&size, sizeof( size ) );

		for ( int i = 0; i < size; i++ )
		{
			ButtonPress press = button_presses[i];
			fout.write( ( char* )&press, sizeof( press ) );
		}

		fout.close();
		return true;
	}

	bool load( std::string filename )
	{
		stopRecording();
		stopPlayback();

		std::ifstream fin( filename.c_str(), std::ios::binary );
		if ( !fin.is_open() )
		{
			dbLogError( "cannot open move from %s", filename.c_str() );
			return false;
		}

		button_presses.clear();

		int size;
		fin.read( ( char* )&size, sizeof( size ) );

		for ( int i = 0; i < size; i++ )
		{
			ButtonPress press;
			fin.read( ( char* )&press, sizeof( press ) );
			button_presses.push_back( press );
		}

		save_movie_button.enable( !empty() );
		fin.close();

		return true;
	}

	State getState()
	{
		return state;
	}

	void startRecording()
	{
		if ( state == NONE )
		{
			button_presses.clear();
			state = RECORDING;

			save_movie_button.disable();
			play_movie_button.disable();
			record_movie_button.check();
		}
	}

	void stopRecording()
	{
		if ( isRecording() )
		{
			state = NONE;

			save_movie_button.enable();
			play_movie_button.enable();
			record_movie_button.uncheck();
		}
	}

	bool isRecording()
	{
		return state == RECORDING;
	}

	void recordButtonState( int frame, int joypad, nes::Joypad::Button button, bool pressed )
	{
		dbAssertMessage( state == RECORDING, "cannot record button presses while not recording" );
		button_presses.push_back( { frame, joypad, button, pressed } );
	}

	void startPlayback()
	{
		if ( state == NONE && !empty() )
		{
			index = 0;
			state = PLAYING;

			play_movie_button.check();
			record_movie_button.disable();
		}
	}

	void stopPlayback()
	{
		if ( isPlaying() )
		{
			state = NONE;

			play_movie_button.uncheck();
			record_movie_button.enable();
		}
	}

	bool isPlaying()
	{
		return state == PLAYING;
	}

	void updateInput( int frame )
	{
		while ( index < ( int )button_presses.size() )
		{
			const ButtonPress& press = button_presses[index];
			if ( press.frame < frame )
			{
				index++;
			}
			else if ( press.frame == frame )
			{
				joypad[press.joypad].setButtonState( press.button, press.pressed );
				index++;
			}
			else
			{
				break;
			}
		}

		if ( index >= ( int )button_presses.size() )
		{
			// stop playback
			state = NONE;
		}
	}
}