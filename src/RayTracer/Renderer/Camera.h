#pragma once

#define _USE_MATH_DEFINES
#include <cmath>
#include <cstdint>

#include <glm/glm.hpp>

class Camera
{
public:
    inline void Init(uint32_t width, uint32_t height, float fov, glm::vec3 origin)
    {
        m_Fov = fov;
        m_FocalLen = 1.0f;
        m_Origin = origin;

        m_AspectRatio = (float)(width/height);
        float theta = m_Fov * (M_PI / 180.0f);
        m_Height_2 = tan(theta/2) * m_FocalLen;
        m_Width_2 = m_AspectRatio * m_Height_2;

        m_LowerLeftCorner = origin + glm::vec3(-m_Height_2, -m_Width_2, -m_FocalLen);
        m_Horizontal = glm::vec3(2 * m_Width_2, 0, 0);
        m_Vertical = glm::vec3(0, 2 * m_Height_2, 0);
    }

    inline glm::vec3 CalcTarget(float u, float v)
    {
        return m_LowerLeftCorner + u * m_Horizontal + v * m_Vertical;
    }

    inline glm::vec3 GetOrigin()
    {
        return m_Origin;
    }

private:
    glm::vec3 m_Origin, m_Front, m_LowerLeftCorner, m_Horizontal, m_Vertical;
    float m_FocalLen, m_Fov, m_AspectRatio, m_Width_2, m_Height_2;
};