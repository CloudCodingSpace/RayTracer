#include "RayTracer.h"

#include <cassert>
#include <string>
#include <cstring>
#include <algorithm>
#include <vector>

#include <vulkan/vk_enum_string_helper.h>

#define VK_CHECK(result) do { if(result != VK_SUCCESS) { printf("VkResult: %s (line: %d, file: %s\n", string_VkResult(result), __LINE__, __FILE__); assert(false); } } while(0);
#define CLAMP(value, min, max) ((value < min) ? min : (value > max) ? max : value)

static u8* ReadFile(std::string path, u64* size) {
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
}

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
                
                VkPhysicalDeviceScalarBlockLayoutFeatures scalarFeatures = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES };
                VkPhysicalDeviceFeatures2 features = {};
                features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
                features.pNext = &scalarFeatures;

                vkGetPhysicalDeviceFeatures2(device, &features);

                if(present && (gIdx != -1) && (pIdx != -1) && (cIdx != -1) && scalarFeatures.scalarBlockLayout) {
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

        for(i32 i = 0; i < 3; i++) 
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

        VkPhysicalDeviceScalarBlockLayoutFeatures scalarFeatures = {};
        scalarFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES;
        scalarFeatures.scalarBlockLayout = VK_TRUE;

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
            VK_CHECK(vkCreateSemaphore(m_Device, &semaInfo, nullptr, &m_ComputeFinished[i]));
        }
        
        m_RenderFinished.resize(m_ScImages.size());
        for(auto& sema : m_RenderFinished)
        {
            VK_CHECK(vkCreateSemaphore(m_Device, &semaInfo, nullptr, &sema));
        }
    }
    // Storage images
    {
        for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++)
        {
            {
                VkImageCreateInfo info = {};
                info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                info.arrayLayers = 1;
                info.extent = { m_StorageImages[i].width, m_StorageImages[i].height, 1 };
                info.format = VK_FORMAT_R8G8B8A8_UNORM;
                info.imageType = VK_IMAGE_TYPE_2D;
                info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                info.mipLevels = 1;
                info.sharingMode = m_UniqueQueues.size() == 1 ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;
                info.queueFamilyIndexCount = m_UniqueQueues.size();
                info.pQueueFamilyIndices = m_UniqueQueues.data();
                info.samples = VK_SAMPLE_COUNT_1_BIT;
                info.tiling = VK_IMAGE_TILING_OPTIMAL;
                info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;

                VK_CHECK(vkCreateImage(m_Device, &info, nullptr, &m_StorageImages[i].image));
            }
            {
                VkMemoryRequirements req = {};
                vkGetImageMemoryRequirements(m_Device, m_StorageImages[i].image, &req);

                VkMemoryAllocateInfo info = {};
                info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                info.allocationSize = req.size;
                info.memoryTypeIndex = FindMemoryType(m_PhysicalDevice, req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

                VK_CHECK(vkAllocateMemory(m_Device, &info, nullptr, &m_StorageImages[i].memory));
                VK_CHECK(vkBindImageMemory(m_Device, m_StorageImages[i].image, m_StorageImages[i].memory, 0));
            }
            {
                VkImageViewCreateInfo info = {};
                info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                info.components = { VK_COMPONENT_SWIZZLE_IDENTITY };
                info.format = VK_FORMAT_R8G8B8A8_UNORM;
                info.image = m_StorageImages[i].image;
                info.viewType = VK_IMAGE_VIEW_TYPE_2D;
                info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                info.subresourceRange.baseArrayLayer = 0;
                info.subresourceRange.baseMipLevel = 0;
                info.subresourceRange.levelCount = 1;
                info.subresourceRange.layerCount = 1;

                VK_CHECK(vkCreateImageView(m_Device, &info, nullptr, &m_StorageImages[i].view));
            }
            {
                VkSamplerCreateInfo info = {};
                info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
                info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
                info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
                info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
                info.anisotropyEnable = VK_FALSE;
                info.magFilter = VK_FILTER_LINEAR;
                info.minFilter = VK_FILTER_LINEAR;
                info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
                info.compareEnable = VK_FALSE;
                info.compareOp = VK_COMPARE_OP_ALWAYS;
                info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
                info.mipLodBias = 0.0f;
                info.minLod = 0.0f;
                info.maxLod = 0.0f;

                VK_CHECK(vkCreateSampler(m_Device, &info, nullptr, &m_StorageImages[i].sampler));
            }

            {
                VkFence fence = nullptr;
                {
                    VkFenceCreateInfo info = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
                    VK_CHECK(vkCreateFence(m_Device, &info, nullptr, &fence));
                }

                VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
                vkBeginCommandBuffer(m_GraphicsCmdBuffs[i], &beginInfo);

                VkImageMemoryBarrier barrier = {};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
                barrier.image = m_StorageImages[i].image;
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                barrier.subresourceRange.layerCount = 1;
                barrier.subresourceRange.levelCount = 1;
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

                vkCmdPipelineBarrier(m_GraphicsCmdBuffs[i], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                        0, 0, nullptr, 0, nullptr, 1, &barrier);

                vkEndCommandBuffer(m_GraphicsCmdBuffs[i]);

                VkSubmitInfo info = {}; 
                info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                info.commandBufferCount = 1;
                info.pCommandBuffers = &m_GraphicsCmdBuffs[i];

                VK_CHECK(vkQueueSubmit(m_GraphicsQueue, 1, &info, fence));
                VK_CHECK(vkWaitForFences(m_Device, 1, &fence, VK_TRUE, UINT64_MAX));

                vkResetCommandBuffer(m_GraphicsCmdBuffs[i], 0);
                vkDestroyFence(m_Device, fence, nullptr);
            }
        }
    }
    // Descriptors
    {
        {
            VkDescriptorSetLayoutBinding binding = {};
            binding.binding = 0;
            binding.descriptorCount = 1;
            binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

            VkDescriptorSetLayoutCreateInfo layInfo = {};
            layInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layInfo.bindingCount = 1;
            layInfo.pBindings = &binding;
            
            VK_CHECK(vkCreateDescriptorSetLayout(m_Device, &layInfo, nullptr, &m_DescLayout));
        }

        {
            VkDescriptorPoolSize size;
            size.descriptorCount = FRAMES_IN_FLIGHT;
            size.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;

            VkDescriptorPoolCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            info.maxSets = FRAMES_IN_FLIGHT;
            info.poolSizeCount = 1;
            info.pPoolSizes = &size;
            
            VK_CHECK(vkCreateDescriptorPool(m_Device, &info, nullptr, &m_DescPool));
        }

        {
            VkDescriptorSetAllocateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            info.descriptorPool = m_DescPool;
            info.descriptorSetCount = 1;
            info.pSetLayouts = &m_DescLayout;
            
            for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++)
                VK_CHECK(vkAllocateDescriptorSets(m_Device, &info, &m_Sets[i]));
        }

        {
            for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++)
            {
                VkDescriptorImageInfo imgInfo = {};
                imgInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                imgInfo.imageView = m_StorageImages[i].view;
                imgInfo.sampler = m_StorageImages[i].sampler;

                VkWriteDescriptorSet write = {};
                write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                write.descriptorCount = 1;
                write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                write.dstSet = m_Sets[i];
                write.pImageInfo = &imgInfo;
                write.dstBinding = 0;

                vkUpdateDescriptorSets(m_Device, 1, &write, 0, nullptr);
            }
        }
    }
    // Pipeline
    {
        {
            VkPushConstantRange range = {};
            range.offset = 0;
            range.size = sizeof(m_PushConstantData);
            range.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

            VkPipelineLayoutCreateInfo layInfo = {};
            layInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            layInfo.pushConstantRangeCount = 1;
            layInfo.pPushConstantRanges = &range;
            layInfo.setLayoutCount = 1;
            layInfo.pSetLayouts = &m_DescLayout;
            
            VK_CHECK(vkCreatePipelineLayout(m_Device, &layInfo, nullptr, &m_PipelineLayout));
        }

        
        VkShaderModuleCreateInfo modInfo = {};
        modInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

        u8* code = ReadFile("./assets/shaders/main.comp.spv", &modInfo.codeSize);
        modInfo.pCode = (const u32*)code;

        VkShaderModule mod = nullptr;
        VK_CHECK(vkCreateShaderModule(m_Device, &modInfo, nullptr, &mod));

        VkPipelineShaderStageCreateInfo stage = {};
        stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        stage.module = mod;
        stage.pName = "main";

        VkComputePipelineCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        info.basePipelineIndex = -1;
        info.layout = m_PipelineLayout;
        info.stage = stage;

        VK_CHECK(vkCreateComputePipelines(m_Device, nullptr, 1, &info, nullptr, &m_Pipeline));
        vkDestroyShaderModule(m_Device, mod, nullptr);
        delete[] code;
    }

    // UI descriptor pool
    {
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
    {
        IMGUI_CHECKVERSION();
		ImGui::CreateContext();

        ImGui::StyleColorsDark();

        ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		ImGuiStyle& style = ImGui::GetStyle();
		style.WindowPadding = ImVec2(0, 0);

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

        for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
            m_IgSets[i] = ImGui_ImplVulkan_AddTexture(m_StorageImages[i].sampler, m_StorageImages[i].view, VK_IMAGE_LAYOUT_GENERAL);
        }
    }
}

