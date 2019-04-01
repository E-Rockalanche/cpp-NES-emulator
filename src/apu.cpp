#include "Nes_Apu.h"

#include "apu.hpp"
#include "common.hpp"
#include "cpu.hpp"
#include "Sound_Queue.h"

namespace APU {

Nes_Apu apu;
Blip_Buffer buffer;

bool muted;

const int OUT_SIZE = 4096;
blip_sample_t outBuf[OUT_SIZE];

Sound_Queue* sound_queue = NULL;

void mute(bool m) {
    muted = m;
    apu.output(muted ? NULL : &buffer);
}

void newSamples(const blip_sample_t* samples, size_t count) {
    sound_queue->write(samples, count);
}

#define SAMPLE_RATE 48000
#define APU_CLOCK_RATE 1789773

void init() {
    buffer.sample_rate(SAMPLE_RATE);
    buffer.clock_rate(APU_CLOCK_RATE);

    apu.output(&buffer);
    apu.dmc_reader(CPU::readDMC);

    sound_queue = new Sound_Queue;
    sound_queue->init(SAMPLE_RATE);
}

void reset() {
    apu.reset();
    buffer.clear();
}

Byte readByte(int elapsed_cycles, Word address) {
	Byte value = 0;
	if (address == apu.status_addr) {
		value = apu.read_status(elapsed_cycles);
	}
	return value;
}

void writeByte(int elapsed_cycles, Word address, Byte value) {
	apu.write_register(elapsed_cycles, address, value);
}

void runFrame(int elapsed_cycles) {
    apu.end_frame(elapsed_cycles);
    buffer.end_frame(elapsed_cycles);

    if (muted) {
        buffer.clear();
    } else if (buffer.samples_avail() >= OUT_SIZE) {
        newSamples(outBuf, buffer.read_samples(outBuf, OUT_SIZE));
    }
}

} // end namespace