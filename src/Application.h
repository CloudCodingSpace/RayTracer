#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <cstdint>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>

#include <vulkan/vk_enum_string_helper.h>

typedef uint32_t u32;
typedef uint64_t u64;
typedef uint8_t u8;
typedef int32_t i32;

#define FRAMES_IN_FLIGHT 2

#define VK_CHECK(result) do { if(result != VK_SUCCESS) { printf("VkResult: %s (line: %d, file: %s\n", string_VkResult(result), __LINE__, __FILE__); assert(false); } } while(0);
#define CLAMP(value, min, max) ((value < min) ? min : (value > max) ? max : value)

u8* ReadFile(std::string path, u64* size);

struct ScCaps
{
    VkExtent2D extent;
    VkSurfaceFormatKHR format;
    VkPresentModeKHR presentMode;
    VkSurfaceCapabilitiesKHR caps; 
};

class Application
{
public:
    Application(const char* name, int width, int height, bool headless);
    ~Application();

    virtual void Run();

protected:
    bool StartFrame();
    void EndFrame();

    ScCaps GetScCaps();
    void CreateSwapchain();
    void Resize();

protected:
    struct ImageInfo {
        u32 width, height;
        VkFormat format;
        bool gpuResource;
        VkImageUsageFlags usage;
        VkMemoryPropertyFlagBits memProps;
        VkImageAspectFlags aspectFlags;
    };

    struct Image {
        ImageInfo info;
        VkImage image;
        VkDeviceMemory mem;
        VkImageView view;
        VkSampler sampler;
    };

    struct BufferInfo {
        u64 size;
        bool bda;
        VkMemoryPropertyFlags memProps;
        VkBufferUsageFlags usage;
        void* data;
    };

    struct Buffer {
        BufferInfo info;
        VkBuffer buffer;
        VkDeviceMemory memory;
        void* mappedMem;
    };

    class Camera
    {
    public:
        void Create(Application* rt);

        void Update(float dt);

        inline float& GetFOV() { return m_Fov; }
        inline glm::vec3 GetPos() { return m_Pos; }
        inline glm::vec3 GetFront() { return m_Front; }
        inline glm::vec3 GetUp() { return m_Up; }
        inline glm::vec3 GetRight() { return m_Right; }
        inline glm::mat4 GetVP() { return m_Proj * m_View; }
    private:
        glm::mat4 m_Proj = glm::mat4(1.0f);
        glm::mat4 m_View = glm::mat4(1.0f);
        glm::vec3 m_Front, m_Right, m_Up, m_Pos;
        Application* m_Rt = nullptr;
        bool m_FirstMouse = true;

        float m_LastX = 400, m_LastY = 300, m_Yaw = -90.0f, m_Pitch = 0.0f, m_Fov = 90.0f;
        const float m_Sensitivity = 0.05f, m_Speed = 0.025f;
    };

protected:
    void CreateBuffer(Buffer& buffer, const BufferInfo& info);
    void DestroyBuffer(Buffer& buffer);
    void UploadDataToBuffer(Buffer& buffer, void* data);

    void CreateImage(Image& image, const ImageInfo& info);
    void DestroyImage(Image& image);

    VkFence CreateFence();
    VkCommandBuffer AllocateCommandBuffer(VkCommandPool pool);
    void FreeCommandBuffer(VkCommandBuffer buffer, VkCommandPool pool);
    void BeginCommandBuffer(VkCommandBuffer buffer, VkCommandBufferUsageFlagBits usage);

protected:
    GLFWwindow* m_Window = nullptr;
    int m_Width, m_Height;
    bool m_Headless;

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

    VkDescriptorPool m_UiDescPool = nullptr;

    u32 m_ImageIdx = 0;
    u32 m_FrameIdx = 0;
};
