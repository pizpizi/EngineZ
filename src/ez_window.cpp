#include "enginez/graphics/ez_window.hpp"
#include "GLFW/glfw3.h"
#include "enginez/ez_engine.hpp"
#include "enginez/graphics/ez_error.hpp"
#include "enginez/graphics/ez_types.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "vulkan/vk_enum_string_helper.h"
#include <cstdint>
#include <format>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <vulkan/vk_layer.h>
#include <vulkan/vulkan_core.h>

using namespace enginez::graphics;
using namespace enginez::err;
using namespace std;

void ezWindow::setupLogger() {
    logger.addConsoleSink(true, logz::DEBUG);
    logger.addFileSink("log.txt", logz::DEBUG);
}

ezWindow::ezWindow(ezWindowCreateInfo& createInfo)
    : engine(createInfo.engine),
      backend(engine.graphicsBackend),
      instance(engine.graphicsBackend.instance),
      device(engine.graphicsBackend.device),
      logger(logz::createDefaultLogger(logz::SINCE_PROGRAM_START, createInfo.title)),
      width(createInfo.width),
      height(createInfo.height),
      title(createInfo.title),
      graphicsQueue(createInfo.graphicsQueue),
      presentQueue(createInfo.presentQueue) {
    setupLogger();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);

    glfwWindow = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!glfwWindow) {
        throw ezError(err::Code::GLFW_INIT_FAIL, "Failed to initialize the glfw window for window: {}", title);
    }
}

void ezWindow::init(ezVulkanBackend* backend) {
    createSurface();
    createSwapchain();
    setupFrameData();
    setupDrawImage();
    setupImgui();
}

void ezWindow::createSurface() {

    if (glfwCreateWindowSurface(instance, glfwWindow, nullptr, &surface) != VK_SUCCESS) {
        throw ezError(err::Code::SURFACE_CREATION_FAIL, "Failed to create the surface for window: {}", title);
    }
    logger.debugf("Created the surface for {} window", title);
}

