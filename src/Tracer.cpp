#include "Tracer.h"

#include <filesystem>

#include <GLFW/glfw3.h>

#include "Rendering/GuiHelper.h"
#include "Window/Input.h"

#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

#include <imgui/imgui.h>

#include <nfd.h>

#include <string>
#include <thread>
#include <cstdlib>
#include <filesystem>

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
                if(m_IsSceneHovered || !m_Camera.IsFirstMouseUsage())
                    m_Camera.Update(m_Window, deltaTime);
            }

            if(Input::IsKeyPressed(m_Window, GLFW_KEY_ESCAPE))
                break;
            
            
            if(m_Accumulate && !m_Camera.IsActive())
                m_FrameIdx++;
            else
            {
                m_FrameIdx = 1;
                m_PrevFrame.GetTexture().SetPixels(nullptr);
            }
        }
        
        // ImGui
        {            
            GuiHelper::StartFrame();
            
            ImGui::Begin("Scene");

            m_IsSceneHovered = ImGui::IsWindowHovered();
            
            if(m_Render)
            {
                m_Fb.Resize(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y);
                if(m_PrevFrame.Resize(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y))
                {
                    m_FrameIdx = 1;
                    m_PrevFrame.GetTexture().SetPixels(nullptr);
                }

                m_PrevFrame.Bind();
                glViewport(0, 0, ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y);
                m_PrevFrame.Unbind();

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
            ImGui::Checkbox("Accumulate", &m_Accumulate);
            
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

            bool resetFrameIdx = false;
            if(ImGui::TreeNodeEx("Skybox", ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanFullWidth))
            {
                ImGui::Spacing(); 
                ImGui::Spacing(); 
                ImGui::Spacing(); 
                
                ImGui::Text("Sky Color");
                if(ImGui::ColorEdit3("##skyColor", &m_SkyColor[0]))
                    resetFrameIdx = true;
                
                ImGui::Spacing(); 
                ImGui::Spacing(); 
                ImGui::Spacing(); 

                ImGui::Checkbox("Use Skybox", &m_UseSkybox);

                ImGui::Spacing(); 
                ImGui::Spacing(); 
                ImGui::Spacing(); 

                ImGui::Text("Skybox Exposure");
                if(ImGui::DragFloat("##skyboxExposure", &m_Exposure, 0.1f, 1.0f, 10.0f))
                    resetFrameIdx = true;

                ImGui::Spacing(); 
                ImGui::Spacing(); 
                ImGui::Spacing(); 

                ImGui::Text("Select Skybox Image");
                ImGui::Image((ImTextureID)(intptr_t)m_SkyboxTex.GetHandle(), ImVec2(96, 54), ImVec2(0, 1), ImVec2(1, 0));
                if(ImGui::IsItemClicked())
                {
                    resetFrameIdx = true;

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
                ImGui::Spacing(); 
                ImGui::Spacing(); 
                ImGui::Spacing();
                
                ImGui::Text("Max Bounces Per Ray");
                if(ImGui::DragInt("##maxBounces", &m_Bounces, 1.0f, 1, 100))
                    resetFrameIdx = true;

                ImGui::Spacing(); 
                ImGui::Spacing(); 
                ImGui::Spacing(); 

                if(ImGui::Button("Add sphere"))
                {
                    m_Scene.spheres.push_back(Sphere());
    
                    glDeleteBuffers(1, &m_SceneSSBO);
    
                    glGenBuffers(1, &m_SceneSSBO);
                    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_SceneSSBO);
                    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Sphere) * m_Scene.spheres.size(), m_Scene.spheres.data(), GL_DYNAMIC_DRAW);
                    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_SceneSSBO);
    
                    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
                }

                ImGui::Spacing(); 

                if(ImGui::Button("Add material"))
                {
                    m_Scene.materials.push_back(Material{});                    
                
                    glDeleteBuffers(1, &m_MatSSBO);
    
                    glGenBuffers(1, &m_MatSSBO);
                    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_MatSSBO);
                    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Material) * m_Scene.materials.size(), m_Scene.materials.data(), GL_DYNAMIC_DRAW);
                    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_MatSSBO);
    
                    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
                }
                
                ImGui::Separator();
               
                if(ImGui::TreeNodeEx("Materials", ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanFullWidth))
                {
                    for(int i = 0; i < m_Scene.materials.size(); i++)
                    {
                        if(ImGui::TreeNodeEx(("Material #" + std::to_string(i)).c_str(), ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanFullWidth))
                        {
                            ImGui::Text("Albedo");
                            if(ImGui::ColorEdit3(("##albedo" + std::to_string(i)).c_str(), &m_Scene.materials[i].albedo[0]))
                                resetFrameIdx = true;

                            ImGui::Spacing();
                            ImGui::Spacing();
                            ImGui::Spacing();

                            ImGui::Text("Emission Color");
                            if(ImGui::ColorEdit3(("##emissionColor" + std::to_string(i)).c_str(), &m_Scene.materials[i].emissionColor[0]))
                                resetFrameIdx = true;
                            
                            ImGui::Spacing();
                            ImGui::Spacing();
                            ImGui::Spacing();

                            ImGui::Text("Emission Power");
                            if(ImGui::DragFloat(("##emissivePower" + std::to_string(i)).c_str(), &m_Scene.materials[i].emissionPower, 0.1f, 0.0f, 10.0f))
                                resetFrameIdx = true;

                            ImGui::Spacing();
                            ImGui::Spacing();
                            ImGui::Spacing();

                            ImGui::Text("Roughness");
                            if(ImGui::DragFloat(("##rougness" + std::to_string(i)).c_str(), &m_Scene.materials[i].roughness, 0.1f, 0.0f, 1.0f))
                                resetFrameIdx = true;

                            const char* items[] = { "Diffuse", "Metallic", "Glass" };
                            int selectedIndex = m_Scene.materials[i].matIdx;  

                            if (ImGui::BeginCombo(("Material Type##" + std::to_string(i)).c_str(), items[selectedIndex])) {
                                for (int j = 0; j < IM_ARRAYSIZE(items); j++) {
                                    bool isSelected = (selectedIndex == j);
                                    if (ImGui::Selectable(items[j], isSelected)) {
                                        m_Scene.materials[i].matIdx = j;  // Update material-specific index
                                        resetFrameIdx = true;
                                    }
                                    if (isSelected) {
                                        ImGui::SetItemDefaultFocus();
                                    }
                                }
                                ImGui::EndCombo();
                            }

                            ImGui::TreePop();
                        }
                    }


                    ImGui::TreePop();
                }
                
                if(ImGui::TreeNodeEx("Spheres", ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanFullWidth)) {
                    for(int i = 0; i < m_Scene.spheres.size(); i++)
                    {
                        if(ImGui::TreeNodeEx(("Sphere " + std::to_string(i + 1)).c_str(), ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanFullWidth))
                        {
                            ImGui::Text("Sphere Center");
                            if(ImGui::DragFloat3(("##sphereCenter" + std::to_string(i)).c_str(), &m_Scene.spheres[i].center[0], 0.1f, -100.0f, 100.0f))
                                resetFrameIdx = true;
                            
                            ImGui::Spacing(); 
                            ImGui::Spacing(); 
                            ImGui::Spacing(); 
                            
                            ImGui::Text("Sphere Radius");
                            if(ImGui::DragFloat(("##sphereRadius" + std::to_string(i)).c_str(), &m_Scene.spheres[i].radius, 0.1f, 0.2f, 1000.0f))
                                resetFrameIdx = true;

                            ImGui::Spacing();
                            ImGui::Spacing();
                            ImGui::Spacing();

                            ImGui::Text("Material Index");
                            if(ImGui::DragInt(("##matIdx" + std::to_string(i)).c_str(), &m_Scene.spheres[i].materialIdx, 1.0f))
                                resetFrameIdx = true;

                            if(m_Scene.spheres[i].materialIdx >= m_Scene.materials.size())
                            {
                                m_Scene.spheres[i].materialIdx = 0;
                            }

                            ImGui::TreePop();
                        }
                    }

                    ImGui::TreePop();
                }
                ImGui::TreePop();
            }

            ImGui::End();

            if(resetFrameIdx)
            {
                m_FrameIdx = 1;
                m_PrevFrame.GetTexture().SetPixels(nullptr);
            }

            glViewport(0, 0, m_Window.GetWindowInfo().width, m_Window.GetWindowInfo().height);
            GuiHelper::EndFrame();
            GuiHelper::Update(m_Window);
        }

        {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, m_Fb.GetHandle());
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_PrevFrame.GetHandle());
            
            glBlitFramebuffer(0, 0, m_Fb.GetTexture().GetWidth(), m_Fb.GetTexture().GetHeight(), 
                        0, 0, m_PrevFrame.GetTexture().GetWidth(), m_PrevFrame.GetTexture().GetHeight(), 
                        GL_COLOR_BUFFER_BIT, GL_NEAREST);
         
            glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
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
    m_PrevFrame.Init(m_Window.GetWindowInfo().width, m_Window.GetWindowInfo().height);
    
    // Skybox texture
    {
        stbi_set_flip_vertically_on_load(true);
        int width, height, channels;
        float* pixels = stbi_loadf("assets/skybox/s4.hdr", &width, &height, &channels, STBI_rgb_alpha);

        m_SkyboxTex.Init(width, height, pixels, true);
    }

    // Scene
    {
        Scene::Deserialize(m_Scene, "scene.save");
    }
    // SSBOs
    {
        glGenBuffers(1, &m_SceneSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_SceneSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Sphere) * m_Scene.spheres.size(), m_Scene.spheres.data(), GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_SceneSSBO);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    
        glGenBuffers(1, &m_MatSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_MatSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Material) * m_Scene.materials.size(), m_Scene.materials.data(), GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_MatSSBO);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }
}

