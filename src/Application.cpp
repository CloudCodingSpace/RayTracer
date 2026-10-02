#include "Application.h"

#include <cassert>
#include <string>
#include <cstring>
#include <algorithm>
#include <vector>

u8* ReadFile(std::string path, u64* size) {
    FILE* file = fopen(path.c_str(), "rb");
    assert(file && "Failed to open file!");

    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    rewind(file);

    u8* content = new u8[*size];
    fread(content, sizeof(u8) * *size, 1, file);

    fclose(file);

    return content;
}

static uint32_t FindMemoryType(VkPhysicalDevice device, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties props = {};
    vkGetPhysicalDeviceMemoryProperties(device, &props);

    for (uint32_t i = 0; i < props.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) && (props.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    assert(false && "Failed to find the suitable memory index!");
    return 0xffffffff;
}

Application::Application(const char* name, int width, int height, bool headless) : m_Width{ width }, m_Height{ height }, m_Headless{ headless }
{
    // Window
    if(!headless) {
        assert(glfwInit() && "Failed to initialize glfw!");
        glfwWindowHint(GLFW_VISIBLE, false);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        
        m_Window = glfwCreateWindow(m_Width, m_Height, name, nullptr, nullptr);
        assert(m_Window && "Failed to create the window!");
    }
    // Instance
    {
        u32 extCount = 0;
        const char** exts = glfwGetRequiredInstanceExtensions(&extCount);

        VkApplicationInfo appInfo = {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = name,
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = name,
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = VK_API_VERSION_1_2
        };

        VkInstanceCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &appInfo,
            .enabledExtensionCount = extCount,
            .ppEnabledExtensionNames = exts
        };

        if(headless)
            info.enabledExtensionCount = 0;

        VK_CHECK(vkCreateInstance(&info, nullptr, &m_Instance));
    }
    // Surface
    if(!headless)
        VK_CHECK(glfwCreateWindowSurface(m_Instance, m_Window, nullptr, &m_Surface));
    // Physical device
    {
        u32 count = 0;
        VK_CHECK(vkEnumeratePhysicalDevices(m_Instance, &count, nullptr));
        std::vector<VkPhysicalDevice> devices(count);
        VK_CHECK(vkEnumeratePhysicalDevices(m_Instance, &count, devices.data()));

        for(auto& device : devices)
        {
            u32 queueCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueCount, nullptr);
            std::vector<VkQueueFamilyProperties> queueProps(queueCount);
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueCount, queueProps.data());
            i32 gIdx = -1, pIdx = -1, cIdx = -1;
            for(u32 i = 0; i < queueCount; i++)
            {
                if(queueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                    gIdx = i;
                if(queueProps[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
                    cIdx = i;

                VkBool32 present = false;
                if(!headless) {
                    VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_Surface, &present));
                    if(present)
                        pIdx = i;
                } else {
                    present = true;
                    pIdx = UINT_MAX;
                }
                
                VkPhysicalDeviceScalarBlockLayoutFeatures scalarFeatures = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES };
                VkPhysicalDeviceBufferDeviceAddressFeatures bdaFeatures = { 
                    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
                    .pNext = &scalarFeatures
                };
                VkPhysicalDeviceFeatures2 features = {};
                features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
                features.pNext = &bdaFeatures;

                vkGetPhysicalDeviceFeatures2(device, &features);

                if(present && (gIdx != -1) && (pIdx != -1) && (cIdx != -1) && scalarFeatures.scalarBlockLayout && bdaFeatures.bufferDeviceAddress) {
                    m_PhysicalDevice = device;
                    m_GraphicsQueueIdx = gIdx;
                    m_PresentQueueIdx = pIdx;
                    m_ComputeQueueIdx = cIdx;
                    break;
                }
            }
        }

        assert((m_PhysicalDevice != nullptr) && "Failed to find a suitable physical device!");

        vkGetPhysicalDeviceFeatures(m_PhysicalDevice, &m_PhysicalDeviceFeatures);
    }
    // Device
    {
        bool extsSupported = false;
        std::vector<const char*> exts = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };
        // Checking if extensions supported
        if(!headless) {
            u32 count = 0;
            VK_CHECK(vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &count, nullptr));
            std::vector<VkExtensionProperties> props(count);
            VK_CHECK(vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &count, props.data()));

            for(auto ext : exts)
            {
                for(auto& prop : props)
                {
                    if(strcmp(prop.extensionName, ext) == 0)
                        extsSupported = true;
                }
            }
            assert(extsSupported && "The device extensions aren't supported!");
        }

        float priority = 1.0f;
        i32 indices[3] = {
            m_GraphicsQueueIdx,
            m_ComputeQueueIdx,
            m_PresentQueueIdx
        };

        for(i32 i = 0; i < (headless ? 2 : 3); i++) 
        {
            bool exists = false;
            for(i32 idx : m_UniqueQueues) 
            {
                if(idx == indices[i])
                    exists = true;
            }
            if(!exists)
                m_UniqueQueues.push_back(indices[i]);
        }

        std::vector<VkDeviceQueueCreateInfo> queueInfos;
        for(i32 idx : m_UniqueQueues)
        {
            VkDeviceQueueCreateInfo info = {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = (u32)idx,
                .queueCount = 1,
                .pQueuePriorities = &priority
            };
            queueInfos.push_back(info);
        }

        VkPhysicalDeviceBufferDeviceAddressFeatures bdaFeatures = {};
        bdaFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
        bdaFeatures.bufferDeviceAddress = VK_TRUE;

        VkPhysicalDeviceScalarBlockLayoutFeatures scalarFeatures = {};
        scalarFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES;
        scalarFeatures.scalarBlockLayout = VK_TRUE;
        scalarFeatures.pNext = &bdaFeatures;

        VkDeviceCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = &scalarFeatures,
            .queueCreateInfoCount = (u32)queueInfos.size(),
            .pQueueCreateInfos = queueInfos.data(),
            .enabledExtensionCount = (u32)exts.size(),
            .ppEnabledExtensionNames = exts.data(),
            .pEnabledFeatures = &m_PhysicalDeviceFeatures
        };

        VK_CHECK(vkCreateDevice(m_PhysicalDevice, &info, nullptr, &m_Device));

        vkGetDeviceQueue(m_Device, m_GraphicsQueueIdx, 0, &m_GraphicsQueue);
        vkGetDeviceQueue(m_Device, m_ComputeQueueIdx, 0, &m_ComputeQueue);
        if(!headless)
            vkGetDeviceQueue(m_Device, m_PresentQueueIdx, 0, &m_PresentQueue);
    }
    
    // Renderpass
    if(!headless) {
        m_ScCaps = GetScCaps();
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = m_ScCaps.format.format;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

        VkAttachmentReference colorRef{};
        colorRef.attachment = 0;
        colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorRef;
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependency.dependencyFlags = 0;

        VkRenderPassCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        info.attachmentCount = 1;
        info.pAttachments = &colorAttachment;
        info.subpassCount = 1;
        info.pSubpasses = &subpass;
        info.dependencyCount = 1;
        info.pDependencies = &dependency;

        VK_CHECK(vkCreateRenderPass(m_Device, &info, nullptr, &m_Pass));
    }
    // Swapchain
    if(!headless)
        CreateSwapchain();
    // Command pool and buffers
    {
        {
            VkCommandPoolCreateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            info.queueFamilyIndex = m_GraphicsQueueIdx;
            info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            
            VK_CHECK(vkCreateCommandPool(m_Device, &info, nullptr, &m_GraphicsCmdPool));
            info.queueFamilyIndex = m_ComputeQueueIdx;
            VK_CHECK(vkCreateCommandPool(m_Device, &info, nullptr, &m_ComputeCmdPool));
        }
        {
            for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++) 
            {
                m_GraphicsCmdBuffs[i] = AllocateCommandBuffer(m_GraphicsCmdPool);
                m_ComputeCmdBuffs[i] = AllocateCommandBuffer(m_ComputeCmdPool);
            }
        }
    }
    // Sync objs
    {
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        VkSemaphoreCreateInfo semaInfo{};
        semaInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++)
        {
            VK_CHECK(vkCreateFence(m_Device, &fenceInfo, nullptr, &m_InFlightFences[i]));
            VK_CHECK(vkCreateSemaphore(m_Device, &semaInfo, nullptr, &m_ImageAvailable[i]));
            VK_CHECK(vkCreateSemaphore(m_Device, &semaInfo, nullptr, &m_ComputeFinished[i]));
        }
        
        m_RenderFinished.resize(m_ScImages.size());
        for(auto& sema : m_RenderFinished)
        {
            VK_CHECK(vkCreateSemaphore(m_Device, &semaInfo, nullptr, &sema));
        }
    }

    // UI descriptor pool
    if(!headless) {
        VkDescriptorPoolSize pool_sizes[] =
        {
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10000 },
        };
        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 10000;
        pool_info.poolSizeCount = 1;
        pool_info.pPoolSizes = pool_sizes;
        VK_CHECK(vkCreateDescriptorPool(m_Device, &pool_info, nullptr, &m_UiDescPool));
    }
    // ImGui
    if(!headless) {
        IMGUI_CHECKVERSION();
		ImGui::CreateContext();

        ImGui::StyleColorsDark();

        ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		
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

        io.Fonts->AddFontFromFileTTF("assets/fonts/consolas.ttf", 18.0f, nullptr, nullptr);

        ImGui_ImplGlfw_InitForVulkan(m_Window, true);
        
        ImGui_ImplVulkan_InitInfo info{};
        info.Subpass = 0;
        info.Allocator = nullptr;
        info.ApiVersion = VK_API_VERSION_1_2;
        info.DescriptorPool = m_UiDescPool;
        info.Device = m_Device;
        info.ImageCount = FRAMES_IN_FLIGHT;
        info.MinImageCount = FRAMES_IN_FLIGHT;
        info.Instance = m_Instance;
        info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        info.PhysicalDevice = m_PhysicalDevice;
        info.Queue = m_GraphicsQueue;
        info.QueueFamily = m_GraphicsQueueIdx;
        info.RenderPass = m_Pass;
        
        ImGui_ImplVulkan_Init(&info);
        ImGui_ImplVulkan_CreateFontsTexture();        
    }
}

