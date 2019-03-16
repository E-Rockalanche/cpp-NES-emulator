TARGET := nes.exe
CXX := g++

LFLAGS := -lmingw32 -lm -lglut32cu -lglu32 -lopengl32 -Wl,-Bdynamic -lSDL2main -lSDL2 -Wl,-Bstatic
CLEAN := del .\obj\*.o .\obj\mappers\*.o .\lib\obj\*.o $(TARGET)

CFLAGS := -c -std=c++17 -Ofast -Wno-unused-parameter
INCLUDE := -I./inc -I./inc/mappers -I./lib/inc -I./lib/inc/boost

SOURCES = $(wildcard src/*.cpp)
OBJECTS = $(patsubst src/%.cpp, obj/%.o, $(SOURCES))

LIB_SOURCES = $(wildcard lib/src/*.cpp)
LIB_OBJECTS = $(patsubst lib/src/%.cpp, lib/obj/%.o, $(LIB_SOURCES))

MAP_SOURCES = $(wildcard src/mappers/*.cpp)
MAP_OBJECTS = $(patsubst src/mappers/%.cpp, obj/mappers/%.o, $(MAP_SOURCES))

MAKE_OBJ = $(CXX) $< -o $@ $(CFLAGS) $(INCLUDE)
MAKE_EXE = $(CXX) $^ -o $@ $(LFLAGS)

$(TARGET): $(OBJECTS) $(MAP_OBJECTS) $(LIB_OBJECTS)
	$(MAKE_EXE)

obj/mappers/%.o: src/mappers/%.cpp inc/mappers/%.hpp inc/cartridge.hpp inc/common.hpp inc/debugging.hpp
	$(MAKE_OBJ)

lib/obj/%.o: lib/src/%.cpp lib/inc/%.h
	$(MAKE_OBJ)

obj/%.o: src/%.cpp inc/%.hpp inc/common.hpp inc/debugging.hpp
	$(MAKE_OBJ)

clean:
	$(CLEAN)