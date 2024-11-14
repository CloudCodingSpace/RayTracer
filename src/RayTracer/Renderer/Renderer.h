#pragma once

#include <cstdint>
#include <glm/glm.hpp>

#include "Scene.h"
#include "Camera.h"

struct RenderSettings
{
    bool opPhong;
    uint32_t shininess;
};

class Renderer
{
public:
    void Init(uint32_t width, uint32_t height);
    void Cleanup();

    void Resize(uint32_t width, uint32_t height);

    uint32_t* GenImage();

private:
    glm::vec4 GetPixelColor(glm::vec3 dir);

    void SetupScene();
    glm::vec4 ProcessBg(Ray& ray);
    glm::vec4 ProcessMaterial(Sphere* sphere, glm::vec3 hitPoint);

    uint32_t Vec4ToUint32(glm::vec4 vec);
private:
    uint32_t* m_Pixels;
    uint32_t m_Width, m_Height;
    glm::vec3 m_CamOrigin;

    RenderSettings m_Settings;

    Scene m_Scene;
    Camera m_Camera;
};