Application::~Application()
{
    VK_CHECK(vkDeviceWaitIdle(m_Device));

    if(!m_Headless) {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        vkDestroyDescriptorPool(m_Device, m_UiDescPool, nullptr);
    }

    for(auto& fence : m_InFlightFences)
        vkDestroyFence(m_Device, fence, nullptr);
    for(auto& sema : m_ImageAvailable)
        vkDestroySemaphore(m_Device, sema, nullptr);
    for(auto& sema : m_ComputeFinished)
        vkDestroySemaphore(m_Device, sema, nullptr);
    for(auto& sema : m_RenderFinished)
        vkDestroySemaphore(m_Device, sema, nullptr);

    vkDestroyCommandPool(m_Device, m_GraphicsCmdPool, nullptr);
    vkDestroyCommandPool(m_Device, m_ComputeCmdPool, nullptr);

    for(auto& fb : m_Framebuffers)
        vkDestroyFramebuffer(m_Device, fb, nullptr);
    for(auto& view : m_ScImageViews)
        vkDestroyImageView(m_Device, view, nullptr);

    if(!m_Headless) {
        vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);
        vkDestroyRenderPass(m_Device, m_Pass, nullptr);
        vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
    }
    vkDestroyDevice(m_Device, nullptr);
    vkDestroyInstance(m_Instance, nullptr);

    glfwDestroyWindow(m_Window);
    glfwTerminate();
}

