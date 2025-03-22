#include "Tracer.h"

#include <filesystem>

#include <GLFW/glfw3.h>

#include "Rendering/GuiHelper.h"
#include "Window/Input.h"

#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

#include <imgui/imgui.h>

#include <nfd.h>

#include <thread>

void Tracer::Run()
{
    Init();

    m_Window.Show();
    while(m_Window.IsOpened()) 
    {
        m_Window.Clear();

        {
            currentTime = glfwGetTime();
            deltaTime = currentTime - lastTime;
            lastTime = currentTime;

            if(m_Render)
            {
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

                if(m_IsMouseHovered || !m_Camera.IsFirstMouseUsage())
                    m_Camera.Update(m_Window);
            }    

            if(Input::IsKeyPressed(m_Window, GLFW_KEY_ESCAPE))
                break;
        }
        
        // ImGui
        {            
            GuiHelper::StartFrame();
            
            ImGui::Begin("Scene");

            m_IsMouseHovered = ImGui::IsWindowHovered();
            
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
            ImGui::Text("%.0f FPS", 1.0f/deltaTime);
            
            ImGui::Checkbox("Render", &m_Render);
            
            ImGui::Spacing();
            ImGui::Spacing();
            
            if (ImGui::Button("Save to file") && m_Render)
            {
                int width = m_Fb.GetTexture().GetWidth();
                int height = m_Fb.GetTexture().GetHeight();
                unsigned char* pixels = new unsigned char[width * height * 4];

                m_Fb.GetTexture().GetPixels(pixels);

                std::thread saveThread([this, pixels, width, height]() {
                    if (!std::filesystem::exists("output"))
                    {
                        std::filesystem::create_directory("output");
                    }

                    WriteToPngFile("output/img.png", pixels, width, height);
                    delete[] pixels;
                });

                saveThread.detach();
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

            if(ImGui::TreeNodeEx("Scene", ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanFullWidth))
            {
                for(int i = 0; i < m_Scene.spheres.size(); i++)
                {
                    if(ImGui::TreeNodeEx(("Sphere " + std::to_string(i + 1)).c_str(), ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanFullWidth))
                    {
                        ImGui::Text("Sphere Center");
                        ImGui::SliderFloat3(("##sphereCenter" + std::to_string(i)).c_str(), &m_Scene.spheres[i].center[0], -100.0f, 100.0f);
                        
                        ImGui::Spacing(); 
                        ImGui::Spacing(); 
                        ImGui::Spacing(); 

                        ImGui::Text("Sphere Albedo");
                        ImGui::ColorEdit3(("##sphereAlbedo" + std::to_string(i)).c_str(), &m_Scene.spheres[i].albedo[0]);
                        
                        ImGui::Spacing(); 
                        ImGui::Spacing(); 
                        ImGui::Spacing(); 

                        ImGui::Text("Sphere Radius");
                        ImGui::SliderFloat(("##sphereRadius" + std::to_string(i)).c_str(), &m_Scene.spheres[i].radius, 0.2f, 100.0f);

                        ImGui::TreePop();
                    }
                }

                ImGui::Spacing(); 
                ImGui::Spacing(); 
                ImGui::Spacing(); 
                
                ImGui::Text("Light Direction");
                ImGui::SliderFloat3("##lightDirection", &m_LightDir[0], -1.0f, 1.0f);

                if(ImGui::Button("Add a sphere"))
                {
                    m_Scene.spheres.push_back(Sphere());

                    glDeleteBuffers(1, &ssbo);

                    glGenBuffers(1, &ssbo);
                    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
                    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Sphere) * m_Scene.spheres.size(), m_Scene.spheres.data(), GL_DYNAMIC_DRAW);
                    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);

                    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
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

    // Scene
    {
        m_Scene.spheres.push_back(Sphere{});
    }
    // SSBO
    {
        glGenBuffers(1, &ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Sphere) * m_Scene.spheres.size(), m_Scene.spheres.data(), GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
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
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(Sphere) * m_Scene.spheres.size(), m_Scene.spheres.data());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);

    m_Shader.Bind();

    m_Shader.PutVec2("u_resolution", glm::vec2(width, height));
    m_Shader.PutVec3("u_camPos", m_Camera.GetPos());
    m_Shader.PutVec3("u_camFront", m_Camera.GetFront());
    m_Shader.PutFloat("u_SkyboxExposure", m_Exposure);
    m_Shader.PutTex("t_Skybox", 0);
    
    m_Shader.PutInt("u_SphereCount", m_Scene.spheres.size());
    m_Shader.PutVec3("u_LightDir", m_LightDir);

    m_SkyboxTex.Active(1);
    m_SkyboxTex.Bind();

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void Tracer::WriteToPngFile(std::string fileName, unsigned char* pixels, int width, int height)
{
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