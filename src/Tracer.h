#pragma once

#include "Window/Window.h"
#include "Rendering/Shader.h"
#include "Rendering/Framebuffer.h"
#include "Rendering/Camera.h"

class Tracer
{
public:
    void Run();

private:
    void Init();
    void Cleanup();

    void Render(int width, int height);
private:
    Window m_Window;
    Shader m_Shader;
    Framebuffer m_Fb;
    Texture m_SkyboxTex;
    Camera m_Camera;

    bool m_Render = false;

    float lastTime, deltaTime, currentTime, m_Exposure = 1.0f;

    uint32_t vao, vbo;

private:
    void WriteToPngFile(std::string fileName, unsigned char* pixels);
};