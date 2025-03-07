#include "GuiHelper.h"

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

void GuiHelper::Init(Window& window, bool dockspace)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    if(dockspace)
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window.GetHandle(), true);
    ImGui_ImplOpenGL3_Init("#version 330 core");    
}

void GuiHelper::StartFrame()
{
    ImGui_ImplGlfw_NewFrame();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    if(ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGui::DockSpaceOverViewport(ImGui::GetID("DockingID"), ImGui::GetMainViewport());
    }
}

void GuiHelper::EndFrame()
{
    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void GuiHelper::Shutdown()
{
    ImGui_ImplGlfw_Shutdown();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();
}