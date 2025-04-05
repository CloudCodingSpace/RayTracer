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
    Framebuffer m_Fb, m_PrevFrame;
    Texture m_SkyboxTex;
    Camera m_Camera;

    float m_Exposure = 1.0f;
    glm::vec3 m_SkyColor = glm::vec3(0.0f);
    Scene m_Scene;

    bool m_IsSceneHovered = false;
    float lastTime, deltaTime, currentTime;
    uint32_t vao, vbo, m_SceneSSBO, m_MatSSBO;
    int m_Bounces = 10, m_FrameIdx = 1;

private:
    void WriteToPngFile(std::string fileName, unsigned char* pixels, int width, int height);
};
