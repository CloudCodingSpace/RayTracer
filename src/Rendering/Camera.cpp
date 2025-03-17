#include "Camera.h"

#include "Window/Input.h"

void Camera::Update(Window& m_Window)
{
    if(Input::IsMBPressed(m_Window, GLFW_MOUSE_BUTTON_RIGHT))
    {
        double mouseX, mouseY;
        glfwGetCursorPos(m_Window.GetHandle(), &mouseX, &mouseY);

        if (firstMouse) {
            glfwSetInputMode(m_Window.GetHandle(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            lastMouseX = mouseX;
            lastMouseY = mouseY;
            firstMouse = false;
        }

        float offsetX = (lastMouseX - mouseX) * m_Sensitivity;
        float offsetY = (lastMouseY - mouseY) * m_Sensitivity;

        lastMouseX = mouseX;
        lastMouseY = mouseY;

        m_Yaw += offsetX;
        m_Pitch += offsetY;

        if (m_Pitch > 89.0f) m_Pitch = 89.0f;
        if (m_Pitch < -89.0f) m_Pitch = -89.0f;

        glm::vec3 front;
        front.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
        front.y = sin(glm::radians(m_Pitch));
        front.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
        m_CamFront = glm::normalize(front);
    }
    else if(Input::IsMBReleased(m_Window, GLFW_MOUSE_BUTTON_RIGHT))
    {
        glfwSetInputMode(m_Window.GetHandle(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        firstMouse = true;
    }
}