void Application::Run()
{
    if(!m_Headless)
        glfwShowWindow(m_Window);
    while(true)
    {
        if(!m_Headless) {
            if(glfwWindowShouldClose(m_Window))
                break;
            
            glfwGetFramebufferSize(m_Window, &m_Width, &m_Height);
        }

        if(!StartFrame())
            continue;
     
        // ImGui
        if(!m_Headless)
            ImGui::ShowDemoWindow();
        
        EndFrame();
        
        if(!m_Headless) {
            if(glfwGetKey(m_Window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
                break;

            glfwPollEvents();
        }
        else
            break;
    }
}

bool Application::StartFrame()
{
    VK_CHECK(vkWaitForFences(m_Device, 1, &m_InFlightFences[m_FrameIdx], true, UINT64_MAX));
   
    if(!m_Headless) {
        VkResult result = vkAcquireNextImageKHR(m_Device, m_Swapchain, UINT64_MAX, m_ImageAvailable[m_FrameIdx], nullptr, &m_ImageIdx);
        if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        {
            Resize();
            m_FrameIdx = (m_FrameIdx + 1) % FRAMES_IN_FLIGHT; 
            return false;
        }
        else
        {
            VK_CHECK(result);
        }
    }
    
    VK_CHECK(vkResetFences(m_Device, 1, &m_InFlightFences[m_FrameIdx]));
    VK_CHECK(vkResetCommandBuffer(m_GraphicsCmdBuffs[m_FrameIdx], 0));
    VK_CHECK(vkResetCommandBuffer(m_ComputeCmdBuffs[m_FrameIdx], 0));

    BeginCommandBuffer(m_GraphicsCmdBuffs[m_FrameIdx], VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    BeginCommandBuffer(m_ComputeCmdBuffs[m_FrameIdx], VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    if(m_Headless)
        return true;

    VkClearValue clearColor = {};
    clearColor.color = {0.1f, 0.1f, 0.1f, 1.0f};

    VkRenderPassBeginInfo rpInfo{};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpInfo.clearValueCount = 1;
    rpInfo.pClearValues = &clearColor;
    rpInfo.renderArea.offset = {0, 0};
    rpInfo.renderArea.extent = m_ScCaps.extent;
    rpInfo.renderPass = m_Pass;
    rpInfo.framebuffer = m_Framebuffers[m_ImageIdx];

    vkCmdBeginRenderPass(m_GraphicsCmdBuffs[m_FrameIdx], &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

    ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

    ImGui::DockSpaceOverViewport(ImGui::GetID("Dockspace"), ImGui::GetMainViewport());

    return true;
}

void Application::EndFrame()
{
    if(!m_Headless) {
        ImGui::EndFrame();
        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_GraphicsCmdBuffs[m_FrameIdx], nullptr);

        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();

        vkCmdEndRenderPass(m_GraphicsCmdBuffs[m_FrameIdx]);
    }

    VK_CHECK(vkEndCommandBuffer(m_GraphicsCmdBuffs[m_FrameIdx]));
    VK_CHECK(vkEndCommandBuffer(m_ComputeCmdBuffs[m_FrameIdx]));

    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };
    
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_ComputeCmdBuffs[m_FrameIdx];
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &m_ImageAvailable[m_FrameIdx];
    submitInfo.pSignalSemaphores = &m_ComputeFinished[m_FrameIdx];

    VK_CHECK(vkQueueSubmit(m_ComputeQueue, 1, &submitInfo, nullptr));
    
    submitInfo.pCommandBuffers = &m_GraphicsCmdBuffs[m_FrameIdx];
    submitInfo.pWaitSemaphores = &m_ComputeFinished[m_FrameIdx];
    submitInfo.pSignalSemaphores = &m_RenderFinished[m_ImageIdx];
    
    VK_CHECK(vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, m_InFlightFences[m_FrameIdx]));

    if(!m_Headless) {
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &m_Swapchain;
        presentInfo.pImageIndices = &m_ImageIdx;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &m_RenderFinished[m_ImageIdx];
        
        VkResult result = vkQueuePresentKHR(m_PresentQueue, &presentInfo);
        if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        {
            Resize();
        }
    }

    m_FrameIdx = (m_FrameIdx + 1) % FRAMES_IN_FLIGHT;
}

ScCaps Application::GetScCaps()
{
    ScCaps caps;
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, m_Surface, &caps.caps));

    {
        caps.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;

        u32 count = 0;
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &count, nullptr));
        std::vector<VkPresentModeKHR> modes(count);
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &count, modes.data()));
    
        for(auto& mode : modes)
        {
            if(mode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                caps.presentMode = mode;
                break;
            }
        }
    }

    {
        u32 count = 0;
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &count, nullptr));
        std::vector<VkSurfaceFormatKHR> formats(count);
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &count, formats.data()));

        caps.format = formats[0];

        for(auto& format : formats)
        {
            if((format.format == VK_FORMAT_R8G8B8A8_UNORM) && (format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR))
            {
                caps.format = format;
                break;
            }
        }
    }

    {
        if(caps.caps.currentExtent.width != UINT32_MAX)
        {
            caps.extent = caps.caps.currentExtent;
        }
        else
        {
            int width, height;
            glfwGetFramebufferSize(m_Window, &width, &height);

            caps.extent.width = width;
            caps.extent.height = height;
            caps.extent.width = CLAMP(caps.extent.width, caps.caps.minImageExtent.width, caps.caps.maxImageExtent.width);
            caps.extent.height = CLAMP(caps.extent.height, caps.caps.minImageExtent.height, caps.caps.maxImageExtent.height);
        }
    }

    return caps;
}

