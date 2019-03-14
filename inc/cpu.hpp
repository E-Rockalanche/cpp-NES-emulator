#ifndef CPU_HPP
#define CPU_HPP


#include <vector>
#include "ppu.hpp"
#include "apu.hpp"
#include "common.hpp"
#include "controller.hpp"
#include "cartridge.hpp"

namespace CPU {
	void init();
	void reset();
	void power();
	void execute();
	void setNMI(bool on = true);
	void setIRQ(bool on = true);
	bool halted();
	bool breaked();

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