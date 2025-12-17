#pragma once

#include <vector>

#if defined(_WIN32)
    #define VK_USE_PLATFORM_WIN32_KHR
    #define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(linux)
    #define VK_USE_PLATFORM_XLIB_KHR
    #define GLFW_EXPOSE_NATIVE_X11
#endif

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Result.hpp"

class Engine{
public:
    void run();

private:
    void initWindow();
    void initVulkan();
    
    std::vector<const char*> getRequiredExtentions();
    Result checkLayerSupport();
    Result checkExtentionSupport(std::vector<const char*> requredExtentions);
    
    VkInstanceCreateInfo createInstanceInfo(VkApplicationInfo* appInfo);
    VkApplicationInfo createAppInfo();
    void createInstance();

    void setupDebugMessenger();
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    void createSurface();

    void pickPhysicalDevice();
    void createLogicalDevice();

    void cleanUp();

    GLFWwindow* window;
    VkInstance vkInstance;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice logicalDevice;
    VkQueue graphicsQueue;
    
    VkSurfaceKHR surface;
    
    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };
    VkDebugUtilsMessengerEXT debugMessenger;

    const uint32_t WIDTH=800, HEIGHT=600;
    const bool DEBUG_ENABLED = true;

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback( VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                        VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                        void* pUserData);
};