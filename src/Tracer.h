#pragma once

#include "Window/Window.h"
#include "Shader.h"
#include "Framebuffer.h"

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

    bool m_Render = false;

    glm::vec3 m_CamPos;
    float lastTime, deltaTime, currentTime;

    uint32_t vao, vbo;

private:
    void WriteToPngFile(std::string fileName, unsigned char* pixels);
};