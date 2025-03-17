#include "Tracer.h"

#include <filesystem>

#include <GLFW/glfw3.h>

#include "Rendering/GuiHelper.h"
#include "Window/Input.h"

#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

#include <imgui/imgui.h>

#include <nfd.h>

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

            auto& m_CamPos = m_Camera.GetPos();
            auto& m_CamFront = m_Camera.GetFront();
            float speed = 2.0f * deltaTime;
            if(Input::IsKeyPressed(m_Window, GLFW_KEY_W))
                m_CamPos += m_CamFront * speed;
            if(Input::IsKeyPressed(m_Window, GLFW_KEY_S))
                m_CamPos -= m_CamFront * speed;
            if(Input::IsKeyPressed(m_Window, GLFW_KEY_A))
                m_CamPos += glm::normalize(glm::cross(m_CamFront, glm::vec3(0.0f, 1.0f, 0.0f))) * speed;
            if(Input::IsKeyPressed(m_Window, GLFW_KEY_D))
                m_CamPos -= glm::normalize(glm::cross(m_CamFront, glm::vec3(0.0f, 1.0f, 0.0f))) * speed;
            if(Input::IsKeyPressed(m_Window, GLFW_KEY_SPACE))
                m_CamPos += glm::vec3(0, 1.0f, 0) * speed;
            if(Input::IsKeyPressed(m_Window, GLFW_KEY_LEFT_SHIFT))
                m_CamPos -= glm::vec3(0, 1.0f, 0) * speed;

            if(Input::IsKeyPressed(m_Window, GLFW_KEY_ESCAPE))
                break;

            m_Camera.Update(m_Window);
        }
        
        // ImGui
        {            
            GuiHelper::StartFrame();
            
            ImGui::Begin("Scene");
            
            if(m_Render)
            {
                m_Fb.Resize(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y);
                m_Fb.Bind();
                glViewport(0, 0, ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y);
                
                Render(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y);
                
                m_Fb.Unbind();
                
                ImGui::Image(reinterpret_cast<ImTextureID>(static_cast<uintptr_t>(m_Fb.GetTexture().GetHandle())), 
                                    ImVec2(m_Fb.GetTexture().GetWidth(), m_Fb.GetTexture().GetHeight()), 
                                    ImVec2(0, 1), ImVec2(1, 0));
            }
            
            ImGui::End();
            
            ImGui::Begin("Settings");
            
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

            ImGui::Separator();

            if(ImGui::TreeNodeEx("Skybox", ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanFullWidth))
            {
                ImGui::Text("Skybox Exposure");
                ImGui::SliderFloat("##skyboxExposure", &m_Exposure, 1.0f, 10.0f);
                
                ImGui::Spacing(); 
                ImGui::Spacing(); 
                ImGui::Spacing(); 

                ImGui::Text("Select Skybox Image");
                ImGui::Image((ImTextureID)(intptr_t)m_SkyboxTex.GetHandle(), ImVec2(96, 54), ImVec2(0, 1), ImVec2(1, 0));
                if(ImGui::IsItemClicked())
                {
                    nfdchar_t* outPath = nullptr;
                    nfdresult_t result = NFD_OpenDialog("hdr", nullptr, &outPath); 

                    if (result == NFD_OKAY) {
                        m_SkyboxTex.Destroy();
                        {
                            stbi_set_flip_vertically_on_load(true);
                            int width, height, channels;
                            float* pixels = stbi_loadf(outPath, &width, &height, &channels, STBI_rgb_alpha);

                            m_SkyboxTex.Init(width, height, pixels, true);
                        }

                        free(outPath);
                    }
                }

                ImGui::TreePop();
            }

            ImGui::End();

            glViewport(0, 0, m_Window.GetWindowInfo().width, m_Window.GetWindowInfo().height);
            GuiHelper::EndFrame();
            GuiHelper::Update(m_Window);
        }

        m_Window.ToggleFullscreenMode(800, 600);
        m_Window.Update();
    }

    Cleanup();
}

void Tracer::Init()
{
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
    
    // Skybox texture
    {
        stbi_set_flip_vertically_on_load(true);
        int width, height, channels;
        float* pixels = stbi_loadf("assets/skybox/s4.hdr", &width, &height, &channels, STBI_rgb_alpha);

        m_SkyboxTex.Init(width, height, pixels, true);
    }
}

void Tracer::Cleanup()
{
    GuiHelper::Shutdown();

    m_SkyboxTex.Destroy();
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
    m_Shader.PutVec3("u_camPos", m_Camera.GetPos());
    m_Shader.PutVec3("u_camFront", m_Camera.GetFront());
    m_Shader.PutFloat("m_SkyboxExposure", m_Exposure);

    m_Shader.PutTex("t_Skybox", 0);
    m_SkyboxTex.Active(1);
    m_SkyboxTex.Bind();

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void Tracer::WriteToPngFile(std::string fileName, unsigned char* pixels)
{
    int width = m_Fb.GetTexture().GetWidth();
    int height = m_Fb.GetTexture().GetHeight();
    int channels = 4;

    unsigned char* flippedPixels = new unsigned char[width * height * channels];
    for (int y = 0; y < height; y++) 
    {
        memcpy(flippedPixels + (height - 1 - y) * width * channels, 
            pixels + y * width * channels, 
            width * channels);
    }

    stbi_write_png(fileName.c_str(), width, height, channels, flippedPixels, width * channels);

    delete[] flippedPixels;
}