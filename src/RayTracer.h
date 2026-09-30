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
    struct Sphere 
    {
        glm::vec3 center;
        float radius;
    };

    struct CameraBufferData
    {
        glm::vec3 front;
        glm::vec3 right;
        glm::vec3 up;
        glm::vec3 position;
        float fov;
    };

    struct BufferRefsData
    {
        VkDeviceAddress cameraBuffer;
        VkDeviceAddress sphereBuffer;
    };

    struct PushConstantData
    {
        glm::vec2 resolution;
        u32 seed;
        float aspectRatio;
        VkDeviceAddress bufferRefs;
    };

private:
    void ResizeImages(u32 width, u32 height);

private:
    VkPipelineLayout m_PipelineLayout = nullptr;
    VkPipeline m_Pipeline = nullptr;

    VkDescriptorPool m_DescPool = nullptr;
    VkDescriptorSetLayout m_DescLayout = nullptr;
    VkDescriptorSet m_Sets[FRAMES_IN_FLIGHT] = {};
    VkDescriptorSet m_IgSets[FRAMES_IN_FLIGHT] = {};

    VkDeviceAddress m_SphereBufferAddress;
    Buffer m_SphereBuffer;
    
    VkDeviceAddress m_BufferRefsAddress[FRAMES_IN_FLIGHT];
    void* m_BufferRefsMappedMem[FRAMES_IN_FLIGHT];
    Buffer m_BufferRefs[FRAMES_IN_FLIGHT];

    VkDeviceAddress m_CameraBufferAddress[FRAMES_IN_FLIGHT];
    void* m_CameraBufferMappedMem[FRAMES_IN_FLIGHT];
    Buffer m_CameraBuffer[FRAMES_IN_FLIGHT];
    
    Image m_StorageImages[FRAMES_IN_FLIGHT];

    std::vector<Sphere> m_Spheres;
    PushConstantData m_PushConstantData{};
    Camera m_Camera{};
    u32 m_DtIdx = 0;
    double m_DeltaTime[DT_SAMPLES];
    double m_LastTime = 0, m_DisplayedDelta = 0;
    ImVec2 m_SceneSize = ImVec2(0, 0);
    bool m_ResizeImages = false;
};
