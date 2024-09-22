#pragma once

#include <cstdint>
#include <glm/glm.hpp>

class Renderer
{
public:
    void Init(uint32_t width, uint32_t height);
    void Cleanup();

    void Resize(uint32_t width, uint32_t height);

    uint32_t* GenImage();

private:
    glm::vec4 GetPixelColor(glm::vec2 coord);
    uint32_t Vec4ToUint32(glm::vec4 vec);


private:
    uint32_t* m_Pixels;
    uint32_t m_Width, m_Height;
};