
#include "fluid_sim_window.hpp"
#include "enginez/graphics/ez_descriptor_set_layout_builder.hpp"
#include "enginez/graphics/ez_image_builder.hpp"
#include "enginez/graphics/ez_pipeline_layout_builder.hpp"
#include "enginez/graphics/ez_types.hpp"
#include "enginez/graphics/ez_vulkan_backend.hpp"
#include "enginez/utils/utilities.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "vulkan/vulkan_core.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

using namespace std;

void FluidSimWindow::draw(Image& drawImage, uint64_t deltaTime) {
    auto cmd         = computeCommandBuffer[currentFrame].handle;
    auto graphicsCmd = graphicsCommandBuffer.handle;

    vkWaitForFences(device.handle, 1, &computeFence[currentFrame], VK_TRUE, 1000000000);
    vkResetFences(device.handle, 1, &computeFence[currentFrame]);

    // ------------ compute command buffer begin ------------ //
    VkCommandBufferBeginInfo computeCommandBufferBeginInfo {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext            = nullptr,
        .flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr,
    };
    vkResetCommandBuffer(cmd, 0);
    vkBeginCommandBuffer(cmd, &computeCommandBufferBeginInfo);

    // ---------------- descriptor set update --------------- //
    VkDescriptorImageInfo imageInfo[1] = {VkDescriptorImageInfo {
        .sampler     = nullptr,
        .imageView   = drawImage.view,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
    }};

    VkWriteDescriptorSet write[3] = {
        VkWriteDescriptorSet {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = computeDS,
            .dstBinding      = 3,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .pImageInfo      = imageInfo,
        },
    };
    backend.updateDescriptorSets(1, write, 0, nullptr);

    // -------------- images to general layout -------------- //
    VkImageMemoryBarrier2 imageBarrier[10] {};
    VkMemoryBarrier2      memoryBarrier[3] {};
    VkDependencyInfo      depInfo;
    imageBarrier[0] = {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask     = VK_PIPELINE_STAGE_2_NONE,
        .srcAccessMask    = VK_ACCESS_2_NONE_KHR,
        .dstStageMask     = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        .dstAccessMask    = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
        .oldLayout        = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout        = VK_IMAGE_LAYOUT_GENERAL,
        .image            = drawImage.handle,
        .subresourceRange = ezVulkanBackend::SUBRESOURCE_WHOLE,
    };
    depInfo = {
        .sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers    = imageBarrier,
    };
    vkCmdPipelineBarrier2(cmd, &depInfo);

    // --------------------- dispatches --------------------- //

    controls.drawBounds.x = swapchainExtent.width;
    controls.drawBounds.y = swapchainExtent.height;
    controls.cellSize     = std::min(float(swapchainExtent.width) / controls.simBounds.x, float(swapchainExtent.height) / controls.simBounds.y);

    blitImages(cmd);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, brushPipeline.layout.handle, 0, 1, &computeDS, 0, nullptr);
    vkCmdPushConstants(cmd, brushPipeline.layout.handle, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(Controls), &controls);

    if (!paused || (shouldUpdate && !updated)) {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, advectPipeline.handle);
        vkCmdDispatch(cmd, std::ceil(SIM_BOUNDS.width / 16.f), std::ceil(SIM_BOUNDS.height / 16.f), 1);
    }

    blitImages(cmd);

    memoryBarrier[0] = {
        .sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
        .pNext         = nullptr,
        .srcStageMask  = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        .dstStageMask  = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
    };
    depInfo = {
        .sType              = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .memoryBarrierCount = 1,
        .pMemoryBarriers    = memoryBarrier,
    };

    if (!paused || (shouldUpdate && !updated)) {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, diffusePipeline.handle);
        vkCmdPipelineBarrier2(cmd, &depInfo);
        vkCmdDispatch(cmd, std::ceil(SIM_BOUNDS.width / 16.f), std::ceil(SIM_BOUNDS.height / 16.f), 1);
    }

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, brushPipeline.handle);
    vkCmdPipelineBarrier2(cmd, &depInfo);
    vkCmdDispatch(cmd, std::ceil(SIM_BOUNDS.width / 16.f), std::ceil(SIM_BOUNDS.height / 16.f), 1);

    if (!paused || (shouldUpdate && !updated)) {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, preProcessPipeline.handle);
        vkCmdPipelineBarrier2(cmd, &depInfo);
        vkCmdDispatch(cmd, std::ceil(SIM_BOUNDS.width / 16.f), std::ceil(SIM_BOUNDS.height / 16.f), 1);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, projectPipeline.handle);
        for (int i = 0; i < iterations; i++) {
            controls.redBlackIdx = 0;
            vkCmdPushConstants(cmd, brushPipeline.layout.handle, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(Controls), &controls);
            vkCmdPipelineBarrier2(cmd, &depInfo);
            vkCmdDispatch(cmd, std::ceil(SIM_BOUNDS.width / 32.f), std::ceil(SIM_BOUNDS.height / 16.f), 1);

            controls.redBlackIdx = 1;
            vkCmdPushConstants(cmd, brushPipeline.layout.handle, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(Controls), &controls);
            vkCmdPipelineBarrier2(cmd, &depInfo);
            vkCmdDispatch(cmd, std::ceil(SIM_BOUNDS.width / 32.f), std::ceil(SIM_BOUNDS.height / 16.f), 1);
        }

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, velocityUpdatePipeline.handle);
        vkCmdPipelineBarrier2(cmd, &depInfo);
        vkCmdDispatch(cmd, std::ceil(SIM_BOUNDS.width / 16.f), std::ceil(SIM_BOUNDS.height / 16.f), 1);
    }
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, visualizePipeline.handle);
    vkCmdPipelineBarrier2(cmd, &depInfo);
    vkCmdDispatch(cmd, std::ceil(drawImage.extent.width / 16.f), std::ceil(drawImage.extent.height / 16.f), 1);

    if (shouldUpdate && !updated) updated = true;

    if (shouldClear) {
        clearImages(cmd);
        shouldClear = false;
    }

    vkEndCommandBuffer(cmd);

    // -------------- submit to compute queueu -------------- //
    VkCommandBufferSubmitInfo computeBufferSubmitInfo {
        .sType         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .pNext         = nullptr,
        .commandBuffer = cmd,
        .deviceMask    = 0,
    };
    VkSemaphoreSubmitInfo computeSemaphoreSubmitInfo {
        .sType       = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .pNext       = nullptr,
        .semaphore   = computeSemaphore[currentFrame],
        .value       = 1,
        .stageMask   = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        .deviceIndex = 0,
    };
    VkSubmitInfo2 computeSubmitInfo {
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
    vkQueueSubmit2(computeQueue.handle, 1, &computeSubmitInfo, computeFence[currentFrame]);

    // ------------ graphics command buffer begin ----------- //
    VkCommandBufferBeginInfo graphicsCommandBufferBeginInfo {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext            = nullptr,
        .flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr,
    };
    vkResetCommandBuffer(graphicsCmd, 0);
    vkBeginCommandBuffer(graphicsCmd, &graphicsCommandBufferBeginInfo);
    vkEndCommandBuffer(graphicsCmd);

    // -------------- submit to graphics queue -------------- //
    VkCommandBufferSubmitInfo graphicsBufferSubmitInfo {
        .sType         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .pNext         = nullptr,
        .commandBuffer = graphicsCmd,
        .deviceMask    = 0,
    };
    VkSemaphoreSubmitInfo graphicsSemaphoreSubmitInfo {
        .sType       = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .pNext       = nullptr,
        .semaphore   = computeSemaphore[currentFrame],
        .value       = 1,
        .stageMask   = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .deviceIndex = 0,
    };
    VkSubmitInfo2 graphicsSubmitInfo {
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

    // ------------------------- ui ------------------------- //

    ImGui::Begin("Visualization");
    ImGui::SliderFloat("Visualization scale", &controls.visScale, 0.001f, 1000.f);
    if (ImGui::BeginCombo("Visualization", VISUALIZATION_TYPE_STRING[controls.visType])) {
        for (auto i = 0; i < VISUALIZATION_TYPE_STRING.size(); i++) {
            if (ImGui::Selectable(VISUALIZATION_TYPE_STRING[i])) {
                controls.visType = i;
            }
        }
        ImGui::EndCombo();
    }
    ImGui::End();

    ImGui::Begin("Brush");
    ImGui::SliderFloat("Brush size", &controls.brushSize, 0.1f, 100.f);
    if (ImGui::BeginCombo("Brush", BRUSH_TYPE_STRING[controls.brushType])) {
        for (auto i = 0; i < BRUSH_TYPE_STRING.size(); i++) {
            if (ImGui::Selectable(BRUSH_TYPE_STRING[i])) {
                controls.brushType = i;
            }
        }
        ImGui::EndCombo();
    }
    if (controls.brushType == BRUSH_SMOKE) {
        ImGui::ColorPicker3("Smoke color", (float*)&controls.cellData);
    } else if (controls.brushType == BRUSH_CELL) {
        if (ImGui::BeginCombo("Type", CELL_TYPE_STRING[controls.cellType])) {
            for (auto i = 0; i < CELL_TYPE_STRING.size(); i++) {
                if (ImGui::Selectable(CELL_TYPE_STRING[i])) {
                    controls.cellType = i;
                }
            }
            ImGui::EndCombo();
        }

        if (controls.cellType == CELL_PRESSURE) {
            ImGui::InputFloat("Pressure", &controls.cellData[0]);
        } else if (controls.cellType == CELL_VELOCITY) {
            ImGui::InputFloat("Horizontal Speed", &controls.cellData[0]);
            ImGui::InputFloat("Vertical Speed", &controls.cellData[1]);
        } else if (controls.cellType == CELL_SMOKE) {
            ImGui::ColorPicker3("Smoke color", (float*)&controls.cellData);
        }
    }
    ImGui::End();
    ImGui::Begin("Controls");
    if (ImGui::Button("update")) {
        shouldUpdate = true;
    } else {
        shouldUpdate = false;
        updated      = false;
    }
    ImGui::Checkbox("paused", &paused);
    ImGui::InputInt("Iterations", &iterations);
    ImGui::InputFloat("Over relaxation", &controls.overRelaxation);
    ImGui::InputFloat("Smoke diffusion", &controls.smokeDiffuse);
    ImGui::InputFloat("Density", &controls.density);
    ImGui::Checkbox("Open edges", (bool*)&controls.openEdges);
    if (ImGui::Button("clear")) {
        shouldClear = true;
    }
    ImGui::End();

    ImGui::SetNextWindowPos({10, 10});
    // ImGui::SetNextWindowSize({200, 20});
    ImGui::Begin("Details", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("Frame Time: %.6fs", deltaTime / 1.0e9);
    ImGui::Text("Frame Rate: %d", (int)(1.0e9 / deltaTime));
    ImGui::End();

    currentFrame = (currentFrame + 1) % FRAMES_IN_FLY;
}

void FluidSimWindow::onMouseMoved(double xpos, double ypos) {
    controls.brushDelta.x = (xpos) / controls.cellSize - controls.brushPos.x;
    controls.brushDelta.y = (swapchainExtent.height - ypos) / controls.cellSize - controls.brushPos.y;

    controls.brushPos.x = xpos;
    controls.brushPos.y = (swapchainExtent.height - ypos);

    controls.brushPos /= controls.cellSize;
};
void FluidSimWindow::onMouseDown(int button, int action, int mods) {
    ImGuiIO& io = ImGui::GetIO();
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        controls.brushDown = false;
        return;
    }

    if (io.WantCaptureMouse) {
        return;
    }

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        controls.brushDown = true;
    }
}

