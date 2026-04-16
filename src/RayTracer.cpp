#include "RayTracer.h"

#include <cassert>
#include <string>
#include <cstring>
#include <algorithm>
#include <vector>

#include <vulkan/vk_enum_string_helper.h>

#define VK_CHECK(result) do { if(result != VK_SUCCESS) { printf("VkResult: %s (line: %d, file: %s\n", string_VkResult(result), __LINE__, __FILE__); assert(false); } } while(0);

RayTracer::RayTracer() : m_Width{800}, m_Height{600}
{
    // Window
    {
        assert(glfwInit() && "Failed to initialize glfw!");
        glfwWindowHint(GLFW_VISIBLE, false);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        
        m_Window = glfwCreateWindow(m_Width, m_Height, "RayTracer", nullptr, nullptr);
        assert(m_Window && "Failed to create the window!");
    }
    // Instance
    {
        u32 extCount = 0;
        const char** exts = glfwGetRequiredInstanceExtensions(&extCount);

        VkApplicationInfo appInfo = {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = "RayTracer",
            .applicationVersion = VK_MAKE_VERSION(2, 0, 0),
            .pEngineName = "RayTracer",
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = VK_API_VERSION_1_0
        };

        VkInstanceCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &appInfo,
            .enabledExtensionCount = extCount,
            .ppEnabledExtensionNames = exts
        };

        VK_CHECK(vkCreateInstance(&info, nullptr, &m_Instance));
    }
    // Surface
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
                VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_Surface, &present));
                if(present)
                    pIdx = i;
                
                if(present && (gIdx != -1) && (pIdx != -1) && (cIdx != -1)) {
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
        {
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
        }
        assert(extsSupported && "The device extensions aren't supported!");

        float priority = 1.0f;
        i32 indices[3] = {
            m_GraphicsQueueIdx,
            m_PresentQueueIdx,
            m_ComputeQueueIdx
        };
        std::vector<i32> uniqueQueues;

        for(i32 i = 0; i < 3; i++) 
        {
            bool exists = false;
            for(i32 idx : uniqueQueues) 
            {
                if(idx == indices[i])
                    exists = true;
            }
            if(!exists)
                uniqueQueues.push_back(indices[i]);
        }

        std::vector<VkDeviceQueueCreateInfo> queueInfos;
        for(i32 idx : uniqueQueues)
        {
            VkDeviceQueueCreateInfo info = {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = (u32)idx,
                .queueCount = 1,
                .pQueuePriorities = &priority
            };
            queueInfos.push_back(info);
        }

        VkDeviceCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .queueCreateInfoCount = (u32)queueInfos.size(),
            .pQueueCreateInfos = queueInfos.data(),
            .enabledExtensionCount = (u32)exts.size(),
            .ppEnabledExtensionNames = exts.data(),
            .pEnabledFeatures = &m_PhysicalDeviceFeatures
        };

        VK_CHECK(vkCreateDevice(m_PhysicalDevice, &info, nullptr, &m_Device));

        vkGetDeviceQueue(m_Device, m_GraphicsQueueIdx, 0, &m_GraphicsQueue);
        vkGetDeviceQueue(m_Device, m_PresentQueueIdx, 0, &m_PresentQueue);
        vkGetDeviceQueue(m_Device, m_ComputeQueueIdx, 0, &m_ComputeQueue);
    }
    
    // Renderpass
    {
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
                VkCommandBufferAllocateInfo info{};
                info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                info.commandBufferCount = 1;
                info.commandPool = m_GraphicsCmdPool;
                info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                
                VK_CHECK(vkAllocateCommandBuffers(m_Device, &info, &m_GraphicsCmdBuffs[i]));
                info.commandPool = m_ComputeCmdPool;
                VK_CHECK(vkAllocateCommandBuffers(m_Device, &info, &m_ComputeCmdBuffs[i]));
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
        }
        
        m_RenderFinished.resize(m_ScImages.size());
        for(auto& sema : m_RenderFinished)
        {
            VK_CHECK(vkCreateSemaphore(m_Device, &semaInfo, nullptr, &sema));
        }
    }
}

