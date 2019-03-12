TARGET := nes.exe
CXX := g++

ifeq ($(OS), unix)
	LFLAGS := -lm -lGL -lGLU -lglut
	CLEAN := rm *.o $(TARGET)
else
	LFLAGS := -static -lm -lglut32cu -lglu32 -lopengl32
	CLEAN := del *.o $(TARGET)
endif

CFLAGS := -c -std=c++17 -Ofast

MAKE_OBJ = $(CXX) $< -o $@ $(CFLAGS)
MAKE_EXE = $(CXX) $^ -o $@ $(LFLAGS)

$(TARGET):	main.o cpu.o ppu.o apu.o cartridge.o mapper1.o joypad.o zapper.o
	$(MAKE_EXE)

main.o: main.cpp cpu.hpp ppu.hpp apu.hpp debugging.hpp common.hpp controller.hpp
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

joypad.o:	joypad.cpp joypad.hpp controller.hpp common.hpp
	$(MAKE_OBJ)

zapper.o:	zapper.cpp zapper.hpp controller.hpp common.hpp
	$(MAKE_OBJ)

clean:
	$(CLEAN)