void Application::CreateSwapchain()
{
    {
        u32 imgCount = m_ScCaps.caps.minImageCount + 1;
        if(m_ScCaps.caps.maxImageCount > 0 && imgCount > m_ScCaps.caps.maxImageCount)
        {
            imgCount = m_ScCaps.caps.maxImageCount;
        }

        VkSwapchainCreateInfoKHR info = {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = m_Surface,
            .minImageCount = imgCount,
            .imageFormat = m_ScCaps.format.format,
            .imageColorSpace = m_ScCaps.format.colorSpace,
            .imageExtent = m_ScCaps.extent,
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .preTransform = m_ScCaps.caps.currentTransform,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = m_ScCaps.presentMode,
            .clipped = VK_TRUE,
            .oldSwapchain = nullptr
        };

        std::vector<u32> queues = {
            (u32)m_GraphicsQueueIdx,
            (u32)m_ComputeQueueIdx,
            (u32)m_PresentQueueIdx
        };

        std::sort(queues.begin(), queues.end());
        queues.erase(std::unique(queues.begin(), queues.end()), queues.end());

        if(queues.size() == 1)
        {
            info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }
        else
        {
            info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            info.queueFamilyIndexCount = queues.size();
            info.pQueueFamilyIndices = queues.data();
        }

        VK_CHECK(vkCreateSwapchainKHR(m_Device, &info, nullptr, &m_Swapchain));
    }
    {
        u32 count = 0;
        VK_CHECK(vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &count, nullptr));
        m_ScImages.resize(count);
        VK_CHECK(vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &count, m_ScImages.data()));
    }
    {
        m_ScImageViews.reserve(m_ScImages.size());
        for(auto& image : m_ScImages)
        {
            VkImageViewCreateInfo info {};
            info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            info.format = m_ScCaps.format.format;
            info.subresourceRange.levelCount = 1;
            info.subresourceRange.layerCount = 1;
            info.subresourceRange.baseMipLevel = 0;
            info.subresourceRange.baseArrayLayer = 0;
            info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            info.image = image;
            info.viewType = VK_IMAGE_VIEW_TYPE_2D;

            VkImageView view;
            VK_CHECK(vkCreateImageView(m_Device, &info, nullptr, &view));
            m_ScImageViews.push_back(view);
        }
    }
    {
        m_Framebuffers.reserve(m_ScImages.size());
        for(auto& view : m_ScImageViews)
        {
            VkFramebufferCreateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            info.attachmentCount = 1;
            info.pAttachments = &view;
            info.renderPass = m_Pass;
            info.layers = 1;
            info.width = m_ScCaps.extent.width;
            info.height = m_ScCaps.extent.height;

            VkFramebuffer fb;
            VK_CHECK(vkCreateFramebuffer(m_Device, &info, nullptr, &fb));
            m_Framebuffers.push_back(fb);
        }
    }
}

