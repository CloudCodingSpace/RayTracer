#pragma once

#include "Window/Window.h"

#include <glm/glm.hpp>

class Camera
{
public:
    void Update(Window& window);

    inline glm::vec3& GetPos() { return m_CamPos; }
    inline glm::vec3& GetFront() { return m_CamFront; }
    inline bool IsFirstMouseUsage() { return firstMouse; }
private:
    glm::vec3 m_CamPos = glm::vec3(0, 0, 2);
    glm::vec3 m_CamFront = glm::vec3(0.0f, 0.0f, -1.0f);
    float m_Yaw = -90.0f; 
    float m_Pitch = 0.0f; 
    float m_Sensitivity = 0.1f; 
    double lastMouseX = 400, lastMouseY = 300;
    bool firstMouse = true;
};