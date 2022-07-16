#pragma once

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include "scene.h"

namespace GUI {
	extern GLFWwindow* window;
	extern bool shouldQuit;
	extern bool animationRenderWindowVisible;

	void init(GLFWwindow* window);
	void cleanup();

	void render();
};