void Application::Resize()
{
    if(m_Headless)
        return;

    int width = 0, height = 0;
    glfwGetFramebufferSize(m_Window, &width, &height);

    while(width == 0 || height == 0)
    {
        glfwGetFramebufferSize(m_Window, &width, &height);
        glfwWaitEvents();
    }

    VK_CHECK(vkDeviceWaitIdle(m_Device));

    for(auto& fb : m_Framebuffers)
        vkDestroyFramebuffer(m_Device, fb, nullptr);
    for(auto& view : m_ScImageViews)
        vkDestroyImageView(m_Device, view, nullptr);
    for(auto& sema : m_RenderFinished)
        vkDestroySemaphore(m_Device, sema, nullptr);
    for(auto& sema : m_ImageAvailable)
        vkDestroySemaphore(m_Device, sema, nullptr);

    vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);
    m_ScImages.clear();
    m_ScImageViews.clear();
    m_Framebuffers.clear();
    m_RenderFinished.clear();

    VkSemaphoreCreateInfo semaInfo{};
    semaInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    m_ScCaps = GetScCaps();
    CreateSwapchain();

    m_RenderFinished.resize(m_ScImages.size());
    for(auto& sema : m_RenderFinished)
        VK_CHECK(vkCreateSemaphore(m_Device, &semaInfo, nullptr, &sema));
    for(auto& sema : m_ImageAvailable)
        VK_CHECK(vkCreateSemaphore(m_Device, &semaInfo, nullptr, &sema));
}

