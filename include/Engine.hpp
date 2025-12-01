#pragma once

#include <iostream>
#include <vector>
#define GLFW_INCLUDE_VULKAN
#include"GLFW/glfw3.h"
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

    VkResult setupDebugMessenger();

    void cleanUp();

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback( 
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {

        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
    }

    GLFWwindow* window;
    VkInstance vkInstance;

    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };
    VkDebugUtilsMessengerEXT debugMessenger;

    const uint32_t WIDTH=800, HEIGHT=600;
    const bool DEBUG_ENABLED = true;
};