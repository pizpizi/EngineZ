#pragma once
#include "enginez/graphics/graphics_backend.hpp"
#include "logz/logger.hpp"
#include <vulkan/vulkan_core.h>

namespace enginez::graphics {

    class VulkanBackend : public GraphicsBackend {
      public:
        void init() override;
        EnginezWindow* createWindow(std::string title, int width, int height) override;
        void cleanUp() override;

      private:
        logz::DefaultLogger& logger = logz::createDefaultLogger(logz::SINCE_PROGRAM_START, "Vulkan");
        logz::DefaultLogger& validationLayerLogger = logz::createDefaultLogger(logz::SINCE_PROGRAM_START, "Validation Layer");

        VkInstance instance;
        VkDebugUtilsMessengerEXT debugMessenger;

        void setupLogger();
        void setupInstance();
        void setupDebugMessenger();

        VkBool32 debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                            VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                                                            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData);

        static VKAPI_ATTR VkBool32 VKAPI_CALL baseDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                            VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                                                            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
                                                          
    };

}; // namespace enginez::graphics