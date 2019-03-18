#ifndef CPU_HPP
#define CPU_HPP

#include <vector>

#include "Nes_Apu.h"
#include "common.hpp"

namespace CPU {
	void init();
	void reset();
	void power();
	void execute();
	void setNMI(bool on = true);
	void setIRQ(bool on = true);
	bool halted();
	bool breaked();

	void runFrame();
	int readDMC(void*, cpu_addr_t address);

	void dump(Word address);
	void dumpStack();
	void dumpState();

	enum BreakpointCondition {
		PROGRAM_POSITION,
		READ_FROM,
		WRITE_TO,
		NMI,
		IRQ
	};

	void addBreakpoint(Word address, BreakpointCondition condition);
	void clearBreakpoints();

	extern bool _break;
	extern bool debug;
}

#endif