#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <cstdint>
#include <vector>
#include <set>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>

typedef uint32_t u32;
typedef uint64_t u64;
typedef uint8_t u8;
typedef int32_t i32;

#define FRAMES_IN_FLIGHT 2

struct ScCaps
{
    VkExtent2D extent;
    VkSurfaceFormatKHR format;
    VkPresentModeKHR presentMode;
    VkSurfaceCapabilitiesKHR caps; 
};

class RayTracer
{
public:
    RayTracer();
    ~RayTracer();

    void Run();

private:
    bool StartFrame();
    void EndFrame();

    ScCaps GetScCaps();
    void CreateSwapchain();
    void Resize();
    void ResizeImages(u32 width, u32 height);

private:
    struct {
        float resolution[2];
    } m_PushConstantData = {};

    GLFWwindow* m_Window = nullptr;
    int m_Width, m_Height;

    VkInstance m_Instance = nullptr;
    VkSurfaceKHR m_Surface = nullptr;
    VkPhysicalDevice m_PhysicalDevice = nullptr;
    VkPhysicalDeviceFeatures m_PhysicalDeviceFeatures = {};
    VkDevice m_Device = nullptr;
    i32 m_GraphicsQueueIdx = -1, m_PresentQueueIdx = -1, m_ComputeQueueIdx = -1;
    VkQueue m_GraphicsQueue = nullptr, m_PresentQueue = nullptr, m_ComputeQueue = nullptr;
    VkRenderPass m_Pass = nullptr;
    std::vector<u32> m_UniqueQueues;

    ScCaps m_ScCaps{};
    VkSwapchainKHR m_Swapchain = nullptr;
    std::vector<VkImage> m_ScImages;
    std::vector<VkImageView> m_ScImageViews;
    std::vector<VkFramebuffer> m_Framebuffers;

    VkCommandPool m_GraphicsCmdPool = nullptr;
    VkCommandPool m_ComputeCmdPool = nullptr;
    VkCommandBuffer m_GraphicsCmdBuffs[FRAMES_IN_FLIGHT];
    VkCommandBuffer m_ComputeCmdBuffs[FRAMES_IN_FLIGHT];
    VkFence m_InFlightFences[FRAMES_IN_FLIGHT];
    VkSemaphore m_ImageAvailable[FRAMES_IN_FLIGHT];
    VkSemaphore m_ComputeFinished[FRAMES_IN_FLIGHT];
    std::vector<VkSemaphore> m_RenderFinished;

    VkPipelineLayout m_PipelineLayout = nullptr;
    VkPipeline m_Pipeline = nullptr;

    VkDescriptorPool m_UiDescPool = nullptr;
    VkDescriptorPool m_DescPool = nullptr;
    VkDescriptorSetLayout m_DescLayout = nullptr;
    VkDescriptorSet m_Sets[FRAMES_IN_FLIGHT] = {};
    VkDescriptorSet m_IgSets[FRAMES_IN_FLIGHT] = {};

    struct {
        VkImage image = nullptr;
        VkDeviceMemory memory = nullptr;
        VkImageView view = nullptr;
        VkSampler sampler = nullptr;
        u32 width = 800, height = 600;
    } m_StorageImages[FRAMES_IN_FLIGHT];

    u32 m_ImageIdx = 0;
    u32 m_FrameIdx = 0;
    bool m_ResizeImages = false;
    ImVec2 m_SceneSize = ImVec2(0, 0);
    double m_DeltaTime = 0, m_LastTime = 0, m_DeltaAccum = 0, m_DisplayedDelta = 0;
};