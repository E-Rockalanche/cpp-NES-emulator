#include "nes.hpp"
#include "debugging.hpp"

NES::NES() {
	good = false;
	cpu.setPPU(&ppu);
	cpu.setAPU(&apu);
	ppu.setCPU(&cpu);
}

bool NES::loadFile(const char* filename) {
	if (cartridge.loadFile(filename)) {
		cpu.setROM(cartridge.getPrgRom(), cartridge.prgRomSize());
		ppu.setROM(cartridge.getChrRom(), cartridge.chrRomSize());
		good = true;
	} else {
		dout("NES could not load " << filename);
		good = false;
	}
	return good;
}

bool NES::reset() {
	if (good) {
		cpu.reset();
		ppu.reset();
		return true;
	}
	return false;
}

bool NES::execute() {
	if (good) {
		cpu._break = false;
		int frame = 0;
		while(!cpu.halted() && !cpu.breaked() && !cpu._break) {
			cpu.clockTick();

			ppu.clockTick();
			ppu.clockTick();
			ppu.clockTick();

			if (ppu.readyToDraw()) {
				// dout("frame " << frame++);
			}
		}
		return true;
	}
	return false;
}