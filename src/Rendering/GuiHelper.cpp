#include "GuiHelper.h"

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

void GuiHelper::Init(Window& window, bool dockspace)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags = ImGuiConfigFlags_ViewportsEnable;
    if(dockspace)
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();

    io.Fonts->AddFontFromFileTTF("assets/fonts/consolas.ttf", 20.0f);
    io.Fonts->Build();

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    style.WindowPadding = ImVec2(0.0f, 0.0f);
    style.FramePadding = ImVec2(5, 5);

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
        // ImGui::DockSpaceOverViewport(ImGui::GetID("DockingID"), ImGui::GetMainViewport());

        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::Begin("DockingSpace Window", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoDocking);
        ImGui::PopStyleVar(2);

        ImGui::DockSpace(ImGui::GetID("Dockspace"), ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
    }
}

void GuiHelper::EndFrame()
{
    if(ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable)
        ImGui::End();

    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void GuiHelper::Update(Window& window)
{
    auto* backupctx = window.GetHandle();
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
    glfwMakeContextCurrent(backupctx);
}

void GuiHelper::Shutdown()
{
    ImGui_ImplGlfw_Shutdown();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();
}