#include "Renderer.h"

#include "Ray.h"
#include "Scene.h"

#include <limits>

void Renderer::Init(uint32_t width, uint32_t height)
{
    m_Width = width;
    m_Height = height;
    m_Pixels = new uint32_t[width * height];
    m_CamOrigin = glm::vec3(0.0f, 0.0f, 2.5f);

    SetupScene();
}

void Renderer::Cleanup()
{
    delete[] m_Pixels;
}

void Renderer::SetupScene()
{
	Sphere s1{};
	s1.radius = 0.5f;
	s1.albedo = { 1.0f, 0.0f, 0.0f };
	s1.center = { 1.0f, 0.0f, -1.0f };
	m_Scene.spheres.push_back(s1);

    Sphere s2{};
    s2.radius = 0.5f;
    s2.center = { 0.0f, 0.0f, 0.0f };
    s2.albedo = { 0.2f, 0.3f, 0.3f };
	m_Scene.spheres.push_back(s2);
}

void Renderer::Resize(uint32_t width, uint32_t height)
{
    m_Width = width;
    m_Height = height;
    if(m_Pixels)
    {
        delete[] m_Pixels;        
    }
    m_Pixels = new uint32_t[width * height];
}


uint32_t Renderer::Vec4ToUint32(glm::vec4 vec)
{
    vec = glm::clamp(vec, glm::vec4(0), glm::vec4(1));

    uint8_t r = (uint8_t)(vec.r * 255.0f);
    uint8_t g = (uint8_t)(vec.g * 255.0f);
    uint8_t b = (uint8_t)(vec.b * 255.0f);
    uint8_t a = (uint8_t)(vec.a * 255.0f);

    uint32_t result = (a << 24) | (b << 16) | (g << 8) | r;
    return result;
}

uint32_t* Renderer::GenImage()
{
    for(int y = (int)(m_Height - 1); y >= 0; y--)
    {
        for(int x = 0; x < (int)m_Width; x++)
        {
            glm::vec2 coord = { (float)x / (float)m_Width, (float)y / (float)m_Height};
			coord = coord * 2.0f - 1.0f;
            coord.x *= (float)m_Width / (float)m_Height;
            m_Pixels[x + (m_Height - (y + 1)) * m_Width] = Vec4ToUint32(GetPixelColor(coord));
        }
    }

    return m_Pixels;
}

glm::vec4 Renderer::GetPixelColor(glm::vec2 coord)
{
    Ray camRay(m_CamOrigin, glm::vec3(coord, -1.0f));

    Sphere* closestObj = nullptr;
    float t = std::numeric_limits<float>::max();

    for (auto& obj : m_Scene.spheres)
    {
        float t0 = obj.Hit(camRay);
        if(t0 > -1.0f) {
            if (t0 < t) {
                t = t0;
                closestObj = &obj;
            }
        }
    }

    if (closestObj == nullptr) {
        goto bg;
    }

    return glm::vec4(closestObj->albedo, 1.0f);

bg:
	float x = 0.5f * (glm::normalize(camRay.GetDir()).y + 1.0f);
	glm::vec3 col = (1.0f - x) * glm::vec3(1.0f, 1.0f, 1.0f) + x * glm::vec3(0.0f, 0.7f, 1.0f);
	return glm::vec4(col, 1.0f);
}