void Application::Camera::Create(Application* app)
{
    m_Rt = app;
    m_Pos = glm::vec3(0, 0, 3);
    m_Front = glm::vec3(0, 0, -1);
    m_Up = glm::vec3(0, 1, 0);
    m_Right = glm::normalize(glm::cross(m_Up, m_Front));
    m_LastX = (float)m_Rt->m_Width / 2;
    m_LastY = (float)m_Rt->m_Height / 2;
    
    if(m_Rt->m_Height == 0)
        return;
    m_Proj = glm::perspective(glm::radians(m_Fov), m_Rt->m_Width/(float)m_Rt->m_Height, 0.01f, 1000.0f);
    m_Proj[1][1] *= -1.0f;
    m_View = glm::lookAt(m_Pos, m_Pos + m_Front, m_Up);
}

void Application::Camera::Update(float dt)
{
    GLFWwindow* window = m_Rt->m_Window;
    bool moved = false;

    {
        if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            m_Pos += m_Front * m_Speed * dt;
            moved = true;
        }
        if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            m_Pos -= m_Front * m_Speed * dt;
            moved = true;
        }
        if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            m_Pos -= m_Right * m_Speed * dt;
            moved = true;
        }
        if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            m_Pos += m_Right * m_Speed * dt;
            moved = true;
        }
        if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
            m_Pos -= m_Up * m_Speed * dt;
            moved = true;
        }
        if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            m_Pos += m_Up * m_Speed * dt;
            moved = true;
        }
    }

    {
        if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
        {
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);

            if(m_FirstMouse)
            {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                m_LastX = xpos;
                m_LastY = ypos;
                m_FirstMouse = false;
            }

            float dtX = xpos - m_LastX;
            float dtY = m_LastY - ypos;

            m_LastX = xpos;
            m_LastY = ypos;

            if(dtX != 0 || dtY != 0)
            {
                dtX *= m_Sensitivity;
                dtY *= m_Sensitivity;

                m_Yaw += dtX;
                m_Pitch += dtY;

                if(m_Pitch > 89.9f)
                    m_Pitch = 89.9f;
                else if(m_Pitch < -89.9f)
                    m_Pitch = -89.9f;
                
                glm::vec3 front(0.0f);
                front.x = glm::cos(glm::radians(m_Yaw)) * glm::cos(glm::radians(m_Pitch));
                front.y = glm::sin(glm::radians(m_Pitch));
                front.z = glm::sin(glm::radians(m_Yaw)) * glm::cos(glm::radians(m_Pitch));

                m_Front = front;
                moved = true;
            }
        }
        else if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            m_FirstMouse = true;
        }
    }

    m_Right = glm::normalize(glm::cross(m_Front, glm::vec3(0, 1, 0)));
    m_Up = glm::normalize(glm::cross(m_Right, m_Front));

    if(!moved)
        return;
    
    if(m_Rt->m_Height == 0)
        return;

    
    m_Proj = glm::perspective(glm::radians(m_Fov), m_Rt->m_Width/(float)m_Rt->m_Height, 0.01f, 1000.0f);
    m_Proj[1][1] *= -1.0f;
    m_View = glm::lookAt(m_Pos, m_Pos + m_Front, m_Up);
}

