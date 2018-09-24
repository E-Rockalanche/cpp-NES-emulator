PROGRAM=test.exe

CXX = g++
CFLAGS = -Wall -Wextra -std=c++11 -c
LFLAGS = -std=c++11 -static
MAKE_OBJ = $(CXX) $(CFLAGS) $< -o $@

$(PROGRAM):	test_cpu.o cpu.o ppu.o apu.o cartridge.o nes.o
	$(CXX) $(LFLAGS) $^ -o $@

test_cpu.o:	test_cpu.cpp nes.hpp cpu.hpp common.hpp
	$(MAKE_OBJ)

nes.o:	nes.cpp nes.hpp cpu.hpp ppu.hpp apu.hpp debugging.hpp
	$(MAKE_OBJ)

cpu.o:	cpu.cpp cpu.hpp debugging.hpp common.hpp
	$(MAKE_OBJ)

ppu.o:	ppu.cpp ppu.hpp debugging.hpp cpu.hpp common.hpp
	$(MAKE_OBJ)

apu.o:	apu.cpp apu.hpp common.hpp debugging.hpp
	$(MAKE_OBJ)

cartridge.o:	cartridge.cpp cartridge.hpp debugging.hpp common.hpp
	$(MAKE_OBJ)

clean:
	del *.o