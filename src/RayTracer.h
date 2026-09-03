#pragma once

#include "Application.h"

#define DT_SAMPLES 10

class RayTracer : virtual Application
{
public:
    RayTracer();
    ~RayTracer();

    virtual void Run() override;

private:
    void ResizeImages(u32 width, u32 height);

private:
    VkPipelineLayout m_PipelineLayout = nullptr;
    VkPipeline m_Pipeline = nullptr;

    VkDescriptorPool m_DescPool = nullptr;
    VkDescriptorSetLayout m_DescLayout = nullptr;
    VkDescriptorSet m_Sets[FRAMES_IN_FLIGHT] = {};
    VkDescriptorSet m_IgSets[FRAMES_IN_FLIGHT] = {};

    Image m_StorageImages[FRAMES_IN_FLIGHT];
    
    u32 m_DtIdx = 0;
    double m_DeltaTime[DT_SAMPLES];
    double m_LastTime = 0, m_DisplayedDelta = 0;
    ImVec2 m_SceneSize = ImVec2(0, 0);
    bool m_ResizeImages = false;
};