void Application::CreateBuffer(Buffer& buffer, const BufferInfo& buffInfo)
{
    buffer.info = buffInfo;

    {
        VkBufferCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        info.queueFamilyIndexCount = m_UniqueQueues.size();
        info.pQueueFamilyIndices = m_UniqueQueues.data();
        info.sharingMode = m_UniqueQueues.size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
        info.size = buffInfo.size;
        info.usage = buffInfo.usage;

        if(buffInfo.bda)
            info.usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        
        VK_CHECK(vkCreateBuffer(m_Device, &info, nullptr, &buffer.buffer));
    }
    {
        VkMemoryRequirements req{};
        vkGetBufferMemoryRequirements(m_Device, buffer.buffer, &req);

        VkMemoryAllocateFlagsInfo flagInfo = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
            .pNext = NULL,
            .flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT
        };

        VkMemoryAllocateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        info.memoryTypeIndex = FindMemoryType(m_PhysicalDevice, req.memoryTypeBits, buffInfo.memProps);
        info.allocationSize = req.size;

        if(buffInfo.bda)
            info.pNext = &flagInfo;
        
        VK_CHECK(vkAllocateMemory(m_Device, &info, nullptr, &buffer.memory));
        VK_CHECK(vkBindBufferMemory(m_Device, buffer.buffer, buffer.memory, 0));
    }

    if((buffInfo.memProps & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) {
        VK_CHECK(vkMapMemory(m_Device, buffer.memory, 0, buffInfo.size, 0, &buffer.mappedMem));
    }

    if(buffInfo.data)
        UploadDataToBuffer(buffer, buffInfo.data);
}

