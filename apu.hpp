#ifndef APU_HPP
#define APU_HPP

#include "common.hpp"

class APU {
public:
	APU();
	~APU();
	Byte readByte(unsigned int index);
	void writeByte(unsigned int index, Byte value);

private:
	enum Register {
		PULSE_1 = 0x00, // 4 bytes
		PULSE_2 = 0x04, // 4 bytes
		TRIANLGE = 0x08, // 4 bytes
		NOISE = 0x0c, // 4 bytes
		DMC = 0x10, // 4 bytes
		// 14 is OAM DMA transfer
		STATUS = 0x15,
		// 16 is joystick 1
		FRAME_COUNTER = 0x17,

		NUM_REGISTERS
	};

	/*
	enum PulseMask {
		DUTY = 0xc0,
		LEN_COUNTER_HALT = 0x20,
		CONST_VOLUME = 0x10,
		VOLUME = 0x0f,

		SWEEP_ENABLE = 0x80,
		PERIOD = 0x70,
		NEGATE = 0x08,
		SHIFT = 0x07,

		TIMER_LOW = 0xff,

		LENGTH_COUNTER = 0xf8,
		TIMER_HIGH = 0x07
	};

	enum TriangleMask {
		LEN_COUNTER_HALT = 0x80,
		LINEAR_COUNTER = 0x7f,
		TIMER_LOW = 0xff,
		LENGTH_COUNTER = 0xf8,
		TIMER_HIGH = 0x07
	};

	enum NoiseMask {
		LEN_COUNTER_HALT = 0x20,
		CONST_VOLUME = 0x10,
		VOLUME = 0x0f,
		LOOP_NOISE = 0x80,
		PERIOD = 0x0f,
		LENGTH_COUNTER = 0xf8
	};

	enum DMCMask {
		IRQ_ENABLE = 0x80,
		LOOP = 0x40,
		FREQUENCY = 0x0f,
		LOAD_COUNTER = 0x7f,
		SAMPLE_ADDRESS = 0xff,
		SAMPLE_LENGTH = 0xff
	};
	*/

	Byte registers[NUM_REGISTERS];

	static const Byte length_table[32];
};

#endif