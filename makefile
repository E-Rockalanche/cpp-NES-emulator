TARGET := test.exe
CXX := g++

ifeq ($(OS), windows)
	LFLAGS := -static -lm -lglut32cu -lglu32 -lopengl32
	CLEAN := del *.o $(TARGET)
else
	LFLAGS := -lm -lGL -lGLU -lglut
	CLEAN := rm *.o $(TARGET)
endif

CFLAGS := -c -Wall -Wextra -std=c++17 -Ofast

MAKE_OBJ = $(CXX) $< -o $@ $(CFLAGS)
MAKE_EXE = $(CXX) $^ -o $@ $(LFLAGS)

$(TARGET):	test_cpu.o cpu.o ppu.o apu.o cartridge.o controller.o mapper1.o
	$(MAKE_EXE)

test_cpu.o:	test_cpu.cpp cpu.hpp ppu.hpp apu.hpp debugging.hpp common.hpp controller.hpp
	$(MAKE_OBJ)

cpu.o:	cpu.cpp cpu.hpp ppu.hpp cartridge.hpp controller.hpp debugging.hpp common.hpp
	$(MAKE_OBJ)

ppu.o:	ppu.cpp ppu.hpp cpu.hpp debugging.hpp common.hpp
	$(MAKE_OBJ)

apu.o:	apu.cpp apu.hpp common.hpp debugging.hpp
	$(MAKE_OBJ)

cartridge.o:	cartridge.cpp cartridge.hpp debugging.hpp common.hpp
	$(MAKE_OBJ)

mapper1.o:	mappers/mapper1.cpp mappers/mapper1.hpp cartridge.hpp debugging.hpp common.hpp
	$(MAKE_OBJ)

controller.o:	controller.cpp controller.hpp debugging.hpp common.hpp
	$(MAKE_OBJ)

clean:
	$(CLEAN)