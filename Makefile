CPP := g++
SOURCES := src/*.cpp src/imgui/*.cpp
INCLUDES := -Isrc/imgui/ -Isrc/ -IDependencies/include/
DEPENDENCIES := $(shell pkg-config --libs glew glfw3)
NAME := "opengl-raytracing"

build:
	$(CPP) $(SOURCES) $(INCLUDES) $(DEPENDENCIES) -o $(NAME)

cleanup:
	rm -rf opengl-raytracing