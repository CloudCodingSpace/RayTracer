#include "RayTracer.h"

#include <stb/stb_image_write.h>

#include <filesystem>

void RayTracer::Init()
{
	m_Width = m_Height = 800;

	m_Renderer.Init(m_Width, m_Height);
}

void RayTracer::Run()
{
	if (!std::filesystem::exists("output")) {
		std::filesystem::create_directory("output");
	}

	OutputPng(m_Renderer.GenImage(), "output/img.png");
}

void RayTracer::Cleanup()
{
	m_Renderer.Cleanup();
}

void RayTracer::OutputPng(uint32_t* data, std::string path)
{
	stbi_write_png(path.c_str(), m_Width, m_Height, 4, data, sizeof(uint32_t) * m_Width);
}