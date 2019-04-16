#include "2_uxrom.hpp"

UxROM::UxROM(Byte* data) : Cartridge(data) {
	dout("UxROM()");
	
	setPRGBank(0, 0, 16 * KB); // switchable
	setPRGBank(1, -1, 16 * KB); // fixed

	setCHRBank(0, 0, 8 * KB);
}

void UxROM::writePRG(Word address, Byte value) {
	if (address & 0x8000) {
		setPRGBank(0, value & 0x0f, 16 * KB);
	}
}