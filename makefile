TARGET := nes.exe
CXX := g++

ifeq ($(OS), unix)
	LFLAGS := -lm -lGL -lGLU -lglut
	CLEAN := rm ./obj/*.o ./obj/mappers/*.o $(TARGET)
else
	LFLAGS := -static -lm -lglut32cu -lglu32 -lopengl32
	CLEAN := del .\obj\*.o .\obj\mappers\*.o $(TARGET)
endif

CFLAGS := -c -std=c++17 -Ofast -Wall -Wextra -Wno-unused-parameter
INCLUDE := -I./inc -I./inc/mappers

SOURCES = $(wildcard src/*.cpp)
OBJECTS = $(patsubst src/%.cpp, obj/%.o, $(SOURCES))

MAPPERS_SRC = $(wildcard src/mappers/*.cpp)
MAPPERS_OBJ = $(patsubst src/mappers/%.cpp, obj/mappers/%.o, $(MAPPERS_SRC))

MAKE_OBJ = $(CXX) $< -o $@ $(CFLAGS) $(INCLUDE)
MAKE_EXE = $(CXX) $^ -o $@ $(LFLAGS)

$(TARGET): $(OBJECTS) $(MAPPERS_OBJ)
	$(MAKE_EXE)

obj/mappers/%.o: src/mappers/%.cpp inc/mappers/%.hpp inc/cartridge.hpp inc/common.hpp inc/debugging.hpp
	$(MAKE_OBJ)

obj/%.o: src/%.cpp inc/%.hpp inc/common.hpp inc/debugging.hpp
	$(MAKE_OBJ)

clean:
	$(CLEAN)