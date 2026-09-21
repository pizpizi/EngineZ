#pragma once

#include <cstdint>
#include <vector>
#define GLFW_INCLUDE_VULKAN

#include "GLFW/glfw3.h"
#include "enginez/ez_engine.hpp"
#include "enginez/graphics/ez_types.hpp"
#include "enginez/graphics/ez_vulkan_backend.hpp"
#include "logz/logger.hpp"
#include "vulkan/vulkan_core.h"
#include <string>

namespace enginez::graphics {

    struct ezWindowCreateInfo {
        Queue     graphicsQueue;
        Queue     presentQueue;
        ezEngine& engine;

        std::string title;
        uint32_t    width, height;
    };

    class ezWindow {
        friend ezVulkanBackend;

        struct FrameData {
            CommandPool   commandPool;
            CommandBuffer commandBuffer;

            Semaphore swapchainSemaphore;
            Fence     renderFence;
        };
//    +----------------------------------------------------+
//    |                       PUBLIC                       |
//    +----------------------------------------------------+
      public:

        ezWindow(ezWindowCreateInfo& createInfo);
        void cleanUp();

        bool isClosed();
//    +----------------------------------------------------+
//    |                       PRIVATE                      |
//    +----------------------------------------------------+
      private:
        // ----------- static vars ---------- //
        static inline const int        FRAMES_IN_FLY     = 1;
        static inline const VkExtent3D DRAW_IMAGE_EXTENT = {1920, 1200, 1};
        // ---------------------------------- //
        
        // -------------- init -------------- //
        void init(ezVulkanBackend* backend);
        void setupLogger();
        void createSwapchain();
        void createSurface();
        void setupFrameData();
        void setupDrawImage();
        void setupImgui();
        void setupCallbacks();
        void assignDebugNames();

        VkSurfaceFormatKHR getSuitableFormat(std::vector<VkSurfaceFormatKHR>& formats);
        VkPresentModeKHR   choosePresentMode(std::vector<VkPresentModeKHR>& modes);
        VkExtent2D         chooseSwapExtent(VkSurfaceCapabilitiesKHR& surfaceCapabilities);
        // ---------------------------------- //

        // ------- internal callbacks ------- //
        void internalUpdate();
        void internalResize();
        // ---------------------------------- //

        // -------- static callbacks -------- //
        static void cursorPosStatic(GLFWwindow* window, double xpos, double ypos);
        static void mouseButtonStatic(GLFWwindow* window, int button, int action, int mods);
        // ---------------------------------- //

        // ----------- render data ---------- //
        FrameData                 frameData[FRAMES_IN_FLY];
        std::vector<VkSemaphore>  renderSemaphores;
        std::vector<VkImage>      swapchainImages;
        VkRenderingInfo           imguiRenderInfo;
        VkRenderingAttachmentInfo colorAttchInfo;

        int         currentFrame = 0;
        FrameData*  currentFrameData;
        VkSemaphore renderSemaphore;
        Image       drawImage;
        // ---------------------------------- //

        bool closed       = false;
        bool shouldResize = false;

        VkSurfaceCapabilitiesKHR surfaceCapabilities;
//    +----------------------------------------------------+
//    |                      PROTECTED                     |
//    +----------------------------------------------------+
      protected:
        // ------------ callbacks ----------- //
        virtual void onMouseMoved(double xpos, double ypos) {};
        virtual void onMouseDown(int button, int action, int mods) {};
        virtual void draw(Image& drawImage) = 0;
        virtual void onOpen()   = 0;
        virtual void onClose()  = 0;
        // ---------------------------------- //

        std::string title;

        GLFWwindow*  glfwWindow;
        VkSurfaceKHR surface;

        VkSwapchainKHR   swapchain;
        VkExtent2D       swapchainExtent;
        VkFormat         swapchainFormat;
        VkColorSpaceKHR  swapchainColorSpace;
        VkPresentModeKHR presentMode;

        Queue graphicsQueue;
        Queue presentQueue;

        ezEngine&             engine;
        ezVulkanBackend&      backend;
        VmaAllocator&         allocator;
        VkInstance&           instance;
        Device&               device;
        std::vector<uint32_t> familyIndexes;
        logz::DefaultLogger&  logger;
    };
} // namespace enginez::graphics