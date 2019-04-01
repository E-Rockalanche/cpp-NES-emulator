TARGET := nes.exe
CXX := g++

SDL_LINK := -Wl,-Bdynamic  -lSDL2main -lSDL2 -lSDL2_ttf -Wl,-Bstatic

LFLAGS := -O3 -lmingw32 -lm -mwindows  -mconsole $(SDL_LINK)
CLEAN := del .\obj\*.o .\obj\mappers\*.o .\lib\obj\*.o $(TARGET)

CFLAGS := -O3 -c -Wall -Wno-unused-parameter -std=c++17
INCLUDE := -I./inc -I./inc/mappers -I./lib/inc -I./lib/inc/boost

SOURCES = $(wildcard src/*.cpp)
OBJECTS = $(patsubst src/%.cpp, obj/%.o, $(SOURCES))

LIB_SOURCES = $(wildcard lib/src/*.cpp)
LIB_OBJECTS = $(patsubst lib/src/%.cpp, lib/obj/%.o, $(LIB_SOURCES))

MAP_SOURCES = $(wildcard src/mappers/*.cpp)
MAP_OBJECTS = $(patsubst src/mappers/%.cpp, obj/mappers/%.o, $(MAP_SOURCES))

MAKE_OBJ = $(CXX) $< -o $@ $(CFLAGS) $(INCLUDE)
MAKE_EXE = $(CXX) $^ -o $@ $(LFLAGS)

$(TARGET): $(LIB_OBJECTS) $(MAP_OBJECTS) $(OBJECTS)
	$(MAKE_EXE)

obj/mappers/%.o: src/mappers/%.cpp inc/mappers/%.hpp inc/cartridge.hpp inc/common.hpp inc/debugging.hpp
	$(MAKE_OBJ)

lib/obj/%.o: lib/src/%.cpp lib/inc/%.h
	$(MAKE_OBJ)

obj/%.o: src/%.cpp inc/%.hpp inc/common.hpp inc/debugging.hpp
	$(MAKE_OBJ)

clean:
	$(CLEAN)