void ezWindow::setupDrawImage() {
    VkExtent3D extent;
    extent.width  = swapchainExtent.width;
    extent.height = swapchainExtent.height;
    extent.depth  = 1;
    image.extent  = extent;

    VkImageUsageFlags usages =
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT;

    VkImageCreateInfo imageCI {
        .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext         = nullptr,
        .flags         = 0,
        .imageType     = VK_IMAGE_TYPE_2D,
        .format        = VK_FORMAT_R16G16B16A16_SFLOAT,
        .extent        = extent,
        .mipLevels     = 1,
        .arrayLayers   = 1,
        .samples       = VK_SAMPLE_COUNT_1_BIT,
        .tiling        = VK_IMAGE_TILING_OPTIMAL,
        .usage         = usages,
        .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    VmaAllocationCreateInfo allocationCI {.usage = VMA_MEMORY_USAGE_GPU_ONLY, .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};

    auto result = vmaCreateImage(backend.allocator, &imageCI, &allocationCI, &image.handle, &image.allocation, nullptr);
    if (result != VK_SUCCESS) {
        throw ezError(Code::IMAGE_CREATION_FAIL, result, "Failed to create the draw image for window: {}", title);
    }

    VkImageViewCreateInfo viewCI {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext            = nullptr,
        .flags            = 0,
        .image            = image.handle,
        .viewType         = VK_IMAGE_VIEW_TYPE_2D,
        .format           = VK_FORMAT_R16G16B16A16_SFLOAT,
        .components       = {},
        .subresourceRange = ezVulkanBackend::SUBRESOURCE_WHOLE,
    };

    result = vkCreateImageView(device.handle, &viewCI, nullptr, &image.view);
    if (result != VK_SUCCESS) {
        throw ezError(Code::IMAGE_VIEW_CREATION_FAIL, result, "Failed to create the draw image view for window: {}", title);
    }

    logger.debugf("Setup the draw image for window: {}", title);
}

void ezWindow::createSwapchain() {
    // ------------------------ data ------------------------ //
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.phyisicalDevice.handle, surface, &surfaceCapabilities);

    vector<VkPresentModeKHR> presentModes;
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device.phyisicalDevice.handle, surface, &presentModeCount, nullptr);
    presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device.phyisicalDevice.handle, surface, &presentModeCount, presentModes.data());

    vector<VkSurfaceFormatKHR> surfaceFormats;
    uint32_t surfaceFormatsCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device.phyisicalDevice.handle, surface, &surfaceFormatsCount, nullptr);
    surfaceFormats.resize(surfaceFormatsCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device.phyisicalDevice.handle, surface, &surfaceFormatsCount, surfaceFormats.data());

    auto chosenFormat      = getSuitableFormat(surfaceFormats);
    auto chosenPresentMode = choosePresentMode(presentModes);
    auto chosenExtent      = chooseSwapExtent(surfaceCapabilities);
    // ------------------------------------------------------ //

    // ------------------------ logs ------------------------ //
    stringstream stream;
    stream << "Surface Capablities:\n";
    stream << "\tminImageCount: " << surfaceCapabilities.minImageCount << "\n";
    stream << "\tcurrentExtent: " << "(" << surfaceCapabilities.currentExtent.width << ", " << surfaceCapabilities.currentExtent.width << ")\n";
    stream << "\tminExtent: " << "(" << surfaceCapabilities.minImageExtent.width << ", " << surfaceCapabilities.minImageExtent.width << ")\n";
    stream << "\tmaxExtent: " << "(" << surfaceCapabilities.maxImageExtent.width << ", " << surfaceCapabilities.maxImageExtent.width << ")\n";
    stream << "\tchosenExtennt: " << "(" << chosenExtent.width << ", " << chosenExtent.width << ")\n";
    stream << "\tsupportedUsageFlags: \n";
    for (uint64_t i = 1; i <= VK_IMAGE_USAGE_FLAG_BITS_MAX_ENUM; i = i << 1) {
        if (surfaceCapabilities.supportedUsageFlags & i) stream << "\t\t" << string_VkImageUsageFlagBits((VkImageUsageFlagBits)i) << "\n";
    }
    stream << "Supported Present Modes:\n";
    for (int i = 0; i < presentModeCount; i++) {
        auto mode = presentModes[i];
        stream << "\t" << i << ". ";
        stream << string_VkPresentModeKHR(mode) << " (" << mode << ")";
        if (mode == chosenPresentMode) {
            stream << " *";
        }
        stream << "\n";
    }
    stream << "Supported Surface Formats:\n";
    for (int i = 0; i < surfaceFormatsCount; i++) {
        auto format = surfaceFormats[i];

        stream << "\t" << i << ". ";
        stream << string_VkFormat(format.format) << " (" << format.format << ") "
               << " | " << string_VkColorSpaceKHR(format.colorSpace) << " (" << format.colorSpace << ")";
        if (format.format == chosenFormat.format && format.colorSpace == chosenFormat.colorSpace) {
            stream << " *";
        }
        stream << "\n";
    }

    logger.debug(stream.str());
    // ------------------------------------------------------ //

    VkSwapchainCreateInfoKHR createInfo {
        .sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface               = surface,
        .minImageCount         = surfaceCapabilities.minImageCount,
        .imageFormat           = chosenFormat.format,
        .imageColorSpace       = chosenFormat.colorSpace,
        .imageExtent           = chosenExtent,
        .imageArrayLayers      = 1,
        .imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices   = nullptr,
        .preTransform          = surfaceCapabilities.currentTransform,
        .compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode           = chosenPresentMode,
        .clipped               = VK_TRUE,
        .oldSwapchain          = nullptr
    };

    VkResult result = vkCreateSwapchainKHR(device.handle, &createInfo, nullptr, &swapchain);

    if (result != VK_SUCCESS) {
        throw ezError(Code::SWAPCHAIN_CREATION_FAIL, result, "Failed to create the swapchain for window: {}", title);
    }

    swapchainExtent = chosenExtent;
    swapchainFormat = chosenFormat.format;
    uint32_t swapchainImageCount;
    vkGetSwapchainImagesKHR(device.handle, swapchain, &swapchainImageCount, nullptr);
    swapchainImages.resize(swapchainImageCount);
    vkGetSwapchainImagesKHR(device.handle, swapchain, &swapchainImageCount, swapchainImages.data());

    logger.debugf("created the swapchain for {} window with extent <{}, {}>", title, swapchainExtent.width, swapchainExtent.height);
}

