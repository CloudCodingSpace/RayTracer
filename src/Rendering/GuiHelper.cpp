#include "GuiHelper.h"

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

void GuiHelper::Init(Window& window, bool dockspace)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags = ImGuiConfigFlags_ViewportsEnable | ImGuiConfigFlags_NavEnableKeyboard;
    if(dockspace)
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();

    io.Fonts->AddFontFromFileTTF("assets/fonts/consolas.ttf", 20.0f);
    io.Fonts->Build();

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 12.0f;
    style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    style.WindowPadding = ImVec2(0.0f, 0.0f);
    style.FrameBorderSize = 3;
    style.FramePadding = ImVec2(5, 5);
    style.FrameRounding = 6;
    style.TabRounding = 6;
    style.GrabRounding = 6;
    style.PopupRounding = 6;
    style.ChildRounding = 6;
    style.WindowRounding = 6;
    style.ScrollbarRounding = 6;
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.13f, 0.13f, 0.13f, 0.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.13f, 0.13f, 0.13f, 0.94f);
    colors[ImGuiCol_Border]                 = ImVec4(0.00f, 0.00f, 0.00f, 0.50f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.14f, 0.14f, 0.14f, 0.74f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.33f, 0.33f, 0.33f, 0.54f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.31f, 0.31f, 0.31f, 0.40f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.23f, 0.23f, 0.23f, 0.75f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.12f, 0.12f, 0.12f, 0.51f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.13f, 0.13f, 0.13f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.40f, 0.40f, 0.41f, 1.00f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.39f, 0.39f, 0.40f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.43f, 0.43f, 0.43f, 1.00f);
    colors[ImGuiCol_Button]                 = ImVec4(0.25f, 0.24f, 0.24f, 0.40f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.46f, 0.46f, 0.46f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(0.29f, 0.29f, 0.29f, 0.31f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.29f, 0.29f, 0.29f, 0.31f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.46f, 0.46f, 0.46f, 1.00f);
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.39f, 0.39f, 0.39f, 0.78f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ResizeGrip]             = ImVec4(0.16f, 0.16f, 0.16f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.20f, 0.20f, 0.20f, 0.67f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.27f, 0.28f, 0.28f, 0.95f);
    colors[ImGuiCol_TabHovered]             = ImVec4(0.27f, 0.27f, 0.27f, 0.80f);
    colors[ImGuiCol_Tab]                    = ImVec4(0.28f, 0.28f, 0.28f, 0.86f);
    colors[ImGuiCol_TabSelected]            = ImVec4(0.47f, 0.47f, 0.47f, 1.00f);
    colors[ImGuiCol_TabSelectedOverline]    = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    colors[ImGuiCol_TabDimmed]              = ImVec4(0.18f, 0.19f, 0.21f, 0.97f);
    colors[ImGuiCol_TabDimmedSelected]      = ImVec4(0.17f, 0.19f, 0.22f, 1.00f);
    colors[ImGuiCol_TabDimmedSelectedOverline]  = ImVec4(0.19f, 0.17f, 0.17f, 1.00f);
    colors[ImGuiCol_DockingPreview]         = ImVec4(0.20f, 0.29f, 0.41f, 0.70f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);

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
        ImGui::SetNextWindowDockID(ImGui::GetID("Dockspace"));
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