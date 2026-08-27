
#include "fluid_sim_window.hpp"
#include "enginez/graphics/ez_types.hpp"
#include "enginez/graphics/ez_vulkan_backend.hpp"
#include "enginez/utils/utilities.hpp"
#include "imgui.h"
#include "vulkan/vulkan_core.h"
#include <cmath>
#include <cstdint>
#include <vector>

using namespace std;

void FluidSimWindow::onUpdate() {
    auto cmd         = computeCommandBuffer.handle;
    auto graphicsCmd = graphicsCommandBuffer.handle;

    vkWaitForFences(device.handle, 1, &computeFence, VK_TRUE, 1000000000);
    vkResetFences(device.handle, 1, &computeFence);

    // ------------ compute command buffer begin ------------ //
    static VkCommandBufferBeginInfo computeCommandBufferBeginInfo {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext            = nullptr,
        .flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr,
    };
    vkResetCommandBuffer(cmd, 0);
    vkBeginCommandBuffer(cmd, &computeCommandBufferBeginInfo);

    // -------------- images to general layout -------------- //
    static VkImageMemoryBarrier2 barrier[3] {};
    static VkDependencyInfo depInfo;
    barrier[0] = {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask     = VK_PIPELINE_STAGE_2_NONE,
        .srcAccessMask    = VK_ACCESS_2_NONE_KHR,
        .dstStageMask     = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        .dstAccessMask    = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
        .oldLayout        = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout        = VK_IMAGE_LAYOUT_GENERAL,
        .image            = pressureMap.handle,
        .subresourceRange = ezVulkanBackend::SUBRESOURCE_WHOLE,
    };
    depInfo = {
        .sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers    = barrier,
    };
    vkCmdPipelineBarrier2(cmd, &depInfo);

    // --------------------- dispatches --------------------- //
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.handle);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.layout.handle, 0, 1, &mainDescriptorSet, 0, nullptr);
    vkCmdPushConstants(cmd, pipeline.layout.handle, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(Controls), &controls);
    vkCmdDispatch(cmd, std::ceil(SIM_BOUNDS.width / 16.f), std::ceil(SIM_BOUNDS.height / 16.f), 1);
    vkEndCommandBuffer(cmd);

    // -------------- submit to compute queueu -------------- //
    static VkCommandBufferSubmitInfo computeBufferSubmitInfo {
        .sType         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .pNext         = nullptr,
        .commandBuffer = cmd,
        .deviceMask    = 0,
    };
    static VkSemaphoreSubmitInfo computeSemaphoreSubmitInfo {
        .sType       = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .pNext       = nullptr,
        .semaphore   = computeSemaphore,
        .value       = 1,
        .stageMask   = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        .deviceIndex = 0,
    };
    static VkSubmitInfo2 computeSubmitInfo {
        .sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .pNext                    = nullptr,
        .flags                    = 0,
        .waitSemaphoreInfoCount   = 0,
        .pWaitSemaphoreInfos      = nullptr,
        .commandBufferInfoCount   = 1,
        .pCommandBufferInfos      = &computeBufferSubmitInfo,
        .signalSemaphoreInfoCount = 1,
        .pSignalSemaphoreInfos    = &computeSemaphoreSubmitInfo,
    };
    vkQueueSubmit2(computeQueue.handle, 1, &computeSubmitInfo, computeFence);

    // ------------ graphics command buffer begin ----------- //
    static VkCommandBufferBeginInfo graphicsCommandBufferBeginInfo {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext            = nullptr,
        .flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr,
    };
    vkResetCommandBuffer(graphicsCmd, 0);
    vkBeginCommandBuffer(graphicsCmd, &graphicsCommandBufferBeginInfo);

    // ------------- layout transitions for blit ------------ //
    barrier[0] = {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask     = VK_PIPELINE_STAGE_2_NONE,
        .srcAccessMask    = VK_ACCESS_2_NONE_KHR,
        .dstStageMask     = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .dstAccessMask    = VK_ACCESS_2_TRANSFER_READ_BIT,
        .oldLayout        = VK_IMAGE_LAYOUT_GENERAL,
        .newLayout        = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .image            = pressureMap.handle,
        .subresourceRange = ezVulkanBackend::SUBRESOURCE_WHOLE,
    };
    barrier[1] = {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask     = VK_PIPELINE_STAGE_2_NONE,
        .srcAccessMask    = VK_ACCESS_2_NONE_KHR,
        .dstStageMask     = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .dstAccessMask    = VK_ACCESS_2_TRANSFER_WRITE_BIT,
        .oldLayout        = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout        = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .image            = drawImage.handle,
        .subresourceRange = ezVulkanBackend::SUBRESOURCE_WHOLE,
    };
    depInfo = {
        .sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 2,
        .pImageMemoryBarriers    = barrier,
    };
    vkCmdPipelineBarrier2(graphicsCmd, &depInfo);

    // ------------------------ blit ------------------------ //
    static VkImageBlit2 blitRegion {
        .sType          = VK_STRUCTURE_TYPE_IMAGE_BLIT_2,
        .pNext          = nullptr,
        .srcSubresource = ezVulkanBackend::SUBRESOURCE_LAYERS_WHOLE,
        .dstSubresource = ezVulkanBackend::SUBRESOURCE_LAYERS_WHOLE
    };
    blitRegion.srcOffsets[1] = {.x = static_cast<int32_t>(SIM_BOUNDS.width), .y = static_cast<int32_t>(SIM_BOUNDS.height), .z = 1};
    blitRegion.dstOffsets[1] = {.x = static_cast<int32_t>(SIM_BOUNDS.width), .y = static_cast<int32_t>(SIM_BOUNDS.height), .z = 1};
    static VkBlitImageInfo2 blitInfo {
        .sType          = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
        .srcImage       = pressureMap.handle,
        .srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .dstImage       = drawImage.handle,
        .dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .regionCount    = 1,
        .pRegions       = &blitRegion,
        .filter         = VK_FILTER_LINEAR,
    };
    vkCmdBlitImage2(graphicsCmd, &blitInfo);
    vkEndCommandBuffer(graphicsCmd);

    // -------------- submit to graphics queue -------------- //
    static VkCommandBufferSubmitInfo graphicsBufferSubmitInfo {
        .sType         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .pNext         = nullptr,
        .commandBuffer = graphicsCmd,
        .deviceMask    = 0,
    };
    static VkSemaphoreSubmitInfo graphicsSemaphoreSubmitInfo {
        .sType       = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .pNext       = nullptr,
        .semaphore   = computeSemaphore,
        .value       = 1,
        .stageMask   = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .deviceIndex = 0,
    };
    static VkSubmitInfo2 graphicsSubmitInfo {
        .sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .pNext                    = nullptr,
        .flags                    = 0,
        .waitSemaphoreInfoCount   = 1,
        .pWaitSemaphoreInfos      = &graphicsSemaphoreSubmitInfo,
        .commandBufferInfoCount   = 1,
        .pCommandBufferInfos      = &graphicsBufferSubmitInfo,
        .signalSemaphoreInfoCount = 0,
        .pSignalSemaphoreInfos    = nullptr,
    };

    vkQueueSubmit2(graphicsQueue.handle, 1, &graphicsSubmitInfo, nullptr);

    ImGui::Begin("Controls");
    ImGui::SliderFloat("Brush size", &controls.brushSize, 0.1f, 100.f);
    ImGui::End();
}

