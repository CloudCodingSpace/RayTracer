#pragma once

#include "Window/Window.h"
#include "Rendering/Shader.h"
#include "Rendering/Framebuffer.h"
#include "Rendering/Camera.h"

#include "Scene.h"

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

    float m_Exposure = 1.0f;
    glm::vec3 m_LightDir = glm::vec3(-1.0f);
    Scene m_Scene;

    bool m_Render = false, m_IsMouseHovered = false;
    float lastTime, deltaTime, currentTime;
    uint32_t vao, vbo, ssbo;

private:
    void WriteToPngFile(std::string fileName, unsigned char* pixels, int width, int height);
};