#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <string>

struct WindowInfo
{
	bool fullscreen = false;
	int width, height, xpos, ypos;
	std::string title;
};

class Window
{
public:
	void Init(WindowInfo info);
	void Destroy();

	void Show();
	bool IsOpened();
	void Clear();
	void SetClearColor(float red, float green, float blue);
	void Update();
	void ToggleFullscreenMode(int width, int height);

	inline const WindowInfo& GetWindowInfo() { return m_Info; }
	inline std::string GetTitle() { return m_Title; }
	inline GLFWwindow* GetHandle() { return m_Handle; }

	inline void SetTitle(std::string title) { glfwSetWindowTitle(m_Handle, title.c_str()); }
private:
	GLFWwindow* m_Handle;
	std::string m_Title;
	WindowInfo m_Info{};

private:
	void ToFullscreenMode();
	void ToWindowedMode(int w, int h);

private:
	static void FramebuffSizeCallback(GLFWwindow* window, int width, int height);
	static void WindowPosCallback(GLFWwindow* window, int xpos, int ypos);
};
