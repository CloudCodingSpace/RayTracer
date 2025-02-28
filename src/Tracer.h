#pragma once

#include "Window/Window.h"
#include "Shader.h"

class Tracer
{
public:
    void Run();

private:
    void Init();
    void Cleanup();

    void Render();
private:
    Window m_Window;
    Shader m_Shader;

    bool m_Render = false;

    glm::vec3 m_CamPos;
    float lastTime, deltaTime, currentTime;

    uint32_t vao, vbo;
};