void FluidSimWindow::blitImages(VkCommandBuffer cmd) {
    VkImageMemoryBarrier2 imageBarrier[10] {};
    VkDependencyInfo      depInfo;
    imageBarrier[0] = {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask     = VK_PIPELINE_STAGE_2_NONE,
        .srcAccessMask    = VK_ACCESS_2_NONE_KHR,
        .dstStageMask     = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        .dstAccessMask    = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
        .oldLayout        = VK_IMAGE_LAYOUT_GENERAL,
        .newLayout        = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .image            = velocityXMap.handle,
        .subresourceRange = ezVulkanBackend::SUBRESOURCE_WHOLE,
    };
    imageBarrier[1] = {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask     = VK_PIPELINE_STAGE_2_NONE,
        .srcAccessMask    = VK_ACCESS_2_NONE_KHR,
        .dstStageMask     = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        .dstAccessMask    = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
        .oldLayout        = VK_IMAGE_LAYOUT_GENERAL,
        .newLayout        = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .image            = velocityYMap.handle,
        .subresourceRange = ezVulkanBackend::SUBRESOURCE_WHOLE,
    };
    imageBarrier[2] = {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask     = VK_PIPELINE_STAGE_2_NONE,
        .srcAccessMask    = VK_ACCESS_2_NONE_KHR,
        .dstStageMask     = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        .dstAccessMask    = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
        .oldLayout        = VK_IMAGE_LAYOUT_GENERAL,
        .newLayout        = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .image            = smokeMap.handle,
        .subresourceRange = ezVulkanBackend::SUBRESOURCE_WHOLE,
    };
    imageBarrier[3] = {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask     = VK_PIPELINE_STAGE_2_NONE,
        .srcAccessMask    = VK_ACCESS_2_NONE_KHR,
        .dstStageMask     = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        .dstAccessMask    = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
        .oldLayout        = VK_IMAGE_LAYOUT_GENERAL,
        .newLayout        = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .image            = velocityXOldMap.handle,
        .subresourceRange = ezVulkanBackend::SUBRESOURCE_WHOLE,
    };
    imageBarrier[4] = {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask     = VK_PIPELINE_STAGE_2_NONE,
        .srcAccessMask    = VK_ACCESS_2_NONE_KHR,
        .dstStageMask     = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        .dstAccessMask    = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
        .oldLayout        = VK_IMAGE_LAYOUT_GENERAL,
        .newLayout        = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .image            = velocityYOldMap.handle,
        .subresourceRange = ezVulkanBackend::SUBRESOURCE_WHOLE,
    };
    imageBarrier[5] = {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask     = VK_PIPELINE_STAGE_2_NONE,
        .srcAccessMask    = VK_ACCESS_2_NONE_KHR,
        .dstStageMask     = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        .dstAccessMask    = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
        .oldLayout        = VK_IMAGE_LAYOUT_GENERAL,
        .newLayout        = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .image            = smokeOldMap.handle,
        .subresourceRange = ezVulkanBackend::SUBRESOURCE_WHOLE,
    };
    depInfo = {
        .sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 6,
        .pImageMemoryBarriers    = imageBarrier,
    };
    vkCmdPipelineBarrier2(cmd, &depInfo);

    // ------------------------ blit ------------------------ //
    VkImageBlit2 blitRegion {
        .sType          = VK_STRUCTURE_TYPE_IMAGE_BLIT_2,
        .pNext          = nullptr,
        .srcSubresource = ezVulkanBackend::SUBRESOURCE_LAYERS_WHOLE,
        .dstSubresource = ezVulkanBackend::SUBRESOURCE_LAYERS_WHOLE
    };
    blitRegion.srcOffsets[1] = {.x = static_cast<int32_t>(SIM_BOUNDS.width + 1), .y = static_cast<int32_t>(SIM_BOUNDS.height), .z = 1};
    blitRegion.dstOffsets[1] = {.x = static_cast<int32_t>(SIM_BOUNDS.width + 1), .y = static_cast<int32_t>(SIM_BOUNDS.height), .z = 1};
    VkBlitImageInfo2 blitInfo {
        .sType          = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
        .srcImage       = velocityXMap.handle,
        .srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .dstImage       = velocityXOldMap.handle,
        .dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .regionCount    = 1,
        .pRegions       = &blitRegion,
        .filter         = VK_FILTER_LINEAR,
    };
    vkCmdBlitImage2(cmd, &blitInfo);
    blitRegion.srcOffsets[1] = {.x = static_cast<int32_t>(SIM_BOUNDS.width), .y = static_cast<int32_t>(SIM_BOUNDS.height + 1), .z = 1};
    blitRegion.dstOffsets[1] = {.x = static_cast<int32_t>(SIM_BOUNDS.width), .y = static_cast<int32_t>(SIM_BOUNDS.height + 1), .z = 1};
    blitInfo.srcImage        = velocityYMap.handle;
    blitInfo.dstImage        = velocityYOldMap.handle;
    vkCmdBlitImage2(cmd, &blitInfo);
    blitRegion.srcOffsets[1] = {.x = static_cast<int32_t>(SIM_BOUNDS.width), .y = static_cast<int32_t>(SIM_BOUNDS.height), .z = 1};
    blitRegion.dstOffsets[1] = {.x = static_cast<int32_t>(SIM_BOUNDS.width), .y = static_cast<int32_t>(SIM_BOUNDS.height), .z = 1};
    blitInfo.srcImage        = smokeMap.handle;
    blitInfo.dstImage        = smokeOldMap.handle;
    vkCmdBlitImage2(cmd, &blitInfo);

    imageBarrier[0] = {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask     = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .srcAccessMask    = VK_ACCESS_2_TRANSFER_READ_BIT,
        .dstStageMask     = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        .dstAccessMask    = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
        .oldLayout        = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .newLayout        = VK_IMAGE_LAYOUT_GENERAL,
        .image            = velocityXMap.handle,
        .subresourceRange = ezVulkanBackend::SUBRESOURCE_WHOLE,
    };
    imageBarrier[1] = {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask     = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .srcAccessMask    = VK_ACCESS_2_TRANSFER_READ_BIT,
        .dstStageMask     = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        .dstAccessMask    = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
        .oldLayout        = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .newLayout        = VK_IMAGE_LAYOUT_GENERAL,
        .image            = velocityYMap.handle,
        .subresourceRange = ezVulkanBackend::SUBRESOURCE_WHOLE,
    };
    imageBarrier[2] = {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask     = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .srcAccessMask    = VK_ACCESS_2_TRANSFER_READ_BIT,
        .dstStageMask     = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        .dstAccessMask    = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
        .oldLayout        = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .newLayout        = VK_IMAGE_LAYOUT_GENERAL,
        .image            = smokeMap.handle,
        .subresourceRange = ezVulkanBackend::SUBRESOURCE_WHOLE,
    };
    imageBarrier[3] = {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask     = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .srcAccessMask    = VK_ACCESS_2_TRANSFER_WRITE_BIT,
        .dstStageMask     = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        .dstAccessMask    = VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
        .oldLayout        = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .newLayout        = VK_IMAGE_LAYOUT_GENERAL,
        .image            = velocityXOldMap.handle,
        .subresourceRange = ezVulkanBackend::SUBRESOURCE_WHOLE,
    };
    imageBarrier[4] = {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask     = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .srcAccessMask    = VK_ACCESS_2_TRANSFER_WRITE_BIT,
        .dstStageMask     = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        .dstAccessMask    = VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
        .oldLayout        = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .newLayout        = VK_IMAGE_LAYOUT_GENERAL,
        .image            = velocityYOldMap.handle,
        .subresourceRange = ezVulkanBackend::SUBRESOURCE_WHOLE,
    };
    imageBarrier[5] = {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask     = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .srcAccessMask    = VK_ACCESS_2_TRANSFER_WRITE_BIT,
        .dstStageMask     = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        .dstAccessMask    = VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
        .oldLayout        = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .newLayout        = VK_IMAGE_LAYOUT_GENERAL,
        .image            = smokeOldMap.handle,
        .subresourceRange = ezVulkanBackend::SUBRESOURCE_WHOLE,
    };
    depInfo = {
        .sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 6,
        .pImageMemoryBarriers    = imageBarrier,
    };
    vkCmdPipelineBarrier2(cmd, &depInfo);
}
void FluidSimWindow::clearImages(VkCommandBuffer cmd) {
    VkClearColorValue clearColor {.float32 = {0.0, 0.0, 0.0, 0.0}};

    VkMemoryBarrier2 memoryBarrier {
        .sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
        .pNext         = nullptr,
        .srcStageMask  = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        .dstStageMask  = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
    };
    VkDependencyInfo depInfo {
        .sType              = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .memoryBarrierCount = 1,
        .pMemoryBarriers    = &memoryBarrier,
    };

    vkCmdPipelineBarrier2(cmd, &depInfo);
    vkCmdClearColorImage(cmd, velocityXMap.handle, VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &ezVulkanBackend::SUBRESOURCE_WHOLE);
    vkCmdClearColorImage(cmd, velocityYMap.handle, VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &ezVulkanBackend::SUBRESOURCE_WHOLE);
    vkCmdClearColorImage(cmd, velocityXOldMap.handle, VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &ezVulkanBackend::SUBRESOURCE_WHOLE);
    vkCmdClearColorImage(cmd, velocityYOldMap.handle, VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &ezVulkanBackend::SUBRESOURCE_WHOLE);
    vkCmdClearColorImage(cmd, smokeMap.handle, VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &ezVulkanBackend::SUBRESOURCE_WHOLE);
    vkCmdClearColorImage(cmd, smokeOldMap.handle, VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &ezVulkanBackend::SUBRESOURCE_WHOLE);
    vkCmdClearColorImage(cmd, pressureMap.handle, VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &ezVulkanBackend::SUBRESOURCE_WHOLE);
    vkCmdClearColorImage(cmd, solidityMap.handle, VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &ezVulkanBackend::SUBRESOURCE_WHOLE);
    vkCmdClearColorImage(cmd, cellDataMap.handle, VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &ezVulkanBackend::SUBRESOURCE_WHOLE);
    vkCmdPipelineBarrier2(cmd, &depInfo);
}

