TARGET := build\nes.exe
CXX := g++

FLAGS = -O3

SDL_LFLAGS := -Wl,-Bdynamic -lSDL2main -lSDL2 -lSDL2_ttf -lSDL2_image -Wl,-Bstatic
LFLAGS := -std=c++17 $(FLAGS) -lmingw32 -lm -mwindows -mconsole $(SDL_LFLAGS)
CCFLAGS := -c -std=c++17 $(FLAGS)
LIB_INCLUDE := -isystem ./lib/inc -isystem ./lib/inc/boost
INCLUDE := -I./inc -I./inc/mappers $(LIB_INCLUDE)

SOURCES = $(wildcard src/*.cpp)
OBJECTS = $(patsubst src/%.cpp, obj/%.o, $(SOURCES))

LIB_SOURCES = $(wildcard lib/src/*.cpp)
LIB_OBJECTS = $(patsubst lib/src/%.cpp, lib/obj/%.o, $(LIB_SOURCES))

MAP_SOURCES = $(wildcard src/mappers/*.cpp)
MAP_OBJECTS = $(patsubst src/mappers/%.cpp, obj/mappers/%.o, $(MAP_SOURCES))

MAKE_OBJ = $(CXX) $< -o $@ $(CCFLAGS) $(INCLUDE)
MAKE_EXE = $(CXX) $^ -o $@ $(LFLAGS)

$(TARGET): $(LIB_OBJECTS) $(MAP_OBJECTS) $(OBJECTS)
	$(MAKE_EXE)

obj/mappers/%.o: src/mappers/%.cpp inc/mappers/%.hpp inc/cartridge.hpp inc/common.hpp
	$(MAKE_OBJ) -Wall -Wextra -Wno-unused-parameter

lib/obj/%.o: lib/src/%.cpp lib/inc/%.h
	$(MAKE_OBJ)

obj/%.o: src/%.cpp inc/%.hpp inc/common.hpp
	$(MAKE_OBJ) -Wall -Wextra -Wno-unused-parameter

clean:
	del .\obj\*.o .\obj\mappers\*.o $(TARGET)

cleanall:
	del .\obj\*.o .\obj\mappers\*.o .\lib\obj\*.o $(TARGET)

