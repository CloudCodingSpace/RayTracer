#include "Renderer.h"

#include "Ray.h"
#include "Scene.h"

#include <limits>
#include <chrono>
#include <iostream>

void Renderer::Init(uint32_t width, uint32_t height)
{
    m_Width = width;
    m_Height = height;
    m_Pixels = new uint32_t[width * height];
    m_CamOrigin = glm::vec3(0.0f, 0.0f, 2.5f);
    
    m_Settings.shininess = 64;
    m_Settings.opPhong = true;

    m_Camera.Init(width, height, 90, glm::vec3(0, 0, 3));

    SetupScene();
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
    auto start = std::chrono::high_resolution_clock::now();

    for(int y = 0; y < (int)m_Height; y++)
    {
        for(int x = 0; x < (int)m_Width; x++)
        {
            float u = (float) x/(m_Width - 1);
            float v = (float) y/(m_Height - 1);
            
            glm::vec3 target = m_Camera.CalcTarget(u, v);
            glm::vec3 rayDir = glm::normalize(target - m_Camera.GetOrigin());

            m_Pixels[x + y * m_Width] = Vec4ToUint32(GetPixelColor(rayDir));
        }
    }

    auto stop = std::chrono::high_resolution_clock::now();
    auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
    // Logging the delta time
    {
        printf("\033[0;32m"); // Green Colour
        std::cout << "[RayTracer] INFO: Generation of image took :- " << delta << std::endl;
        printf("\033[0;0m");
    }

    return m_Pixels;
}

void Renderer::Cleanup()
{
    delete[] m_Pixels;
}

void Renderer::SetupScene()
{
	Sphere s1{};
	s1.radius = 0.5f;
	s1.center = { 1.0f, 0.0f, 0.0f };
	s1.mat.albedo = { 1.0f, 0.0f, 0.0f };
    s1.mat.ambient = 0.3f;
    s1.mat.type = MATERIAL_TYPE_DIFFUSE;
	m_Scene.spheres.push_back(s1);

    Sphere s2{};
    s2.radius = 0.5f;
    s2.center = { -1.0f, 0.0f, 0.0f };
    s2.mat.albedo = { 0.2f, 0.3f, 0.3f };
    s2.mat.ambient = 0.3f;
    s2.mat.type = MATERIAL_TYPE_DIFFUSE;
	m_Scene.spheres.push_back(s2);

    LightSource source1{};
    source1.origin = { -1.0f, 0.0f, 1.0f };
    source1.color = { 1.0f, 1.0f, 1.0f };
    source1.intensity = 1.0f;
    m_Scene.lightSources.push_back(source1);

    LightSource source2{};
    source2.color = { 1.0f, 1.0f, 1.0f };
    source2.origin = { 1.0f, 0.0f, 1.0f };
    source2.intensity = 1.0f;
    m_Scene.lightSources.push_back(source2);
}

glm::vec4 Renderer::ProcessBg(Ray& ray)
{
    float x = 0.5f * (glm::normalize(ray.GetDir()).y + 1.0f);
	glm::vec3 col = (1.0f - x) * glm::vec3(1.0f, 1.0f, 1.0f) + x * glm::vec3(0.0f, 0.7f, 1.0f);
	return glm::vec4(col, 1.0f);
}

glm::vec4 Renderer::GetPixelColor(glm::vec3 dir)
{
    Ray camRay(m_CamOrigin, dir);

    Sphere* closestObj = nullptr;
    float t = std::numeric_limits<float>::max();

    for (auto& obj : m_Scene.spheres)
    {
        float t0 = obj.Hit(camRay);
        if (t0 < t && t0 > -1.0f) {
            t = t0;
            closestObj = &obj;
        }
    }

    if (closestObj == nullptr) {
        return ProcessBg(camRay);
    }

    return ProcessMaterial(closestObj, camRay.At(t));
}

glm::vec4 Renderer::ProcessMaterial(Sphere* sphere, glm::vec3 hitPoint)
{
    Material mat = sphere->mat;
    glm::vec3 normal = glm::normalize(hitPoint - sphere->center);

    glm::vec3 finalColor(mat.albedo);
    glm::vec3 lightContribution(0.0f);

    for (const auto& source : m_Scene.lightSources)
    {
        glm::vec3 dir = glm::normalize(source.origin - hitPoint);
        
        if(sphere->mat.type == MATERIAL_TYPE_DIFFUSE)
        {
            float diffuse = 0.0f;
            diffuse = glm::max(glm::dot(normal, dir), 0.0f);
            if(m_Settings.opPhong)
            {
                glm::vec3 viewDir = glm::normalize(m_CamOrigin - hitPoint);
                glm::vec3 reflectDir = glm::reflect(-dir, normal);
                float specular = glm::pow(glm::max(glm::dot(viewDir, reflectDir), 0.0f), m_Settings.shininess);
                diffuse += specular + sphere->mat.ambient;
            }
            
            lightContribution += diffuse * (source.color * source.intensity);
        }
    }

    finalColor *= lightContribution;

    return glm::vec4(finalColor, 1.0f);
}
