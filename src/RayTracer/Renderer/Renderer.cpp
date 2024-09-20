#include "Renderer.h"

void Renderer::Init(uint32_t width, uint32_t height)
{
    m_Width = width;
    m_Height = height;
    m_Pixels = new uint32_t[width * height];
}

void Renderer::Cleanup()
{
    delete[] m_Pixels;
}

void Renderer::Resize(uint32_t width, uint32_t height)
{
    m_Width = width;
    m_Height = height;
    if(m_Pixels)
    {
        delete[] m_Pixels;        
        m_Pixels = new uint32_t[width * height];
    }
}

uint32_t* Renderer::GenImage()
{
    for(int y = 0; y < m_Height; y++)
    {
        for(int x = 0; x < m_Height; x++)
        {
            glm::vec2 coord = { (float)x / (float)m_Width, (float)y / (float)m_Height };
			coord = coord * 2.0f - 1.0f; // -1 -> 1
            m_Pixels[x + y * m_Width] = Vec4ToUint32(GetPixelColor(coord.x, coord.y));
        }
    }

    return m_Pixels;
}

glm::vec4 Renderer::GetPixelColor(int x, int y)
{
    return glm::vec4(0, 1, 0, 1);
}

uint32_t Renderer::Vec4ToUint32(glm::vec4 vec)
{
    uint8_t r = (uint8_t)(vec.r * 255.0f);
    uint8_t g = (uint8_t)(vec.g * 255.0f);
    uint8_t b = (uint8_t)(vec.b * 255.0f);
    uint8_t a = (uint8_t)(vec.a * 255.0f);

    uint32_t result = (a << 24) | (b << 16) | (g << 8) | r;
    return result;
}