void FluidSimWindow::onMouseMoved(double xpos, double ypos) {
    controls.brushPos.x = xpos;
    controls.brushPos.y = ypos;
};

void FluidSimWindow::onOpen() {
    createImages();
    createPipelines();
    createDescriptorPool();
    createDescriptorSets();
    createCommandPool();
    createCommandBuffer();
    createSynchObjects();
    assignDebugNames();
}

void FluidSimWindow::onClose() {
    engine.terminate();
}

void FluidSimWindow::createImages() {
    vector<uint32_t> familyIndices;
    familyIndices.push_back(graphicsQueue.family);
    if (graphicsQueue.family != computeQueue.family) familyIndices.push_back(computeQueue.family);

    VkImageCreateInfo imageCI {
        .sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext                 = nullptr,
        .flags                 = 0,
        .imageType             = VK_IMAGE_TYPE_2D,
        .format                = VK_FORMAT_R8G8B8A8_UNORM,
        .extent                = SIM_BOUNDS,
        .mipLevels             = 1,
        .arrayLayers           = 1,
        .samples               = VK_SAMPLE_COUNT_1_BIT,
        .tiling                = VK_IMAGE_TILING_OPTIMAL,
        .usage                 = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        .sharingMode           = familyIndices.size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = static_cast<uint32_t>(familyIndices.size()),
        .pQueueFamilyIndices   = familyIndices.data(),
        .initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    VmaAllocationCreateInfo allocationCI {
        .usage         = VMA_MEMORY_USAGE_GPU_ONLY,
        .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    };

    vmaCreateImage(allocator, &imageCI, &allocationCI, &pressureMap.handle, &pressureMap.allocation, nullptr);

    VkImageViewCreateInfo viewCI {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext            = nullptr,
        .flags            = 0,
        .image            = pressureMap.handle,
        .viewType         = VK_IMAGE_VIEW_TYPE_2D,
        .format           = VK_FORMAT_R8G8B8A8_UNORM,
        .components       = {},
        .subresourceRange = ezVulkanBackend::SUBRESOURCE_WHOLE,
    };

    vkCreateImageView(device.handle, &viewCI, nullptr, &pressureMap.view);

    pressureMap.extent = SIM_BOUNDS;
}
void FluidSimWindow::createPipelines() {
    VkDescriptorSetLayoutBinding binding {
        .binding         = 0,
        .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .descriptorCount = 1,
        .stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT,
    };
    VkPushConstantRange pushConstantRange {
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .offset     = 0,
        .size       = sizeof(Controls),
    };
    mainDescriptorSetLayout = backend.createDescriptorSetLayout(&binding, 1).value();
    auto pipelineLayout     = backend.createPipelineLayout(1, &mainDescriptorSetLayout, 1, &pushConstantRange).value();

    auto shader = backend.createShader("shaders/test.comp.spv").value();

    pipeline = backend.createComputePipeline(shader, pipelineLayout).value();
}

void FluidSimWindow::createDescriptorSets() {
    backend.allocateDescriptorSets(descriptorPool, 1, &mainDescriptorSetLayout, &mainDescriptorSet);

    VkDescriptorImageInfo imageInfo {
        .sampler     = nullptr,
        .imageView   = pressureMap.view,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
    };

    VkWriteDescriptorSet write {
        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet          = mainDescriptorSet,
        .dstBinding      = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .pImageInfo      = &imageInfo,
    };

    backend.updateDescriptorSets(1, &write, 0, nullptr);
}
void FluidSimWindow::createDescriptorPool() {
    vector<VkDescriptorPoolSize> poolSize = {
        {
            .type            = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 1,
        },
    };
    descriptorPool = backend.createDescriptorSetPool(poolSize).value();
}

void FluidSimWindow::createCommandPool() {
    computeCommandBufferPool  = backend.createCommandPool(computeQueue).value();
    graphicsCommandBufferPool = backend.createCommandPool(graphicsQueue).value();
}
void FluidSimWindow::createCommandBuffer() {
    computeCommandBuffer  = backend.allocateCommandBuffer(computeCommandBufferPool).value();
    graphicsCommandBuffer = backend.allocateCommandBuffer(graphicsCommandBufferPool).value();
}
void FluidSimWindow::createSynchObjects() {
    computeFence     = backend.createFence().value();
    computeSemaphore = backend.createSemaphore().value();
}

void FluidSimWindow::assignDebugNames() {
    setDebugName(device.handle, computeQueue.handle, VK_OBJECT_TYPE_QUEUE, "queue_compute");
    setDebugName(device.handle, graphicsQueue.handle, VK_OBJECT_TYPE_QUEUE, "queue_graphics");
    setDebugName(device.handle, presentQueue.handle, VK_OBJECT_TYPE_QUEUE, "queue_present");

    setDebugName(device.handle, pressureMap.handle, VK_OBJECT_TYPE_IMAGE, "pressure_map");

    // setDebugName(device.handle, renderSemaphore, VK_OBJECT_TYPE_SEMAPHORE, "sem_render");
    setDebugName(device.handle, computeSemaphore, VK_OBJECT_TYPE_SEMAPHORE, "sem_compute");

    setDebugName(device.handle, computeFence, VK_OBJECT_TYPE_FENCE, "fen_compute");

    setDebugName(device.handle, computeCommandBuffer.handle, VK_OBJECT_TYPE_COMMAND_BUFFER, "computeBuffer");
    setDebugName(device.handle, graphicsCommandBuffer.handle, VK_OBJECT_TYPE_COMMAND_BUFFER, "graphicsBuffer");
}