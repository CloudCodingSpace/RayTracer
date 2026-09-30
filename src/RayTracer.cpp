#include "RayTracer.h"

RayTracer::RayTracer() : Application("RayTracer", 1280, 720)
{
    // Camera
    m_Camera.Create(this);

    // Spheres
    {
        m_Spheres.push_back({ {  0, 0, 2 }, 0.5 });
        m_Spheres.push_back({ { -2, 0, 2 }, 0.5 });
        m_Spheres.push_back({ {  2, 0, 2 }, 0.5 });
    }

    // Buffers
    {
        // Camera buffer
        {
            BufferInfo info{};
            info.bda = true;
            info.size = sizeof(CameraBufferData);
            info.memProps = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

            for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
                CreateBuffer(m_CameraBuffer[i], info);
                VK_CHECK(vkMapMemory(m_Device, m_CameraBuffer[i].memory, 0, sizeof(info.size), 0, &m_CameraBufferMappedMem[i]));
                
                VkBufferDeviceAddressInfo bdaInfo{};
                bdaInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
                bdaInfo.buffer = m_CameraBuffer[i].buffer;
                m_CameraBufferAddress[i] = vkGetBufferDeviceAddress(m_Device, &bdaInfo);
            }
        }

        // Sphere buffer
        {
            BufferInfo info{};
            info.bda = true;
            info.size = sizeof(u32) + sizeof(Sphere) * m_Spheres.size();
            info.memProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

            CreateBuffer(m_SphereBuffer, info);

            void* data = calloc(1, info.size);
            u32 sphereCount = m_Spheres.size();
            memcpy(data, &sphereCount, sizeof(u32));
            memcpy(((u8*)data) + sizeof(u32), m_Spheres.data(), info.size - sizeof(u32));

            UploadDataToBuffer(m_SphereBuffer, data);

            free(data);

            VkBufferDeviceAddressInfo bdaInfo{};
            bdaInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
            bdaInfo.buffer = m_SphereBuffer.buffer;
            m_SphereBufferAddress = vkGetBufferDeviceAddress(m_Device, &bdaInfo);
        }

        // Buffer refs buffer
        {        
            BufferInfo info{};
            info.bda = true;
            info.size = sizeof(BufferRefsData);
            info.memProps = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

            for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
                CreateBuffer(m_BufferRefs[i], info);
                VK_CHECK(vkMapMemory(m_Device, m_BufferRefs[i].memory, 0, sizeof(info.size), 0, &m_BufferRefsMappedMem[i]));
                
                VkBufferDeviceAddressInfo bdaInfo{};
                bdaInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
                bdaInfo.buffer = m_BufferRefs[i].buffer;
                m_BufferRefsAddress[i] = vkGetBufferDeviceAddress(m_Device, &bdaInfo);
            }
        }
    }

    // Storage images
    {
        VkFence fence = CreateFence();

        BeginCommandBuffer(m_GraphicsCmdBuffs[0], VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        u32 width = m_Width, height = m_Height;
        for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++)
        {
            ImageInfo imgInfo{};
            imgInfo.width = width;
            imgInfo.height = height;
            imgInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
            imgInfo.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
            imgInfo.gpuResource = true;
            imgInfo.memProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            imgInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

            CreateImage(m_StorageImages[i], imgInfo);

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

                vkCmdPipelineBarrier(m_GraphicsCmdBuffs[0], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                        0, 0, nullptr, 0, nullptr, 1, &barrier);
            }
        }

        vkEndCommandBuffer(m_GraphicsCmdBuffs[0]);

        VkSubmitInfo info = {}; 
        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        info.commandBufferCount = 1;
        info.pCommandBuffers = &m_GraphicsCmdBuffs[0];

        VK_CHECK(vkQueueSubmit(m_GraphicsQueue, 1, &info, fence));
        VK_CHECK(vkWaitForFences(m_Device, 1, &fence, VK_TRUE, UINT64_MAX));

        vkResetCommandBuffer(m_GraphicsCmdBuffs[0], 0);
        vkDestroyFence(m_Device, fence, nullptr);
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

    for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
        m_IgSets[i] = ImGui_ImplVulkan_AddTexture(m_StorageImages[i].sampler, m_StorageImages[i].view, VK_IMAGE_LAYOUT_GENERAL);
    }
}

RayTracer::~RayTracer()
{
    VK_CHECK(vkDeviceWaitIdle(m_Device));

    vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
    vkDestroyPipeline(m_Device, m_Pipeline, nullptr);
    
    vkDestroyDescriptorPool(m_Device, m_DescPool, nullptr);

    DestroyBuffer(m_SphereBuffer);

    for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
        vkUnmapMemory(m_Device, m_CameraBuffer[i].memory);
        DestroyBuffer(m_CameraBuffer[i]);
        vkUnmapMemory(m_Device, m_BufferRefs[i].memory);
        DestroyBuffer(m_BufferRefs[i]);
    }

    for(auto& set : m_IgSets)
        ImGui_ImplVulkan_RemoveTexture(set);

    for(auto& image : m_StorageImages) 
        DestroyImage(image); 
    vkDestroyDescriptorSetLayout(m_Device, m_DescLayout, nullptr);
}

