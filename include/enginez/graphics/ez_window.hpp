#pragma once

#include <cstdint>
#define GLFW_INCLUDE_VULKAN

#include "GLFW/glfw3.h"
#include "enginez/graphics/ez_types.hpp"
#include "logz/logger.hpp"
#include "vulkan/vulkan_core.h"
#include <expected>
#include <string>

namespace enginez {
    class ezEngine;
}

namespace enginez::graphics {
    class ezVulkanBackend;

    struct ezWindowCreateInfo {
        Queue graphicsQueue;
        Queue presentQueue;
        ezEngine& engine;

        std::string title;
        uint32_t width, height;
    };

    class ezWindow {
        friend ezVulkanBackend;

        struct FrameData {
            CommandPool commandPool;
            CommandBuffer commandBuffer;

            Semaphore swapchainSemaphore, renderSemaphore;
            Fence renderFence;
        };

      public:
        static inline const int FRAMES_IN_FLY = 2;

        ezWindow(ezWindowCreateInfo& createInfo);
        void cleanUp();

        virtual void onUpdate() = 0;
        virtual void onOpen()   = 0;
        virtual void onClose()  = 0;

        void setHeight(int val);
        void setWidth(int val);
        void setTitle(std::string val);
        bool isClosed();

      private:
        void internalUpdate();
        void init(ezVulkanBackend* backend);

        bool closed = false;

        void setupLogger();
        void createSwapchain();
        void createSurface();
        void setupFrameData();
        void setupDrawImage();
        void setupImgui();
        VkSurfaceFormatKHR getSuitableFormat(std::vector<VkSurfaceFormatKHR>& formats);
        VkPresentModeKHR choosePresentMode(std::vector<VkPresentModeKHR>& modes);
        VkExtent2D chooseSwapExtent(VkSurfaceCapabilitiesKHR& surfaceCapabilities);

        VkRenderingInfo imguiRenderInfo;
        VkRenderingAttachmentInfo colorAttchInfo;

        FrameData frameData[FRAMES_IN_FLY];
        std::vector<VkImage> swapchainImages;

      protected:
        int currentFrame = 0;
        FrameData* currentFrameData;
        Image image;
        
        ezEngine& engine;
        ezVulkanBackend& backend;
        VkInstance& instance;
        Device& device;

        VkSurfaceCapabilitiesKHR surfaceCapabilities;

        logz::DefaultLogger& logger;
        VkSwapchainKHR swapchain;
        VkSurfaceKHR surface;
        VkExtent2D swapchainExtent;
        VkFormat swapchainFormat;
        uint32_t width, height;
        std::string title;

        GLFWwindow* glfwWindow;

        Queue graphicsQueue;
        Queue presentQueue;
    };

} // namespace enginez::graphics