void Tracer::Cleanup()
{
    GuiHelper::Shutdown();

    m_SkyboxTex.Destroy();
    m_Fb.Destroy();

    m_Shader.Destroy();

    glDeleteBuffers(1, &m_MatSSBO);
    glDeleteBuffers(1, &m_SceneSSBO);

    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    m_Window.Destroy();

    Scene::Serialize(m_Scene, "scene.save");
}

void Tracer::Render(int width, int height)
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_SceneSSBO);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(Sphere) * m_Scene.spheres.size(), m_Scene.spheres.data());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_SceneSSBO);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_MatSSBO);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(Material) * m_Scene.materials.size(), m_Scene.materials.data());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_MatSSBO);

    m_Shader.Bind();

    m_Shader.PutVec2("u_resolution", glm::vec2(width, height));
    m_Shader.PutVec3("u_camPos", m_Camera.GetPos());
    m_Shader.PutVec3("u_camFront", m_Camera.GetFront());
    m_Shader.PutFloat("u_SkyboxExposure", m_Exposure);
    m_Shader.PutTex("t_Skybox", 0);
    m_Shader.PutTex("t_PrevFrame", 1);

    m_Shader.PutInt("u_MaxBounces", m_Bounces);
    m_Shader.PutInt("u_FrameIdx", m_FrameIdx);
    m_Shader.PutInt("u_Accumulate", (int)m_Accumulate);
    m_Shader.PutInt("u_CamActive", (int)m_Camera.IsActive());
    m_Shader.PutInt("u_SphereCount", m_Scene.spheres.size());
    m_Shader.PutInt("u_UseSkybox", m_UseSkybox);

    m_Shader.PutUint("u_RndmSeed", (uint32_t)rand());
    m_Shader.PutVec3("u_SkyColor", m_SkyColor);

    m_SkyboxTex.Active(1);
    m_SkyboxTex.Bind();

    m_SkyboxTex.Active(2);
    m_PrevFrame.GetTexture().Bind();

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