void ezWindow::setupFrameData() {
    for (auto i = 0; i < FRAMES_IN_FLY; i++) {
        frameData[i].commandPool        = backend.createCommandPool(graphicsQueue).value();
        frameData[i].commandBuffer      = backend.allocateCommandBuffer(frameData[i].commandPool).value();
        frameData[i].renderFence        = backend.createFence().value();
        frameData[i].renderSemaphore    = backend.createSemaphore().value();
        frameData[i].swapchainSemaphore = backend.createSemaphore().value();
    }
    logger.debugf("Setup the required frame data for {} frames in the {} window", FRAMES_IN_FLY, title);
}

VkSurfaceFormatKHR ezWindow::getSuitableFormat(vector<VkSurfaceFormatKHR>& formats) {
    // TODO: better format selection
    for (auto& fmt : formats) {
        if (fmt.format == ezVulkanBackend::DESIRED_SWAPCHAIN_COLOR_FORMAT) {
            return fmt;
        }
    }

    throw ezError(
        Code::NO_SUITABLE_FORMAT,
        "Desired color format ({}) is not supported by window: {}",
        string_VkFormat(ezVulkanBackend::DESIRED_SWAPCHAIN_COLOR_FORMAT),
        title
    );
}

VkPresentModeKHR ezWindow::choosePresentMode(std::vector<VkPresentModeKHR>& modes) {
    for (auto& m : modes) {
        if (m == VK_PRESENT_MODE_FIFO_RELAXED_KHR) {
            return m;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D ezWindow::chooseSwapExtent(VkSurfaceCapabilitiesKHR& surfaceCapabilities) {
    if (surfaceCapabilities.currentExtent.width != UINT32_MAX) {
        return surfaceCapabilities.currentExtent;
    } else {
        int width, height;
        glfwGetWindowSize(glfwWindow, &width, &height);
        VkExtent2D extent {};
        extent.width  = static_cast<uint32_t>(width);
        extent.height = static_cast<uint32_t>(height);

        extent.width  = max(surfaceCapabilities.minImageExtent.width, min(surfaceCapabilities.maxImageExtent.width, extent.width));
        extent.height = max(surfaceCapabilities.minImageExtent.height, min(surfaceCapabilities.maxImageExtent.height, extent.height));

        return extent;
    }
}

void ezWindow::cleanUp() {
    glfwDestroyWindow(glfwWindow);
    logger.debugf("cleaned up window: {}", title);
}

void ezWindow::setupImgui() {
    VkDescriptorPoolSize pool_sizes[] = {
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 8},
        {VK_DESCRIPTOR_TYPE_SAMPLER, 2},
    };
    DescriptorPool imguiPool = backend.createDescriptorSetPool(pool_sizes).value();

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForVulkan(glfwWindow, true);

    VkPipelineRenderingCreateInfo pipelineRenderingCI {
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount    = 1,
        .pColorAttachmentFormats = &ezVulkanBackend::DESIRED_COLOR_FORMAT,
    };
    ImGui_ImplVulkan_PipelineInfo pipelineInfo {
        .MSAASamples                 = VK_SAMPLE_COUNT_1_BIT,
        .PipelineRenderingCreateInfo = pipelineRenderingCI,
    };
    ImGui_ImplVulkan_InitInfo init_info {
        .Instance            = instance,
        .PhysicalDevice      = device.phyisicalDevice.handle,
        .Device              = device.handle,
        .QueueFamily         = graphicsQueue.family,
        .Queue               = graphicsQueue.handle,
        .DescriptorPool      = imguiPool.handle,
        .MinImageCount       = surfaceCapabilities.minImageCount,
        .ImageCount          = surfaceCapabilities.minImageCount,
        .PipelineCache       = nullptr,
        .PipelineInfoMain    = pipelineInfo,
        .UseDynamicRendering = true,
    };
    ImGui_ImplVulkan_Init(&init_info);

    colorAttchInfo = {
        .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .pNext       = nullptr,
        .imageView   = image.view,
        .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
        .loadOp      = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
    };
    imguiRenderInfo = {
        .sType                = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea           = {.extent = swapchainExtent},
        .layerCount           = 1,
        .viewMask             = 0,
        .colorAttachmentCount = 1,
        .pColorAttachments    = &colorAttchInfo,
    };
}

void ezWindow::internalUpdate() {
    if (glfwWindowShouldClose(glfwWindow)) {
        closed = true;
        logger.debug(std::format("window {} marked as closed", title));
    }

    static VkImageMemoryBarrier2 barrier[2];
    static VkDependencyInfo depInfo;

    currentFrameData    = &frameData[currentFrame % FRAMES_IN_FLY];
    VkCommandBuffer cmd = currentFrameData->commandBuffer.handle;

    // ------------------- wait for fence ------------------- //
    vkWaitForFences(device.handle, 1, &currentFrameData->renderFence, VK_TRUE, 1000000000);
    vkResetFences(device.handle, 1, &currentFrameData->renderFence);

    // ---------------- begin command buffer ---------------- //
    VkCommandBufferBeginInfo cmdBeginInfo {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext            = nullptr,
        .flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr,
    };

    vkResetCommandBuffer(cmd, 0);
    vkBeginCommandBuffer(cmd, &cmdBeginInfo);

    // --------------- image layout transition -------------- //
    barrier[0] = {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask     = VK_PIPELINE_STAGE_2_NONE,
        .srcAccessMask    = VK_ACCESS_2_NONE_KHR,
        .dstStageMask     = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstAccessMask    = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        .oldLayout        = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout        = VK_IMAGE_LAYOUT_GENERAL,
        .image            = image.handle,
        .subresourceRange = ezVulkanBackend::SUBRESOURCE_WHOLE,
    };
    depInfo = {
        .sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers    = barrier,
    };
    vkCmdPipelineBarrier2(currentFrameData->commandBuffer.handle, &depInfo);

    // --------------------- main update -------------------- //
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

    onUpdate();

    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();
    // ------------------------ imgui ----------------------- //
    barrier[0] = {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask     = VK_PIPELINE_STAGE_2_NONE,
        .srcAccessMask    = VK_ACCESS_2_NONE_KHR,
        .dstStageMask     = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstAccessMask    = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        .oldLayout        = VK_IMAGE_LAYOUT_GENERAL,
        .newLayout        = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .image            = image.handle,
        .subresourceRange = ezVulkanBackend::SUBRESOURCE_WHOLE,
    };
    depInfo = {
        .sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers    = barrier,
    };
    vkCmdPipelineBarrier2(cmd, &depInfo);

    vkCmdBeginRendering(cmd, &imguiRenderInfo);
    ImGui_ImplVulkan_RenderDrawData(draw_data, cmd);
    vkCmdEndRendering(cmd);

    // ----------------- get swapchain image ---------------- //
    uint32_t swapchainImageIndex;
    vkAcquireNextImageKHR(device.handle, swapchain, 1000000000, currentFrameData->swapchainSemaphore, nullptr, &swapchainImageIndex);
    VkImage swapchainImage = swapchainImages[swapchainImageIndex];

    // ------------- transitions before transfer ------------ //
    barrier[0] = {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask     = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask    = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        .dstStageMask     = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .dstAccessMask    = VK_ACCESS_2_TRANSFER_READ_BIT,
        .oldLayout        = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .newLayout        = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .image            = image.handle,
        .subresourceRange = ezVulkanBackend::SUBRESOURCE_WHOLE,
    };
    barrier[1] = {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask     = VK_PIPELINE_STAGE_2_NONE,
        .srcAccessMask    = VK_ACCESS_2_NONE,
        .dstStageMask     = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .dstAccessMask    = VK_ACCESS_2_TRANSFER_WRITE_BIT,
        .oldLayout        = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout        = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .image            = swapchainImage,
        .subresourceRange = ezVulkanBackend::SUBRESOURCE_WHOLE,
    };
    depInfo = {
        .sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 2,
        .pImageMemoryBarriers    = barrier,
    };
    vkCmdPipelineBarrier2(cmd, &depInfo);

    // ------------------ copy to swapchain ----------------- //
    VkImageBlit2 blitRegion {
        .sType          = VK_STRUCTURE_TYPE_IMAGE_BLIT_2,
        .pNext          = nullptr,
        .srcSubresource = ezVulkanBackend::SUBRESOURCE_LAYERS_WHOLE,
        .dstSubresource = ezVulkanBackend::SUBRESOURCE_LAYERS_WHOLE
    };
    blitRegion.srcOffsets[1] = {.x = static_cast<int32_t>(image.extent.width), .y = static_cast<int32_t>(image.extent.height), .z = 1};
    blitRegion.dstOffsets[1] = {.x = static_cast<int32_t>(swapchainExtent.width), .y = static_cast<int32_t>(swapchainExtent.height), .z = 1};
    VkBlitImageInfo2 blitInfo {
        .sType          = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
        .srcImage       = image.handle,
        .srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .dstImage       = swapchainImage,
        .dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .regionCount    = 1,
        .pRegions       = &blitRegion,
        .filter         = VK_FILTER_LINEAR,
    };
    vkCmdBlitImage2(cmd, &blitInfo);

    // ------------------- present layout ------------------- //
    barrier[0] = {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask     = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .srcAccessMask    = VK_ACCESS_2_TRANSFER_WRITE_BIT,
        .dstStageMask     = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
        .dstAccessMask    = VK_ACCESS_2_NONE,
        .oldLayout        = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .newLayout        = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .image            = swapchainImage,
        .subresourceRange = ezVulkanBackend::SUBRESOURCE_WHOLE,
    };
    depInfo = {
        .sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers    = barrier,
    };
    vkCmdPipelineBarrier2(cmd, &depInfo);

    // ----------------- end command buffer ----------------- //
    vkEndCommandBuffer(cmd);

    // ------------------- submit to queue ------------------ //
    VkCommandBufferSubmitInfo cmdInfo {
        .sType         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .commandBuffer = cmd,
        .deviceMask    = 0,
    };
    VkSemaphoreSubmitInfo waitInfo {
        .sType       = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .semaphore   = currentFrameData->swapchainSemaphore,
        .value       = 1,
        .stageMask   = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .deviceIndex = 0,
    };
    VkSemaphoreSubmitInfo signalInfo {
        .sType       = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .semaphore   = currentFrameData->renderSemaphore,
        .value       = 1,
        .stageMask   = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .deviceIndex = 0,
    };
    VkSubmitInfo2 submitInfo {
        .sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .waitSemaphoreInfoCount   = 1,
        .pWaitSemaphoreInfos      = &waitInfo,
        .commandBufferInfoCount   = 1,
        .pCommandBufferInfos      = &cmdInfo,
        .signalSemaphoreInfoCount = 1,
        .pSignalSemaphoreInfos    = &signalInfo,
    };
    vkQueueSubmit2(graphicsQueue.handle, 1, &submitInfo, currentFrameData->renderFence);

    // ----------------------- present ---------------------- //
    VkPresentInfoKHR presentInfo = {
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = &currentFrameData->renderSemaphore,
        .swapchainCount     = 1,
        .pSwapchains        = &swapchain,
        .pImageIndices      = &swapchainImageIndex,
    };
    presentInfo.pImageIndices = &swapchainImageIndex;
    vkQueuePresentKHR(presentQueue.handle, &presentInfo);

    currentFrame++;
}

bool ezWindow::isClosed() {
    return closed;
}

void ezWindow::setHeight(int val) {
    // TODO
    throw std::runtime_error("not implemented");
}
void ezWindow::setWidth(int val) {
    // TODO
    throw std::runtime_error("not implemented");
}
void ezWindow::setTitle(std::string val) {
    // TODO
    throw std::runtime_error("not implemented");
}