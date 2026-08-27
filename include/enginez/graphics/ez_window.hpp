#pragma once

#include <cstdint>
#include <vector>
#define GLFW_INCLUDE_VULKAN

#include "GLFW/glfw3.h"
#include "enginez/graphics/ez_types.hpp"
#include "logz/logger.hpp"
#include "vulkan/vulkan_core.h"
#include <string>
#include "enginez/ez_engine.hpp"
#include "enginez/graphics/ez_vulkan_backend.hpp"

namespace enginez::graphics {

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

            Semaphore swapchainSemaphore;
            Fence renderFence;
        };

      public:
        static inline const int FRAMES_IN_FLY = 1;
        static inline const VkExtent3D DRAW_IMAGE_EXTENT = {1920, 1200, 1};

        ezWindow(ezWindowCreateInfo& createInfo);
        void cleanUp();

        virtual void onUpdate() = 0;
        virtual void onOpen()   = 0;
        virtual void onClose()  = 0;

        void setHeight(int val);
        void setWidth(int val);
        void setTitle(std::string val);
        bool isClosed();

        bool shouldResize = false;
        bool inResizeFrame = false;
        uint32_t resizeCounter = 0;

      private:
        static void resizeStatic(GLFWwindow* window, int width, int height);
        static void cursorPosStatic(GLFWwindow* window, double xpos, double ypos);
        void resize();
        void internalUpdate();
        void init(ezVulkanBackend* backend);

        bool closed = false;

        void setupLogger();
        void createSwapchain();
        void createSurface();
        void setupFrameData();
        void setupDrawImage();
        void setupImgui();
        void setupCallbacks();
        void assignDebugNames();
        VkSurfaceFormatKHR getSuitableFormat(std::vector<VkSurfaceFormatKHR>& formats);
        VkPresentModeKHR choosePresentMode(std::vector<VkPresentModeKHR>& modes);
        VkExtent2D chooseSwapExtent(VkSurfaceCapabilitiesKHR& surfaceCapabilities);

        VkRenderingInfo imguiRenderInfo;
        VkRenderingAttachmentInfo colorAttchInfo;

        FrameData frameData[FRAMES_IN_FLY];
        std::vector<VkSemaphore> renderSemaphores;
        std::vector<VkImage> swapchainImages;

      protected:
        virtual void onMouseMoved(double xpos, double ypos){};
        virtual void onMouseDown(){};


        int currentFrame = 0;
        FrameData* currentFrameData;
        VkSemaphore renderSemaphore;
        Image drawImage;
        
        ezEngine& engine;
        ezVulkanBackend& backend;
        VmaAllocator& allocator;

        VkInstance& instance;
        Device& device;

        VkSurfaceCapabilitiesKHR surfaceCapabilities;

        logz::DefaultLogger& logger;
        VkSwapchainKHR swapchain;
        VkSurfaceKHR surface;
        VkExtent2D swapchainExtent;
        VkFormat swapchainFormat;
        VkColorSpaceKHR swapchainColorSpace;
        VkPresentModeKHR presentMode;
        std::string title;

        GLFWwindow* glfwWindow;

        Queue graphicsQueue;
        Queue presentQueue;

        std::vector<uint32_t> familyIndexes;
    };

} // namespace enginez::graphics