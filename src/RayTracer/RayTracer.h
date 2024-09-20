#pragma once

#include "Renderer/Renderer.h"

#include <string>

class RayTracer
{
public:
    void Init();
    void Run();
    void Cleanup();
private:
    uint32_t m_Width, m_Height;
    Renderer m_Renderer;
private:
    void OutputPng(uint32_t* data, std::string path);
};