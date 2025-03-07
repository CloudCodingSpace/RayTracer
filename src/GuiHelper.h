#pragma once

#include "Window/Window.h"

class GuiHelper
{
public:
    static void Init(Window& window, bool dockspace = true);
    static void StartFrame();
    static void EndFrame();
    static void Shutdown();
};