RayTracer::~RayTracer()
{
    VK_CHECK(vkDeviceWaitIdle(m_Device));

    for(auto& set : m_IgSets)
        ImGui_ImplVulkan_RemoveTexture(set);

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
    vkDestroyPipeline(m_Device, m_Pipeline, nullptr);
    
    vkDestroyDescriptorPool(m_Device, m_DescPool, nullptr);
    vkDestroyDescriptorSetLayout(m_Device, m_DescLayout, nullptr);

    vkDestroyDescriptorPool(m_Device, m_UiDescPool, nullptr);

    for(auto& image : m_StorageImages) 
    {
        vkDestroySampler(m_Device, image.sampler, nullptr);
        vkDestroyImageView(m_Device, image.view, nullptr);
        vkDestroyImage(m_Device, image.image, nullptr);
        vkFreeMemory(m_Device, image.memory, nullptr);
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
        // Delta time
        {
            double currentTime = glfwGetTime();
            m_DeltaTime = currentTime - m_LastTime;
            m_LastTime = currentTime;

            m_DeltaAccum += m_DeltaTime;
            if(m_DeltaAccum >= 0.2)
            {
                m_DisplayedDelta = m_DeltaTime * 1000;
                m_DeltaAccum = 0;
            }
        }

        if(m_ResizeImages) {
            ResizeImages(m_SceneSize.x, m_SceneSize.y);
            m_ResizeImages = false;
        }

        glfwGetFramebufferSize(m_Window, &m_Width, &m_Height);
        if(!StartFrame())
            continue;
        
        // Draw commands
        {
            m_PushConstantData.resolution[0] = m_StorageImages[m_FrameIdx].width;
            m_PushConstantData.resolution[1] = m_StorageImages[m_FrameIdx].height;

            const int localSizeX = 16;
            const int localSizeY = 16;

            vkCmdBindPipeline(m_ComputeCmdBuffs[m_FrameIdx], VK_PIPELINE_BIND_POINT_COMPUTE, m_Pipeline);
            vkCmdPushConstants(m_ComputeCmdBuffs[m_FrameIdx], m_PipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(m_PushConstantData), &m_PushConstantData);
            vkCmdBindDescriptorSets(m_ComputeCmdBuffs[m_FrameIdx], VK_PIPELINE_BIND_POINT_COMPUTE, m_PipelineLayout, 0, 1, &m_Sets[m_FrameIdx], 0, nullptr);

            u32 groupX = (m_StorageImages[m_FrameIdx].width + localSizeX - 1) / localSizeX;
            u32 groupY = (m_StorageImages[m_FrameIdx].height + localSizeY - 1) / localSizeY;

            vkCmdDispatch(m_ComputeCmdBuffs[m_FrameIdx], groupX, groupY, 1);
        }

        // Memory
        {
            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

            barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;

            barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

            barrier.image = m_StorageImages[m_FrameIdx].image;

            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;

            vkCmdPipelineBarrier(m_ComputeCmdBuffs[m_FrameIdx], VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        }

        // ImGui
        {
            ImGui::Begin("Scene");
            ImVec2 res = ImGui::GetContentRegionAvail();
            if((m_StorageImages[m_FrameIdx].width != res.x || m_StorageImages[m_FrameIdx].height != res.y) && res.x != 0 && res.y != 0) {
                m_SceneSize = res;
                m_ResizeImages = true;
            }
            
            ImGui::Image((ImTextureID)m_IgSets[m_FrameIdx], res, ImVec2(0, 1), ImVec2(1, 0));

            ImGui::End();

            ImGui::Begin("Settings");
            ImGui::TextColored(ImVec4(0, 255, 0, 255), "Delta Time: %.2fms", m_DisplayedDelta);
            ImGui::TextColored(ImVec4(0, 255, 0, 255), "Frame time: %.2f Hz", 1000/m_DisplayedDelta);
            ImGui::End();
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
    VK_CHECK(vkResetCommandBuffer(m_ComputeCmdBuffs[m_FrameIdx], 0));
    {
        VkCommandBufferBeginInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        VK_CHECK(vkBeginCommandBuffer(m_GraphicsCmdBuffs[m_FrameIdx], &info));
        VK_CHECK(vkBeginCommandBuffer(m_ComputeCmdBuffs[m_FrameIdx], &info));
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

    ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

    ImGui::DockSpaceOverViewport(ImGui::GetID("Dockspace"), ImGui::GetMainViewport());

    return true;
}

void RayTracer::EndFrame()
{
    ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_GraphicsCmdBuffs[m_FrameIdx], nullptr);

	ImGui::UpdatePlatformWindows();
	ImGui::RenderPlatformWindowsDefault();

    vkCmdEndRenderPass(m_GraphicsCmdBuffs[m_FrameIdx]);
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
            caps.extent.width = CLAMP(caps.extent.width, caps.caps.minImageExtent.width, caps.caps.maxImageExtent.width);
            caps.extent.height = CLAMP(caps.extent.height, caps.caps.minImageExtent.height, caps.caps.maxImageExtent.height);
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

void RayTracer::ResizeImages(u32 width, u32 height)
{
    VK_CHECK(vkDeviceWaitIdle(m_Device));

    for(auto& image : m_StorageImages) 
    {
        vkDestroySampler(m_Device, image.sampler, nullptr);
        vkDestroyImageView(m_Device, image.view, nullptr);
        vkDestroyImage(m_Device, image.image, nullptr);
        vkFreeMemory(m_Device, image.memory, nullptr);
    }

    for(auto& set : m_IgSets)
        ImGui_ImplVulkan_RemoveTexture(set);

    VkCommandBuffer buff = nullptr;
    {
        VkCommandBufferAllocateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        info.commandBufferCount = 1;
        info.commandPool = m_GraphicsCmdPool;
        info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        VK_CHECK(vkAllocateCommandBuffers(m_Device, &info, &buff));
    }

    VkFence fence = nullptr;
    {
        VkFenceCreateInfo info = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        VK_CHECK(vkCreateFence(m_Device, &info, nullptr, &fence));
    }

    VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    vkBeginCommandBuffer(buff, &beginInfo);

    // Creation
    for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++)
    {
        m_StorageImages[i].width = width;
        m_StorageImages[i].height = height;

        {
            VkImageCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            info.arrayLayers = 1;
            info.extent = { m_StorageImages[i].width, m_StorageImages[i].height, 1 };
            info.format = VK_FORMAT_R8G8B8A8_UNORM;
            info.imageType = VK_IMAGE_TYPE_2D;
            info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            info.mipLevels = 1;
            info.sharingMode = m_UniqueQueues.size() == 1 ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;
            info.queueFamilyIndexCount = m_UniqueQueues.size();
            info.pQueueFamilyIndices = m_UniqueQueues.data();
            info.samples = VK_SAMPLE_COUNT_1_BIT;
            info.tiling = VK_IMAGE_TILING_OPTIMAL;
            info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;

            VK_CHECK(vkCreateImage(m_Device, &info, nullptr, &m_StorageImages[i].image));
        }
        {
            VkMemoryRequirements req = {};
            vkGetImageMemoryRequirements(m_Device, m_StorageImages[i].image, &req);

            VkMemoryAllocateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            info.allocationSize = req.size;
            info.memoryTypeIndex = FindMemoryType(m_PhysicalDevice, req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            VK_CHECK(vkAllocateMemory(m_Device, &info, nullptr, &m_StorageImages[i].memory));
            VK_CHECK(vkBindImageMemory(m_Device, m_StorageImages[i].image, m_StorageImages[i].memory, 0));
        }
        {
            VkImageViewCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            info.components = { VK_COMPONENT_SWIZZLE_IDENTITY };
            info.format = VK_FORMAT_R8G8B8A8_UNORM;
            info.image = m_StorageImages[i].image;
            info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            info.subresourceRange.baseArrayLayer = 0;
            info.subresourceRange.baseMipLevel = 0;
            info.subresourceRange.levelCount = 1;
            info.subresourceRange.layerCount = 1;

            VK_CHECK(vkCreateImageView(m_Device, &info, nullptr, &m_StorageImages[i].view));
        }
        {
            VkSamplerCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            info.anisotropyEnable = VK_FALSE;
            info.magFilter = VK_FILTER_LINEAR;
            info.minFilter = VK_FILTER_LINEAR;
            info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            info.compareEnable = VK_FALSE;
            info.compareOp = VK_COMPARE_OP_ALWAYS;
            info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            info.mipLodBias = 0.0f;
            info.minLod = 0.0f;
            info.maxLod = 0.0f;

            VK_CHECK(vkCreateSampler(m_Device, &info, nullptr, &m_StorageImages[i].sampler));
        }

        {
            VkImageMemoryBarrier barrier = {};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
            barrier.image = m_StorageImages[i].image;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.layerCount = 1;
            barrier.subresourceRange.levelCount = 1;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

            vkCmdPipelineBarrier(buff, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    0, 0, nullptr, 0, nullptr, 1, &barrier);
        }
        {
            VkDescriptorImageInfo imgInfo = {};
            imgInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            imgInfo.imageView = m_StorageImages[i].view;
            imgInfo.sampler = m_StorageImages[i].sampler;

            VkWriteDescriptorSet write = {};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.descriptorCount = 1;
            write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            write.dstSet = m_Sets[i];
            write.pImageInfo = &imgInfo;
            write.dstBinding = 0;

            vkUpdateDescriptorSets(m_Device, 1, &write, 0, nullptr);
        }

        // ImGui sets
        m_IgSets[i] = ImGui_ImplVulkan_AddTexture(m_StorageImages[i].sampler, m_StorageImages[i].view, VK_IMAGE_LAYOUT_GENERAL);
    }

    vkEndCommandBuffer(buff);

    VkSubmitInfo info = {}; 
    info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    info.commandBufferCount = 1;
    info.pCommandBuffers = &buff;

    VK_CHECK(vkQueueSubmit(m_GraphicsQueue, 1, &info, fence));
    VK_CHECK(vkWaitForFences(m_Device, 1, &fence, VK_TRUE, UINT64_MAX));

    vkDestroyFence(m_Device, fence, nullptr);
    vkFreeCommandBuffers(m_Device, m_GraphicsCmdPool, 1, &buff);
}
