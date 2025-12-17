#pragma once

#include "SwapchainDetails.hpp"
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
    
    std::vector<const char*> getRequiredGlfwExtensions();
    Result checkLayerSupport();
    Result checkExtensionSupport(std::vector<const char*> requredExtensions);
    Result checkDeviceExtensionSupport(std::vector<const char*> requredExtensions, VkPhysicalDevice& device);
    
    VkInstanceCreateInfo createInstanceInfo(VkApplicationInfo* appInfo);
    VkApplicationInfo createAppInfo();
    void createInstance();

    void setupDebugMessenger();
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    void createSurface();

    void createSwapchain();
    VkSurfaceFormatKHR chooseSwapchainFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const std::vector<VkPresentModeKHR>& availablePresentModes);

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

    void pickPhysicalDevice();
    void createLogicalDevice();


    void cleanUp();

    GLFWwindow* window;
    VkInstance vkInstance;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice logicalDevice;
    
    VkQueue graphicsQueue;
    VkQueue computeQueue;
    VkQueue presentQueue;
    
    VkSurfaceKHR surface;
    
    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };
    std::vector<const char*> requiredDeviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    std::vector<const char*> requiredExtensions = {
        
    };
    VkDebugUtilsMessengerEXT debugMessenger;

    const uint32_t WIDTH=800, HEIGHT=600;
    const bool DEBUG_ENABLED = true;

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback( VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                        VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                        void* pUserData);
};