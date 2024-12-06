#include "Tracer.h"

#include "Window/Input.h"

#include <stb/stb_image.h>

void Tracer::Run()
{
    Init();

    m_Window.Show();
    while(m_Window.IsOpened()) 
    {
        m_Window.Clear();

        // Update
        {
            currentTime = glfwGetTime();
            deltaTime = currentTime - lastTime;
            lastTime = currentTime;

            m_Window.SetTitle(m_Window.GetTitle() + " | FPS :- " + std::to_string(1.0f/deltaTime) + " | Delta :- " + std::to_string(deltaTime) + "s");

            if(Input::IsKeyPressed(m_Window, GLFW_KEY_W))
                m_CamPos.z -= 2.0f * deltaTime;
            if(Input::IsKeyPressed(m_Window, GLFW_KEY_S))
                m_CamPos.z += 2.0f * deltaTime;
            if(Input::IsKeyPressed(m_Window, GLFW_KEY_A))
                m_CamPos.x -= 2.0f * deltaTime;
            if(Input::IsKeyPressed(m_Window, GLFW_KEY_D))
                m_CamPos.x += 2.0f * deltaTime;

            if(Input::IsKeyPressed(m_Window, GLFW_KEY_SPACE))
                m_CamPos.y += 2.0f * deltaTime;
            if(Input::IsKeyPressed(m_Window, GLFW_KEY_LEFT_SHIFT))
                m_CamPos.y -= 2.0f * deltaTime;
        }

        Render();

        m_Window.Update();
    }

    Cleanup();
}

void Tracer::Init()
{
    m_CamPos = glm::vec3(0, 0, 0);
    lastTime = 0.0f;
    currentTime = 0.0f;
    deltaTime = 0.0f;

    // Window
    {
        WindowInfo info{};
        info.width = 800;
        info.height = 600;
        info.fullscreen = false;
        info.title = "RayTracer";

        m_Window.Init(info);
    }
    // Shader
    {
        m_Shader.Init("assets/shaders/main.glsl");
    }
    // GL
    {
        float vertices[] = {
            -1.0f,  1.0f, 0.0f,
            -1.0f, -1.0f, 0.0f,
             1.0f, -1.0f, 0.0f,
            -1.0f,  1.0f, 0.0f,
             1.0f, -1.0f, 0.0f,
             1.0f,  1.0f, 0.0f
        };

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);

        // Skybox Texture
        int width, height, channels;
        stbi_set_flip_vertically_on_load(true);
        stbi_uc* pixels = stbi_load("assets/skybox/s2.jpg", &width, &height, &channels, 4);
        
        glGenTextures(1, &skyTex);
        m_Shader.PutFloat("u_skyboxTexture", 1.0f);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, skyTex);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

        stbi_image_free(pixels);
    }
}

void Tracer::Cleanup()
{
    glDeleteTextures(1, &skyTex);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    m_Window.Destroy();
}

void Tracer::Render()
{
    m_Shader.Bind();
 
    auto info = m_Window.GetWindowInfo();
    m_Shader.PutVec2("u_resolution", glm::vec2(info.width, info.height));
    m_Shader.PutVec3("u_camPos", m_CamPos);
    m_Shader.PutFloat("u_skyboxStrength", 1.0f);
    m_Shader.PutFloat("u_skyboxCeiling", 1.0f);
    m_Shader.PutFloat("u_skyboxGamma", 1.0f);

    glActiveTexture(GL_TEXTURE0);
    m_Shader.PutTex("u_skyboxTexture", 0);
    glBindTexture(GL_TEXTURE_2D, skyTex);
 
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}