void Application::DestroyBuffer(Buffer& buffer)
{
    if((buffer.info.memProps & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) {
        vkUnmapMemory(m_Device, buffer.memory);
    }

    vkDestroyBuffer(m_Device, buffer.buffer, nullptr);
    vkFreeMemory(m_Device, buffer.memory, nullptr);

    memset(&buffer, 0, sizeof(buffer));
}

void Application::UploadDataToBuffer(Buffer& buffer, void* data)
{
    if((buffer.info.memProps & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) {
        memcpy(buffer.mappedMem, data, buffer.info.size);
        return;
    }

    if((buffer.info.usage & VK_BUFFER_USAGE_TRANSFER_DST_BIT) != VK_BUFFER_USAGE_TRANSFER_DST_BIT || !buffer.info.size)
        assert(false && "Can't upload to buffer!");

    VkCommandBuffer cmdBuff = AllocateCommandBuffer(m_GraphicsCmdPool);
    VkFence fence = CreateFence();

    BufferInfo buffInfo{};
    buffInfo.size = buffer.info.size;
    buffInfo.memProps = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    buffInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    Buffer stagingBuffer{};
    CreateBuffer(stagingBuffer, buffInfo);

    memcpy(stagingBuffer.mappedMem, data, buffInfo.size);

    BeginCommandBuffer(cmdBuff, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    
    VkBufferCopy region{};
    region.size = buffInfo.size;
    region.srcOffset = 0;
    region.dstOffset = 0;
    vkCmdCopyBuffer(cmdBuff, stagingBuffer.buffer, buffer.buffer, 1, &region);

    vkEndCommandBuffer(cmdBuff);

    VkSubmitInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    info.commandBufferCount = 1;
    info.pCommandBuffers = &cmdBuff;

    VK_CHECK(vkQueueSubmit(m_GraphicsQueue, 1, &info, fence));
    VK_CHECK(vkWaitForFences(m_Device, 1, &fence, VK_TRUE, UINT64_MAX));

    DestroyBuffer(stagingBuffer);
    vkDestroyFence(m_Device, fence, nullptr);
    FreeCommandBuffer(cmdBuff, m_GraphicsCmdPool);
}

void Application::CreateImage(Image& image, const ImageInfo& imgInfo)
{
    image.info = imgInfo;

    {
        VkImageCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        info.arrayLayers = 1;
        info.extent.width = imgInfo.width;
        info.extent.height = imgInfo.height;
        info.extent.depth = 1;
        info.format = imgInfo.format;
        info.imageType = VK_IMAGE_TYPE_2D;
        info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        info.mipLevels = 1;
        info.queueFamilyIndexCount = m_UniqueQueues.size();
        info.pQueueFamilyIndices = m_UniqueQueues.data();
        info.sharingMode = m_UniqueQueues.size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
        info.samples = VK_SAMPLE_COUNT_1_BIT;
        info.tiling = VK_IMAGE_TILING_OPTIMAL;
        info.usage = imgInfo.usage;

        VK_CHECK(vkCreateImage(m_Device, &info, nullptr, &image.image));
    }
    {
        VkMemoryRequirements memReq{};
        vkGetImageMemoryRequirements(m_Device, image.image, &memReq);

        VkMemoryAllocateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        info.allocationSize = memReq.size;
        info.memoryTypeIndex = FindMemoryType(m_PhysicalDevice, memReq.memoryTypeBits, imgInfo.memProps);
        
        VK_CHECK(vkAllocateMemory(m_Device, &info, nullptr, &image.mem));
        VK_CHECK(vkBindImageMemory(m_Device, image.image, image.mem, 0));
    }
    {
        VkImageViewCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        info.components = { VK_COMPONENT_SWIZZLE_IDENTITY };
        info.format = imgInfo.format;
        info.image = image.image;
        info.subresourceRange = {
            .aspectMask = imgInfo.aspectFlags,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        };
        info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        
        VK_CHECK(vkCreateImageView(m_Device, &info, nullptr, &image.view));
    }

    if(!imgInfo.gpuResource)
        return;
    
    {
        VkSamplerCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        info.anisotropyEnable = VK_FALSE;
        info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        info.compareEnable = VK_FALSE;
        info.compareOp = VK_COMPARE_OP_ALWAYS;
        info.minFilter = VK_FILTER_LINEAR;
        info.magFilter = VK_FILTER_LINEAR;
        info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        
        VK_CHECK(vkCreateSampler(m_Device, &info, nullptr, &image.sampler));
    }
}

void Application::DestroyImage(Image& image)
{
    vkDestroyImage(m_Device, image.image, nullptr);
    vkDestroyImageView(m_Device, image.view, nullptr);
    vkFreeMemory(m_Device, image.mem, nullptr);

    if(!image.info.gpuResource)
        return;
    vkDestroySampler(m_Device, image.sampler, nullptr);

    memset(&image, 0, sizeof(image));
}

VkFence Application::CreateFence() {
    VkFence fence = nullptr;
    VkFenceCreateInfo info = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    VK_CHECK(vkCreateFence(m_Device, &info, nullptr, &fence));
    return fence;
}

VkCommandBuffer Application::AllocateCommandBuffer(VkCommandPool pool) {
    VkCommandBufferAllocateInfo info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    info.commandBufferCount = 1;
    info.commandPool = pool;
    info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    
    VkCommandBuffer buff = nullptr;
    VK_CHECK(vkAllocateCommandBuffers(m_Device, &info, &buff));
    return buff;
}

void Application::FreeCommandBuffer(VkCommandBuffer buffer, VkCommandPool pool) {
    vkFreeCommandBuffers(m_Device, pool, 1, &buffer);
}

void Application::BeginCommandBuffer(VkCommandBuffer buffer, VkCommandBufferUsageFlagBits usage) {
    VkCommandBufferBeginInfo info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    info.flags = usage;

    VK_CHECK(vkBeginCommandBuffer(buffer, &info));
}
