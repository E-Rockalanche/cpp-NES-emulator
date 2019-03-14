#include "apu.hpp"
#include "common.hpp"

namespace APU {

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

const Byte length_table[32] = {
	0x0a, 0xfe, 0x14, 0x02, 0x28, 0x04, 0x50, 0x06,
	0xa0, 0x08, 0x3c, 0x0a, 0x0e, 0x0c, 0x1a, 0x0e, 
	0x0c, 0x10, 0x18, 0x12, 0x30, 0x14, 0x60, 0x16,
	0xc0, 0x18, 0x48, 0x1a, 0x10, 0x1c, 0x20, 0x1e
};

Byte readByte(unsigned int index) {
	assert(index < NUM_REGISTERS, "APU register index " << index << " out of bounds " << NUM_REGISTERS);
	assert(index != 0x14, "cannot read from APU at $4014");

	// dout("APU read " << toHex(registers[index]) << " from register " << index);

	return registers[index];
}

void writeByte(unsigned int index, Byte value) {
	assert(index < NUM_REGISTERS, "APU register index " << index << " out of bounds " << NUM_REGISTERS);
	assert(index != 0x14, "cannot write to APU at $4014");

	// dout("APU write " << toHex(value) << " to register " << index);
	
	registers[index] = value;
}

} // end namespace