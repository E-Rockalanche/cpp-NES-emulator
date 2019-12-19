TARGET := bin\nes.exe
CXX := g++

SRC_FOLDERS = src src/mappers

FLAGS = -O3 -DIMGUI_IMPL_OPENGL_LOADER_GLAD -Wall -Wextra

SDL_LFLAGS := -Wl,-Bdynamic -lSDL2main -lSDL2 -lSDL2_ttf -lSDL2_image -Wl,-Bstatic
LFLAGS := -std=c++17 $(FLAGS) -lmingw32 -lm -mwindows -mconsole $(SDL_LFLAGS)
CCFLAGS := -c -std=c++17 $(FLAGS)
LIB_INCLUDE := -isystem ./lib/inc -isystem ./lib/inc/boost -isystem ./glad/include/glad -isystem ../imgui
INCLUDE := -I./inc -I./inc/mappers -I../imgui $(LIB_INCLUDE)

SOURCES = $(wildcard $(patsubst %, %/*.cpp, $(SRC_FOLDERS)))
OBJECTS = $(patsubst src/%.cpp, obj/%.o, $(SOURCES))

LIB_SOURCES = $(wildcard lib/src/*.cpp)
LIB_OBJECTS = $(patsubst lib/src/%.cpp, lib/obj/%.o, $(LIB_SOURCES))

IMGUI_SOURCES = $(wildcard ../imgui/*.cpp)
IMGUI_OBJECTS = $(patsubst ../imgui/%.cpp, obj/imgui/%.o, $(IMGUI_SOURCES))

MAKE_OBJ = $(CXX) $< -o $@ $(CCFLAGS) $(INCLUDE)
MAKE_EXE = $(CXX) $^ -o $@ $(LFLAGS)

$(TARGET): $(LIB_OBJECTS) $(IMGUI_OBJECTS) $(OBJECTS)
	$(MAKE_EXE)

lib/obj/%.o: lib/src/%.cpp lib/inc/%.h
	$(MAKE_OBJ)

obj/imgui/%.o: ../imgui/%.cpp
	$(MAKE_OBJ)

obj/%.o: src/%.cpp inc/%.hpp inc/common.hpp
	$(MAKE_OBJ)

obj/glad.o:	C:/MinGW/src/glad.c
	$(MAKE_OBJ)



test_code/%.o:	test_code/%.cpp
	$(MAKE_OBJ)

imgui_test.exe:	test_code/imgui_test.o $(IMGUI_OBJECTS) obj/glad.o
	$(MAKE_EXE)



clean:
	del .\obj\*.o .\obj\mappers\*.o $(TARGET)

cleanall:
	del .\obj\*.o .\obj\mappers\*.o .\lib\obj\*.o $(TARGET)

run:
	$(TARGET)