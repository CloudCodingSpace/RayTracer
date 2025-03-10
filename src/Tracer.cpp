#include "Tracer.h"

#include <filesystem>

#include <GLFW/glfw3.h>

#include "GuiHelper.h"
#include "Window/Input.h"

#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

#include <imgui/imgui.h>

void Tracer::Run()
{
    Init();

    m_Window.Show();
    while(m_Window.IsOpened()) 
    {
        m_Window.Clear();

        // Checking keyevents
        {
            currentTime = glfwGetTime();
            deltaTime = currentTime - lastTime;
            lastTime = currentTime;

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

            if(Input::IsKeyPressed(m_Window, GLFW_KEY_ESCAPE))
                break;
        }
        
        // ImGui
        {            
            GuiHelper::StartFrame();
            
            ImGui::Begin("Scene");
            
            if(m_Render)
            {
                m_Fb.Resize(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y);
                m_Fb.Bind();
                glViewport(0, 0, ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().x);
                
                Render(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y);
                
                m_Fb.Unbind();
                
                ImGui::Image(reinterpret_cast<ImTextureID>(static_cast<uintptr_t>(m_Fb.GetTexture().GetHandle())), 
                                    ImVec2(m_Fb.GetTexture().GetWidth(), m_Fb.GetTexture().GetHeight()), 
                                    ImVec2(0, 1), ImVec2(1, 0));
            }
            
            ImGui::End();
            
            ImGui::Begin("Settings");
            ImGui::Text("This is the settings panel");
            
            ImGui::Spacing();
            ImGui::Spacing();
            
            ImGui::Text("Delta Time: %.2fms", deltaTime * 1000);
            
            ImGui::Checkbox("Render", &m_Render);
            
            ImGui::Spacing();
            ImGui::Spacing();
            
            if(ImGui::Button("Save to file") && m_Render)
            {
                unsigned char* pixels = new unsigned char[m_Fb.GetTexture().GetWidth() * m_Fb.GetTexture().GetHeight() * 4];
                
                if(!std::filesystem::exists("output"))
                {
                    std::filesystem::create_directory("output");
                }

                m_Fb.GetTexture().GetPixels(pixels);

                WriteToPngFile("output/img.png", pixels);

                delete[] pixels;
            }

            ImGui::End();

            glViewport(0, 0, m_Window.GetWindowInfo().width, m_Window.GetWindowInfo().height);
            GuiHelper::EndFrame();
            GuiHelper::Update(m_Window);
        }

        m_Window.Update();
    }

    Cleanup();
}

void Tracer::Init()
{
    m_CamPos = glm::vec3(0, 0, 2);
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
    }

    GuiHelper::Init(m_Window);

    m_Fb.Init(m_Window.GetWindowInfo().width, m_Window.GetWindowInfo().height);
}

void Tracer::Cleanup()
{
    GuiHelper::Shutdown();

    m_Fb.Destroy();

    m_Shader.Destroy();

    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    m_Window.Destroy();
}

void Tracer::Render(int width, int height)
{
    m_Shader.Bind();

    m_Shader.PutVec2("u_resolution", glm::vec2(width, height));
    m_Shader.PutVec3("u_camPos", m_CamPos);

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void Tracer::WriteToPngFile(std::string fileName, unsigned char* pixels)
{
    stbi_write_png(fileName.c_str(), m_Fb.GetTexture().GetWidth(), m_Fb.GetTexture().GetHeight(), 4, pixels, m_Fb.GetTexture().GetWidth() * 4);
}