void FluidSimWindow::onOpen() {
    createImages();
    createPipelines();
    createDescriptorPool();
    createDescriptorSets();
    createCommandPool();
    createCommandBuffer();
    createSynchObjects();
    setImagesLayouts();
    assignDebugNames();
}

void FluidSimWindow::onClose() {
    engine.terminate();
}

void FluidSimWindow::createImages() {
    ezImageBuilder imageBuilder(backend);

    imageBuilder.addFamily(computeQueue.family)
        .setFormat(VK_FORMAT_R32_SFLOAT)
        .setExtent(SIM_BOUNDS)
        .setLayout(VK_IMAGE_LAYOUT_UNDEFINED)
        .setUsage(VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT)
        .build(pressureMap)
        .build(divergenceMap)
        .setFormat(VK_FORMAT_R16G16B16A16_SFLOAT)
        .build(cellDataMap)

        .setFormat(VK_FORMAT_R8_UINT)
        .build(solidityMap)

        .setUsage(VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
        .setFormat(VK_FORMAT_R16G16B16A16_SFLOAT)
        .build(smokeMap)
        .build(smokeOldMap)

        .setFormat(VK_FORMAT_R32_SFLOAT)
        .setExtent({SIM_BOUNDS.width + 1, SIM_BOUNDS.height, 1})
        .build(velocityXMap)
        .build(velocityXOldMap)
        .setExtent({SIM_BOUNDS.width, SIM_BOUNDS.height + 1, 1})
        .build(velocityYOldMap)
        .build(velocityYMap);
}
void FluidSimWindow::createPipelines() {
    ezDescriptorSetLayoutBuilder descriptorSetLayoutBuilder(backend);
    ezPipelineLayoutBuilder      pipelineLayoutBuilder(backend);

    descriptorSetLayoutBuilder.addBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
        .addBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
        .addBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
        .addBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
        .addBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
        .addBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
        .addBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
        .addBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
        .addBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
        .addBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
        .addBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
        .build(computeDSLayout);

    PipelineLayout pipelineLayout;
    pipelineLayoutBuilder.addSet(computeDSLayout).addConstant(sizeof(Controls), VK_SHADER_STAGE_COMPUTE_BIT).build(pipelineLayout);

    auto advectShader         = backend.createShader("shaders/fluid_sim_advect.spv").value();
    auto brushShader          = backend.createShader("shaders/fluid_sim_brush.spv").value();
    auto projectShader        = backend.createShader("shaders/fluid_sim_project.spv").value();
    auto velocityUpdateShader = backend.createShader("shaders/fluid_sim_update_velocities.spv").value();
    auto visualizeShader      = backend.createShader("shaders/fluid_sim_visualize.spv").value();
    auto preProcessShader     = backend.createShader("shaders/fluid_sim_pre_process.spv").value();
    auto diffuseShader        = backend.createShader("shaders/fluid_sim_diffuse.spv").value();

    projectPipeline        = backend.createComputePipeline(projectShader, pipelineLayout).value();
    advectPipeline         = backend.createComputePipeline(advectShader, pipelineLayout).value();
    brushPipeline          = backend.createComputePipeline(brushShader, pipelineLayout).value();
    velocityUpdatePipeline = backend.createComputePipeline(velocityUpdateShader, pipelineLayout).value();
    visualizePipeline      = backend.createComputePipeline(visualizeShader, pipelineLayout).value();
    preProcessPipeline     = backend.createComputePipeline(preProcessShader, pipelineLayout).value();
    diffusePipeline        = backend.createComputePipeline(diffuseShader, pipelineLayout).value();
}
void FluidSimWindow::createDescriptorSets() {
    backend.allocateDescriptorSets(descriptorPool, 1, computeDSLayout, &computeDS);

    VkDescriptorImageInfo imageInfo[10] = {
        VkDescriptorImageInfo {
            .sampler     = nullptr,
            .imageView   = pressureMap.view,
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
        },
        VkDescriptorImageInfo {
            .sampler     = nullptr,
            .imageView   = velocityXMap.view,
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
        },
        VkDescriptorImageInfo {
            .sampler     = nullptr,
            .imageView   = velocityYMap.view,
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
        },
        VkDescriptorImageInfo {
            .sampler     = nullptr,
            .imageView   = smokeMap.view,
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
        },
        VkDescriptorImageInfo {
            .sampler     = nullptr,
            .imageView   = velocityXOldMap.view,
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
        },
        VkDescriptorImageInfo {
            .sampler     = nullptr,
            .imageView   = velocityYOldMap.view,
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
        },
        VkDescriptorImageInfo {
            .sampler     = nullptr,
            .imageView   = smokeOldMap.view,
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
        },
        VkDescriptorImageInfo {
            .sampler     = nullptr,
            .imageView   = divergenceMap.view,
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
        },
        VkDescriptorImageInfo {
            .sampler     = nullptr,
            .imageView   = solidityMap.view,
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
        },
        VkDescriptorImageInfo {
            .sampler     = nullptr,
            .imageView   = cellDataMap.view,
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
        },
    };

    VkWriteDescriptorSet write[10] = {
        VkWriteDescriptorSet {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = computeDS,
            .dstBinding      = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .pImageInfo      = imageInfo,
        },
        VkWriteDescriptorSet {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = computeDS,
            .dstBinding      = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .pImageInfo      = imageInfo + 1,
        },
        VkWriteDescriptorSet {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = computeDS,
            .dstBinding      = 2,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .pImageInfo      = imageInfo + 2,
        },
        VkWriteDescriptorSet {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = computeDS,
            .dstBinding      = 4,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .pImageInfo      = imageInfo + 3,
        },
        VkWriteDescriptorSet {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = computeDS,
            .dstBinding      = 5,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .pImageInfo      = imageInfo + 4,
        },
        VkWriteDescriptorSet {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = computeDS,
            .dstBinding      = 6,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .pImageInfo      = imageInfo + 5,
        },
        VkWriteDescriptorSet {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = computeDS,
            .dstBinding      = 7,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .pImageInfo      = imageInfo + 6,
        },
        VkWriteDescriptorSet {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = computeDS,
            .dstBinding      = 8,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .pImageInfo      = imageInfo + 7,
        },
        VkWriteDescriptorSet {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = computeDS,
            .dstBinding      = 9,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .pImageInfo      = imageInfo + 8,
        },
        VkWriteDescriptorSet {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = computeDS,
            .dstBinding      = 10,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .pImageInfo      = imageInfo + 9,
        },
    };

    backend.updateDescriptorSets(10, write, 0, nullptr);
}
void FluidSimWindow::createDescriptorPool() {
    vector<VkDescriptorPoolSize> poolSize = {
        {
            .type            = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 11,
        },
    };
    descriptorPool = backend.createDescriptorSetPool(poolSize).value();
}
void FluidSimWindow::setImagesLayouts() {
    VkImageMemoryBarrier2 barriers[10] {
        VkImageMemoryBarrier2 {
            .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask     = VK_PIPELINE_STAGE_2_NONE,
            .srcAccessMask    = VK_ACCESS_2_NONE_KHR,
            .dstStageMask     = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
            .dstAccessMask    = VK_ACCESS_2_NONE_KHR,
            .oldLayout        = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout        = VK_IMAGE_LAYOUT_GENERAL,
            .image            = pressureMap.handle,
            .subresourceRange = ezVulkanBackend::SUBRESOURCE_WHOLE,
        },
        VkImageMemoryBarrier2 {
            .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask     = VK_PIPELINE_STAGE_2_NONE,
            .srcAccessMask    = VK_ACCESS_2_NONE_KHR,
            .dstStageMask     = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
            .dstAccessMask    = VK_ACCESS_2_NONE_KHR,
            .oldLayout        = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout        = VK_IMAGE_LAYOUT_GENERAL,
            .image            = velocityXMap.handle,
            .subresourceRange = ezVulkanBackend::SUBRESOURCE_WHOLE,
        },
        VkImageMemoryBarrier2 {
            .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask     = VK_PIPELINE_STAGE_2_NONE,
            .srcAccessMask    = VK_ACCESS_2_NONE_KHR,
            .dstStageMask     = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
            .dstAccessMask    = VK_ACCESS_2_NONE_KHR,
            .oldLayout        = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout        = VK_IMAGE_LAYOUT_GENERAL,
            .image            = velocityXOldMap.handle,
            .subresourceRange = ezVulkanBackend::SUBRESOURCE_WHOLE,
        },
        VkImageMemoryBarrier2 {
            .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask     = VK_PIPELINE_STAGE_2_NONE,
            .srcAccessMask    = VK_ACCESS_2_NONE_KHR,
            .dstStageMask     = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
            .dstAccessMask    = VK_ACCESS_2_NONE_KHR,
            .oldLayout        = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout        = VK_IMAGE_LAYOUT_GENERAL,
            .image            = velocityYMap.handle,
            .subresourceRange = ezVulkanBackend::SUBRESOURCE_WHOLE,
        },
        VkImageMemoryBarrier2 {
            .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask     = VK_PIPELINE_STAGE_2_NONE,
            .srcAccessMask    = VK_ACCESS_2_NONE_KHR,
            .dstStageMask     = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
            .dstAccessMask    = VK_ACCESS_2_NONE_KHR,
            .oldLayout        = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout        = VK_IMAGE_LAYOUT_GENERAL,
            .image            = velocityYOldMap.handle,
            .subresourceRange = ezVulkanBackend::SUBRESOURCE_WHOLE,
        },
        VkImageMemoryBarrier2 {
            .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask     = VK_PIPELINE_STAGE_2_NONE,
            .srcAccessMask    = VK_ACCESS_2_NONE_KHR,
            .dstStageMask     = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
            .dstAccessMask    = VK_ACCESS_2_NONE_KHR,
            .oldLayout        = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout        = VK_IMAGE_LAYOUT_GENERAL,
            .image            = smokeMap.handle,
            .subresourceRange = ezVulkanBackend::SUBRESOURCE_WHOLE,
        },
        VkImageMemoryBarrier2 {
            .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask     = VK_PIPELINE_STAGE_2_NONE,
            .srcAccessMask    = VK_ACCESS_2_NONE_KHR,
            .dstStageMask     = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
            .dstAccessMask    = VK_ACCESS_2_NONE_KHR,
            .oldLayout        = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout        = VK_IMAGE_LAYOUT_GENERAL,
            .image            = smokeOldMap.handle,
            .subresourceRange = ezVulkanBackend::SUBRESOURCE_WHOLE,
        },
        VkImageMemoryBarrier2 {
            .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask     = VK_PIPELINE_STAGE_2_NONE,
            .srcAccessMask    = VK_ACCESS_2_NONE_KHR,
            .dstStageMask     = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
            .dstAccessMask    = VK_ACCESS_2_NONE_KHR,
            .oldLayout        = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout        = VK_IMAGE_LAYOUT_GENERAL,
            .image            = divergenceMap.handle,
            .subresourceRange = ezVulkanBackend::SUBRESOURCE_WHOLE,
        },
        VkImageMemoryBarrier2 {
            .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask     = VK_PIPELINE_STAGE_2_NONE,
            .srcAccessMask    = VK_ACCESS_2_NONE_KHR,
            .dstStageMask     = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
            .dstAccessMask    = VK_ACCESS_2_NONE_KHR,
            .oldLayout        = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout        = VK_IMAGE_LAYOUT_GENERAL,
            .image            = solidityMap.handle,
            .subresourceRange = ezVulkanBackend::SUBRESOURCE_WHOLE,
        },
        VkImageMemoryBarrier2 {
            .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask     = VK_PIPELINE_STAGE_2_NONE,
            .srcAccessMask    = VK_ACCESS_2_NONE_KHR,
            .dstStageMask     = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
            .dstAccessMask    = VK_ACCESS_2_NONE_KHR,
            .oldLayout        = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout        = VK_IMAGE_LAYOUT_GENERAL,
            .image            = cellDataMap.handle,
            .subresourceRange = ezVulkanBackend::SUBRESOURCE_WHOLE,
        }
    };
    VkDependencyInfo depInfo {
        .sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 10,
        .pImageMemoryBarriers    = barriers,
    };
    VkCommandBufferBeginInfo computeCommandBufferBeginInfo {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext            = nullptr,
        .flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr,
    };
    vkBeginCommandBuffer(computeCommandBuffer[0].handle, &computeCommandBufferBeginInfo);
    vkCmdPipelineBarrier2(computeCommandBuffer[0].handle, &depInfo);

    vkEndCommandBuffer(computeCommandBuffer[0].handle);

    // -------------- submit to compute queueu -------------- //
    static VkCommandBufferSubmitInfo computeBufferSubmitInfo {
        .sType         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .pNext         = nullptr,
        .commandBuffer = computeCommandBuffer[0].handle,
        .deviceMask    = 0,
    };
    static VkSubmitInfo2 computeSubmitInfo {
        .sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .pNext                    = nullptr,
        .flags                    = 0,
        .waitSemaphoreInfoCount   = 0,
        .pWaitSemaphoreInfos      = nullptr,
        .commandBufferInfoCount   = 1,
        .pCommandBufferInfos      = &computeBufferSubmitInfo,
        .signalSemaphoreInfoCount = 0,
        .pSignalSemaphoreInfos    = nullptr,
    };
    vkQueueSubmit2(computeQueue.handle, 1, &computeSubmitInfo, nullptr);

    vkQueueWaitIdle(computeQueue.handle);
}
void FluidSimWindow::createCommandPool() {
    computeCommandBufferPool  = backend.createCommandPool(computeQueue).value();
    graphicsCommandBufferPool = backend.createCommandPool(graphicsQueue).value();
}
void FluidSimWindow::createCommandBuffer() {
    for (auto i = 0; i < FRAMES_IN_FLY; i++) {
        computeCommandBuffer[i] = backend.allocateCommandBuffer(computeCommandBufferPool).value();
    }
    graphicsCommandBuffer = backend.allocateCommandBuffer(graphicsCommandBufferPool).value();
}
void FluidSimWindow::createSynchObjects() {
    for (auto i = 0; i < FRAMES_IN_FLY; i++) {
        computeFence[i]     = backend.createFence().value();
        computeSemaphore[i] = backend.createSemaphore().value();
    }
}
void FluidSimWindow::assignDebugNames() {
    setDebugName(device.handle, computeQueue.handle, VK_OBJECT_TYPE_QUEUE, "queue_compute");
    setDebugName(device.handle, graphicsQueue.handle, VK_OBJECT_TYPE_QUEUE, "queue_graphics");
    setDebugName(device.handle, presentQueue.handle, VK_OBJECT_TYPE_QUEUE, "queue_present");

    setDebugName(device.handle, pressureMap.handle, VK_OBJECT_TYPE_IMAGE, "pressureMap");
    setDebugName(device.handle, velocityXMap.handle, VK_OBJECT_TYPE_IMAGE, "velocityXMap");
    setDebugName(device.handle, velocityXOldMap.handle, VK_OBJECT_TYPE_IMAGE, "velocityXOldMap");
    setDebugName(device.handle, velocityYMap.handle, VK_OBJECT_TYPE_IMAGE, "velocityYMap");
    setDebugName(device.handle, velocityYOldMap.handle, VK_OBJECT_TYPE_IMAGE, "velocityYOldMap");
    setDebugName(device.handle, smokeMap.handle, VK_OBJECT_TYPE_IMAGE, "smokeMap");
    setDebugName(device.handle, smokeOldMap.handle, VK_OBJECT_TYPE_IMAGE, "smokeOldMap");

    setDebugName(device.handle, advectPipeline.handle, VK_OBJECT_TYPE_PIPELINE, "advectPipeline");
    setDebugName(device.handle, projectPipeline.handle, VK_OBJECT_TYPE_PIPELINE, "projectPipeline");
    setDebugName(device.handle, velocityUpdatePipeline.handle, VK_OBJECT_TYPE_PIPELINE, "velocityUpdatePipeline");
    setDebugName(device.handle, brushPipeline.handle, VK_OBJECT_TYPE_PIPELINE, "brushPipeline");
    setDebugName(device.handle, visualizePipeline.handle, VK_OBJECT_TYPE_PIPELINE, "visualizePipeline");

    // setDebugName(device.handle, renderSemaphore, VK_OBJECT_TYPE_SEMAPHORE, "sem_render");
    // setDebugName(device.handle, computeSemaphore, VK_OBJECT_TYPE_SEMAPHORE, "sem_compute");

    // setDebugName(device.handle, computeFence, VK_OBJECT_TYPE_FENCE, "fen_compute");

    for (auto i = 0; i < FRAMES_IN_FLY; i++) {
        setDebugName(device.handle, computeCommandBuffer[i].handle, VK_OBJECT_TYPE_COMMAND_BUFFER, "computeBuffer_{}", i);
    }
    setDebugName(device.handle, graphicsCommandBuffer.handle, VK_OBJECT_TYPE_COMMAND_BUFFER, "graphicsBuffer");
}