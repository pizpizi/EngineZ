#pragma once

#include "GLFW/glfw3.h"
#include "logz/logger.hpp"
#include "vulkan/vulkan_core.h"
#include <string>

namespace enginez::graphics {
    class VulkanWindow {
      public:
        VulkanWindow(VkInstance& instance, std::string name, int width, int height);
        void cleanUp();
        void update();

        void setHeight(int val);
        void setWidth(int val);
        void setTitle(std::string val);
        bool isClosed();
      private:
        logz::DefaultLogger& logger;

        VkSwapchainKHR swapchain;
        VkSurfaceKHR surface;

        int width, height;
        std::string title;

        GLFWwindow* glfwWindow;
        bool closed = false;

        void setupLogger();
    };

} // namespace enginez::graphics