#pragma once

#include <glm/glm.hpp>

class Ray
{
public:
    Ray(glm::vec3 origin, glm::vec3 dir) {
        m_Origin = origin;
        m_Dir = dir;
    }

    inline glm::vec3 GetOrigin() {
        return m_Origin;
    }

    inline glm::vec3 GetDir() {
        return m_Dir;
    }

    inline void SetOrigin(glm::vec3& origin) {
        m_Origin = origin;
    }

    inline void SetDir(glm::vec3& dir) {
        m_Dir = dir;
    }

    inline glm::vec3 At(float t) {
        return m_Origin + m_Dir * t;
    }

private:
    glm::vec3 m_Origin, m_Dir;
};