void RayTracer::Run()
{
    glfwShowWindow(m_Window);
    while(!glfwWindowShouldClose(m_Window))
    {
        // Delta time
        {
            m_DtIdx = (m_DtIdx + 1) % DT_SAMPLES;

            double currentTime = glfwGetTime() * 1000;
            double dt = currentTime - m_LastTime;
            m_LastTime = currentTime;

            m_DeltaTime[m_DtIdx] = dt;
            for(u32 i = 0; i < DT_SAMPLES; i++) {
                m_DisplayedDelta += m_DeltaTime[i];
            }
            m_DisplayedDelta /= DT_SAMPLES;
        }

        if(m_ResizeImages) {
            ResizeImages(m_SceneSize.x, m_SceneSize.y);
            m_ResizeImages = false;
        }

        glfwGetFramebufferSize(m_Window, &m_Width, &m_Height);
        if(!StartFrame())
            continue;
            
        // Update
        {
            m_Camera.Update(m_DisplayedDelta);
            CameraBufferData cbData{};
            cbData.fov = m_Camera.GetFOV();
            cbData.front = m_Camera.GetFront();
            cbData.position = m_Camera.GetPos();
            cbData.right = m_Camera.GetRight();
            cbData.up = m_Camera.GetUp();

            memcpy(m_CameraBufferMappedMem[m_FrameIdx], &cbData, sizeof(CameraBufferData));

            BufferRefsData data{};
            data.cameraBuffer = m_CameraBufferAddress[m_FrameIdx];
            data.sphereBuffer = m_SphereBufferAddress;
            memcpy(m_BufferRefsMappedMem[m_FrameIdx], &data, sizeof(BufferRefsData));
        }

        // Main rendering
        {
            // Draw commands
            {
                m_PushConstantData.resolution[0] = m_StorageImages[m_FrameIdx].info.width;
                m_PushConstantData.resolution[1] = m_StorageImages[m_FrameIdx].info.height;
                m_PushConstantData.aspectRatio = m_PushConstantData.resolution[0]/m_PushConstantData.resolution[1];
                m_PushConstantData.bufferRefs = m_BufferRefsAddress[m_FrameIdx];

                const int localSizeX = 16;
                const int localSizeY = 16;

                vkCmdBindPipeline(m_ComputeCmdBuffs[m_FrameIdx], VK_PIPELINE_BIND_POINT_COMPUTE, m_Pipeline);
                vkCmdPushConstants(m_ComputeCmdBuffs[m_FrameIdx], m_PipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(m_PushConstantData), &m_PushConstantData);
                vkCmdBindDescriptorSets(m_ComputeCmdBuffs[m_FrameIdx], VK_PIPELINE_BIND_POINT_COMPUTE, m_PipelineLayout, 0, 1, &m_Sets[m_FrameIdx], 0, nullptr);

                u32 groupX = (m_StorageImages[m_FrameIdx].info.width + localSizeX - 1) / localSizeX;
                u32 groupY = (m_StorageImages[m_FrameIdx].info.height + localSizeY - 1) / localSizeY;

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
                if((m_StorageImages[m_FrameIdx].info.width != res.x || m_StorageImages[m_FrameIdx].info.height != res.y) && res.x != 0 && res.y != 0) {
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
        }
        
        if(glfwGetKey(m_Window, GLFW_KEY_UP) == GLFW_PRESS && m_Camera.GetFOV() < 180)
            m_Camera.GetFOV() += 0.02 * m_DisplayedDelta;
        else if(glfwGetKey(m_Window, GLFW_KEY_DOWN) == GLFW_PRESS)
            m_Camera.GetFOV() -= 0.02 * m_DisplayedDelta;

        if(glfwGetKey(m_Window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            break;

        glfwPollEvents();
    }
}

void RayTracer::ResizeImages(u32 width, u32 height)
{
    VK_CHECK(vkDeviceWaitIdle(m_Device));

    for(auto& image : m_StorageImages)
        DestroyImage(image);

    for(auto& set : m_IgSets)
        ImGui_ImplVulkan_RemoveTexture(set);

    VkCommandBuffer buff = AllocateCommandBuffer(m_GraphicsCmdPool);

    VkFence fence = CreateFence();

    BeginCommandBuffer(buff, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    // Creation
    for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++)
    {
        ImageInfo imgInfo{};
        imgInfo.width = width;
        imgInfo.height = height;
        imgInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        imgInfo.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
        imgInfo.gpuResource = true;
        imgInfo.memProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        imgInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        CreateImage(m_StorageImages[i], imgInfo);

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
    FreeCommandBuffer(buff, m_GraphicsCmdPool);
}
