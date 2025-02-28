#include "Tracer.h"

#include <GLFW/glfw3.h>
#include "Window/Input.h"

#include <stb/stb_image.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

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

        // Update 
        {
            if(Input::IsKeyPressed(m_Window, GLFW_KEY_ESCAPE))
                break;
        }

        Render();

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
    // ImGui
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;

        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 5.0f;
        style.WindowPadding = ImVec2(0.0f, 0.0f);
        style.WindowBorderSize = 0.1f;

        ImGui::StyleColorsDark();
    
        ImGui_ImplGlfw_InitForOpenGL(m_Window.GetHandle(), true);
        ImGui_ImplOpenGL3_Init("#version 330 core");
    }
}

void Tracer::Cleanup()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    m_Window.Destroy();
}

void Tracer::Render()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Settings");
    ImGui::Text("This is the settings panel");

    ImGui::Separator();

    ImGui::Text("Delta Time: %.2fms", deltaTime * 1000);

    ImGui::Checkbox("Render", &m_Render);

    if(m_Render)
    {
        m_Shader.Bind();
 
        auto info = m_Window.GetWindowInfo();
        m_Shader.PutVec2("u_resolution", glm::vec2(info.width, info.height));
        m_Shader.PutVec3("u_camPos", m_CamPos);

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