RayTracer::~RayTracer()
{
    VK_CHECK(vkDeviceWaitIdle(m_Device));

    for(auto& fence : m_InFlightFences)
        vkDestroyFence(m_Device, fence, nullptr);
    for(auto& sema : m_ImageAvailable)
        vkDestroySemaphore(m_Device, sema, nullptr);
    for(auto& sema : m_RenderFinished)
        vkDestroySemaphore(m_Device, sema, nullptr);

    vkDestroyCommandPool(m_Device, m_GraphicsCmdPool, nullptr);
    vkDestroyCommandPool(m_Device, m_ComputeCmdPool, nullptr);

    for(auto& fb : m_Framebuffers)
        vkDestroyFramebuffer(m_Device, fb, nullptr);
    for(auto& view : m_ScImageViews)
        vkDestroyImageView(m_Device, view, nullptr);

    vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);
    vkDestroyRenderPass(m_Device, m_Pass, nullptr);
    vkDestroyDevice(m_Device, nullptr);
    vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
    vkDestroyInstance(m_Instance, nullptr);

    glfwDestroyWindow(m_Window);
    glfwTerminate();
}

void RayTracer::Run()
{
    glfwShowWindow(m_Window);
    while(!glfwWindowShouldClose(m_Window))
    {
        glfwGetFramebufferSize(m_Window, &m_Width, &m_Height);
        if(!StartFrame())
            continue;
        
        // Draw commands
        {

        }
        
        EndFrame();
        
        if(glfwGetKey(m_Window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            break;

        glfwPollEvents();
    }
}

bool RayTracer::StartFrame()
{
    VK_CHECK(vkWaitForFences(m_Device, 1, &m_InFlightFences[m_FrameIdx], true, UINT64_MAX));
    
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
    
    VK_CHECK(vkResetFences(m_Device, 1, &m_InFlightFences[m_FrameIdx]));
    VK_CHECK(vkResetCommandBuffer(m_GraphicsCmdBuffs[m_FrameIdx], 0));
    {
        VkCommandBufferBeginInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        VK_CHECK(vkBeginCommandBuffer(m_GraphicsCmdBuffs[m_FrameIdx], &info));
    }

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

    return true;
}

void RayTracer::EndFrame()
{
    vkCmdEndRenderPass(m_GraphicsCmdBuffs[m_FrameIdx]);
    VK_CHECK(vkEndCommandBuffer(m_GraphicsCmdBuffs[m_FrameIdx]));

    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_GraphicsCmdBuffs[m_FrameIdx];
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &m_ImageAvailable[m_FrameIdx];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &m_RenderFinished[m_ImageIdx];
    
    VK_CHECK(vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, m_InFlightFences[m_FrameIdx]));

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

    m_FrameIdx = (m_FrameIdx + 1) % FRAMES_IN_FLIGHT;
}

ScCaps RayTracer::GetScCaps()
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
            caps.extent.width = glm::clamp(caps.extent.width, caps.caps.minImageExtent.width, caps.caps.maxImageExtent.width);
            caps.extent.height = glm::clamp(caps.extent.height, caps.caps.minImageExtent.height, caps.caps.maxImageExtent.height);
        }
    }

    return caps;
}

void RayTracer::CreateSwapchain()
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

void RayTracer::Resize()
{
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

    m_ScCaps = GetScCaps();
    CreateSwapchain();

    VkSemaphoreCreateInfo semaInfo{};
    semaInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    m_RenderFinished.resize(m_ScImages.size());
    for(auto& sema : m_RenderFinished)
        VK_CHECK(vkCreateSemaphore(m_Device, &semaInfo, nullptr, &sema));
    for(auto& sema : m_ImageAvailable)
        VK_CHECK(vkCreateSemaphore(m_Device, &semaInfo, nullptr, &sema));
}