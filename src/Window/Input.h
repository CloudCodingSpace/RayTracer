#pragma once

#include "Window.h"
#include <iostream>

#include <glm/glm.hpp>

class Input
{
public:
	static inline bool IsKeyPressed(Window& window, int key)
	{
		return glfwGetKey(window.GetHandle(), key) == GLFW_PRESS;
	}

	static inline bool IsKeyReleased(Window& window, int key)
	{
		return glfwGetKey(window.GetHandle(), key) == GLFW_RELEASE;
	}

	static inline bool IsMBPressed(Window& window, int button)
	{
		return glfwGetMouseButton(window.GetHandle(), button) == GLFW_PRESS;
	}
	
	static inline bool IsMBReleased(Window& window, int button)
	{
		return glfwGetMouseButton(window.GetHandle(), button) == GLFW_RELEASE;
	}

	static inline void SetMousePos(Window& window, double x, double y)
	{
		glfwSetCursorPos(window.GetHandle(), x, y);
	}

	static inline glm::dvec2 GetMousePos(Window& window)
	{
		glm::dvec2 vec(0.0);
		glfwGetCursorPos(window.GetHandle(), &vec.x, &vec.y);
		return vec;
	}

	static inline void DisableMouse(Window& window)
	{
		glfwSetInputMode(window.GetHandle(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}
	
	static inline void HideMouse(Window& window)
	{
		glfwSetInputMode(window.GetHandle(), GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
	}

	static inline void ShowMouse(Window& window)
	{
		glfwSetInputMode(window.GetHandle(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}

};