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

    float m_Exposure = 1.0f, m_SphereRadius = 0.5f;
    glm::vec3 m_SphereAlbedo = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 m_SphereCenter = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 m_LightDir = glm::vec3(-1.0f, -1.0f, -1.0f);

    bool m_Render = false;
    float lastTime, deltaTime, currentTime;
    uint32_t vao, vbo;

private:
    void WriteToPngFile(std::string fileName, unsigned char* pixels, int width, int height);
};