#include "Window.h"
#include "Input.h"

#include <Utils/Logger.h>
#include <Utils/Utils.h>

#include <thread>
#include <cassert>

#include <GLFW/glfw3.h>

using namespace std::chrono_literals;

void Window::Init(WindowInfo info)
{
	assert(info.width && info.height && "The width and the height can't be 0!");

	m_Info = info;
	m_Title = info.title;

	if (!glfwInit())
	{
		FATAL("Failed to init the Windowing API!")
	}

#if !defined(NDEBUG) || defined(_DEBUG)
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true); 
#endif

	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWmonitor* monitor = nullptr;
	const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

	if(info.fullscreen)
	{
		monitor = glfwGetPrimaryMonitor();

		info.width = mode->width;
		info.height = mode->height;
		m_Info = info;
	}

	m_Handle = glfwCreateWindow(info.width, info.height, m_Title.c_str(), monitor, nullptr);

	if (!m_Handle)
		FATAL("Failed to create the window!")

	glfwMakeContextCurrent(m_Handle);
	glfwSetWindowUserPointer(m_Handle, this);
	glfwSetFramebufferSizeCallback(m_Handle, FramebuffSizeCallback);
	glfwSetWindowPosCallback(m_Handle, WindowPosCallback);
	
	if((info.xpos == 0 && info.ypos == 0) && !info.fullscreen)
	{
		info.xpos = (mode->width - info.width)/2;
		info.ypos = (mode->height - info.height)/2;
		glfwSetWindowPos(m_Handle, info.xpos, info.ypos);
	}
	else if(info.xpos != 0 || info.ypos != 0)
		glfwSetWindowPos(m_Handle, info.xpos, info.ypos);

	if(!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
		FATAL("Failed to load the OpenGL functions!")

#if !defined(NDEBUG) || defined(_DEBUG)
	int flags;
	glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
	if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
	{
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); 
		glDebugMessageCallback(Utils::EnableGLDebugOutput, nullptr);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
	}
#endif

	glEnable(GL_DEPTH_TEST);
	glViewport(0, 0, info.width, info.height);
}

void Window::ToggleFullscreenMode(int width, int height)
{
	static bool s_Fs = [*this]() {
			return m_Info.fullscreen;
	} ();

	if(Input::IsKeyPressed(*this, GLFW_KEY_F11))
	{
		if(s_Fs)
		{
			ToWindowedMode(width, height);
			std::this_thread::sleep_for(100ms);
		}
		else
		{
			ToFullscreenMode();
			std::this_thread::sleep_for(100ms);
		}
		s_Fs = !s_Fs;
	}
}

void Window::ToFullscreenMode()
{
	const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
	glfwSetWindowMonitor(m_Handle, glfwGetPrimaryMonitor(), m_Info.xpos, m_Info.ypos, mode->width, mode->height, mode->refreshRate);
}

void Window::ToWindowedMode(int w, int h)
{
	const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
	glfwSetWindowMonitor(m_Handle, nullptr, 100, 100, w, h, mode->refreshRate);
}

void Window::Destroy()
{
	glfwDestroyWindow(m_Handle);
}

void Window::Show()
{
	glfwShowWindow(m_Handle);
}

bool Window::IsOpened()
{
	return !glfwWindowShouldClose(m_Handle);
}

void Window::Clear()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void Window::SetClearColor(float red, float green, float blue) 
{
	glClearColor(red, green, blue, 1.0f);
}

void Window::Update()
{
	glfwPollEvents();
	glfwSwapBuffers(m_Handle);
}

void Window::FramebuffSizeCallback(GLFWwindow *window, int width, int height)
{
	Window* win = static_cast<Window*>(glfwGetWindowUserPointer(window));

	win->m_Info.width = width;
	win->m_Info.height = height;

	glViewport(0, 0, width, height);
}

void Window::WindowPosCallback(GLFWwindow *window, int xpos, int ypos)
{
	Window* win = static_cast<Window*>(glfwGetWindowUserPointer(window));

	win->m_Info.xpos = xpos;
	win->m_Info.ypos = ypos;
}
