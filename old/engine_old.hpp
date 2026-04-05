#pragma once

#include "vulkan_utilities.hpp"
#include <cstdint>
#include <map>
#include <vector>
#include <vulkan/vulkan_core.h>
#include "logz/logger.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "result.hpp"

using namespace VulkanUtilities;

class Engine{
public:
    void run();

private:
    void initWindow();
    void initVulkan();
    
    // --- Create Functions
    void createInstance();
    void createSurface();
    void createSwapchain();
    void createLogicalDevice();
    void createPhysicalDevice();
    void setupDebugMessenger();
    void createGraphicsPipeLine();
    void createRenderPass();
    void createFramebuffers();
    void createCommandBuffers();
    void recordCommandBuffer();
    void createSynchronization();

    void draw();
    // --- Validation Functions
    Result checkLayerSupport();
    Result checkExtensionSupport(std::vector<const char*> requredExtensions);
    Result checkDeviceExtensionSupport(std::vector<const char*> requredExtensions, VkPhysicalDevice& device);
    
    // --- Helper Functions

    // --- --- Chose Functions
    VkExtent2D chooseSwapExtent(VkSurfaceCapabilitiesKHR& surfaceCapabilities);
    VkSurfaceFormatKHR chooseSwapchainFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    
    // --- --- Get Functions
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    std::vector<const char*> getRequiredGlfwExtensions();
    std::map<uint32_t, uint32_t> getRequiredQueueStructure();
    
    // --- --- Creation Info Functions
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    VkInstanceCreateInfo createInstanceInfo(VkApplicationInfo* appInfo);
    VkApplicationInfo createAppInfo();

    // --- --- Creator Functions
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags asspectFlags);
    VkShaderModule createShaderModule(const char* source);

    void cleanUp();

    // --- Fields
    GLFWwindow* window;
    VkInstance vkInstance;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice logicalDevice;
    VkExtent2D swapchainExtent;
    VkFormat swapchainFormat;
    VkPipelineLayout pipelineLayout;
    VkRenderPass renderPass;
    VkPipeline graphicsPipeline;

    uint32_t currentFrame = 0;

    std::vector<VkSemaphore> imageAcquireSemaphores, renderSemaphores;
    std::vector<VkFence> renderFences;
    
    // --- --- Queues
    VkQueue graphicsQueue;
    VkQueue computeQueue;
    VkQueue presentQueue;

    uint32_t graphicsQueueFamily;
    uint32_t computeQueueFamily;
    uint32_t presentQueueFamily;
    
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;
    std::vector<ImageViewPair> swapchainImages;
    std::vector<VkFramebuffer> framebuffers;
    std::vector<VkCommandBuffer> commandBuffers;
    VkCommandPool commandPool;
    
    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };
    std::vector<const char*> requiredDeviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    std::vector<const char*> requiredExtensions = {
        
    };
    std::map<VkExtendedQueueFlagBits, uint32_t> requiredQueues{
        {GRAPHICS, 1},
        {COMPUTE, 1},
        {PRESENT, 1}
    };
    VkDebugUtilsMessengerEXT debugMessenger;


    const uint32_t WIDTH=800, HEIGHT=600;
    const bool DEBUG_ENABLED = true;

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback( VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                        VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                        void* pUserData);
};