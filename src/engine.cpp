#include <X11/X.h>
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <cstring>
#include <format>
#include <ostream>
#include <set>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "enginez/engine.hpp"
#include "enginez/result.hpp"
#include "GLFW/glfw3.h"

using std::cout, std::endl, std::vector, std::min, std::max;

std::chrono::time_point<std::chrono::system_clock> prevTime, curTime;

void Engine::run(){
    this->initWindow();
    this->initVulkan();

    prevTime = std::chrono::system_clock::now();

    while (!glfwWindowShouldClose(window)){
        glfwPollEvents();
        draw();

    }
    // vkGetDeviceGroupSurfacePresentModesKHR(VkDevice device, VkSurfaceKHR surface, VkDeviceGroupPresentModeFlagsKHR *pModes)

    glfwSetWindowUserPointer(window, this);
    // glfwSetWindowSizeCallback(window, (GLFWwindow* window, int width, int height) {})
    cleanUp();
}

void Engine::initWindow(){
    glfwInit();
    
    cout<<"Glfw initialized"<<endl;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Engine", nullptr, nullptr);

    if(window == nullptr){
        glfwTerminate();
        throw std::runtime_error("Failed to create the window");
    }


    cout<<"Glfw Window created"<<endl;
}

// ---------------------------------------------------------------------------------------
// -------------------------------- Instance Creation ------------------------------------
// ---------------------------------------------------------------------------------------

vector<const char*> Engine::getRequiredGlfwExtensions(){
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    
    vector<const char*> requiredExtensions = vector<const char*>(); 
    
    for(int i = 0 ; i < glfwExtensionCount; i++){
        requiredExtensions.push_back(glfwExtensions[i]);
    }
    
    requiredExtensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME); //for macos moltenVk
    
    return requiredExtensions;
}
Result Engine::checkExtensionSupport(vector<const char*> requiredExtensions){
    uint32_t availableExtensionsCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionsCount, nullptr);
    vector<VkExtensionProperties> availableExtensions(availableExtensionsCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionsCount, availableExtensions.data());
    
    cout<<"Available extensions:"<<endl;
    for(int j = 0 ; j < availableExtensions.size(); j++){
        cout <<"\t"<<availableExtensions[j].extensionName<<endl;
    }
    
    for(int i = 0 ; i < requiredExtensions.size(); i++){
        bool found = false;
        for(int j = 0 ; j < availableExtensions.size(); j++){
            if(strcmp(availableExtensions[j].extensionName, requiredExtensions[i]) == 0){
                found = true;
                break;
            }
        }
        if(!found){
            return {false, std::format("Extension \"{}\" is not available", requiredExtensions[i])};
        }
    }
    
    return {true};
}
Result Engine::checkLayerSupport(){
    uint32_t availableLayersCount;
    vkEnumerateInstanceLayerProperties(&availableLayersCount, nullptr);
    vector<VkLayerProperties> availableLayers = vector<VkLayerProperties>(availableLayersCount);
    vkEnumerateInstanceLayerProperties(&availableLayersCount, availableLayers.data());
    
    for(int i = 0; i < validationLayers.size(); i++){
        bool found = false;
        for(int j = 0 ; j < availableLayersCount; j++){
            if(strcmp(availableLayers[j].layerName, validationLayers[i]) == 0){
                found = true;
                break;
            }
        }
        if(!found){
            return {false, (std::format("Validation Layer \"{}\" is not available", validationLayers[i]))};
        }
    }
    return {true};
}
VkApplicationInfo Engine::createAppInfo(){
    VkApplicationInfo appInfo = VkApplicationInfo();
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    
    appInfo.pApplicationName = "Engine";
    appInfo.pEngineName      = "Engine";
    appInfo.engineVersion       = VK_MAKE_VERSION(0, 0, 0);
    appInfo.applicationVersion  = VK_MAKE_VERSION(0, 0, 0);
    appInfo.apiVersion          = VK_API_VERSION_1_3;   
    
    return appInfo;
}
VkInstanceCreateInfo Engine::createInstanceInfo(VkApplicationInfo* appInfo){
    VkInstanceCreateInfo createInfo = VkInstanceCreateInfo();
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = appInfo;
    
    vector<const char*> glfwRequiredExtensions = getRequiredGlfwExtensions();
    Result result = checkExtensionSupport(glfwRequiredExtensions);
    
    requiredExtensions.insert(requiredExtensions.begin(), glfwRequiredExtensions.begin(), glfwRequiredExtensions.end());
    if(DEBUG_ENABLED){
        requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    if(!result.success){
        throw std::runtime_error(result.message);
    }
    cout<<"All required extensions are available:"<<endl;
    for(int i = 0 ; i < requiredExtensions.size(); i++){
        cout<<"\t"<<requiredExtensions[i]<<endl;
    }
    const char** extArray = new const char*[requiredExtensions.size()];
    for(size_t i = 0; i < requiredExtensions.size(); i++){
        extArray[i] = requiredExtensions[i];
    }
    createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
    createInfo.ppEnabledExtensionNames = extArray;
    
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    
    createInfo.enabledLayerCount = 0;
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if(DEBUG_ENABLED){
        result = checkLayerSupport();
        if(!result.success){
            throw std::runtime_error(result.message);
        }

        cout<<"All required validation layers are available:"<<endl;
        for(int i = 0 ; i < validationLayers.size(); i++){
            cout<<"\t"<<validationLayers[i]<<endl;
        }
        
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
        
        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
    }
    
    return createInfo;
}
void Engine::createInstance(){
    VkApplicationInfo appInfo = createAppInfo();
    VkInstanceCreateInfo instanceInfo = createInstanceInfo(&appInfo);
    
    if(vkCreateInstance(&instanceInfo, nullptr, &vkInstance) != VK_SUCCESS){
        throw std::runtime_error("failed to create Vulkan instance");
    }
}

// ---------------------------------------------------------------------------------------
// -------------------------------- Debug Setup ------------------------------------------
// ---------------------------------------------------------------------------------------

VKAPI_ATTR VkBool32 VKAPI_CALL Engine::debugCallback( 
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {

        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
    }

void Engine::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo){
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | 
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    // VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr;
}
void Engine::setupDebugMessenger(){
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    populateDebugMessengerCreateInfo(createInfo);
    

    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(vkInstance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        if(func(vkInstance, &createInfo, nullptr, &debugMessenger)!=VK_SUCCESS){
            throw std::runtime_error("Couldn't setup the debug messenger");
        }
    } else {
        throw std::runtime_error("Couldn't setup the debug messenger: VK_ERROR_EXTENSION_NOT_PRESENT");
    }
}

// ---------------------------------------------------------------------------------------
// --------------------------- Physical Device Setup -------------------------------------
// ---------------------------------------------------------------------------------------

Result Engine::checkDeviceExtensionSupport(vector<const char*> requiredExtensions, VkPhysicalDevice& device){
    uint32_t availableExtensionsCount = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &availableExtensionsCount, nullptr);
    vector<VkExtensionProperties> availableExtensions(availableExtensionsCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &availableExtensionsCount, availableExtensions.data());
    
    for(auto req: requiredDeviceExtensions){
        bool found = false;
        for(auto ava: availableExtensions){
            if(!strcmp(ava.extensionName, req)){
                found = true;
                break;
            }
        }
        if(!found){
            return {false, std::format("\tExtension \"{}\" is not available", req)};
        }
    }
    std::string out = "\tAll required device extentions are available\n";
    for(auto req: requiredDeviceExtensions){
        out += "\t\t";
        out += req;
        out += "\n";
    }
    return {true, out};
}



void Engine::createPhysicalDevice(){
    uint32_t physicalDeviceCount;
    vkEnumeratePhysicalDevices(vkInstance, &physicalDeviceCount, nullptr);
    if (physicalDeviceCount == 0) {
        throw std::runtime_error("Failed to find GPUs with Vulkan support");
    }
    
    vector<VkPhysicalDevice> availableDevices = vector<VkPhysicalDevice>(physicalDeviceCount);
    vkEnumeratePhysicalDevices(vkInstance, &physicalDeviceCount, availableDevices.data());
    
    cout<<"Available physical devices|================"<<endl;
    int maxScore = 0;
    for(int i = 0 ; i < physicalDeviceCount; i++){
        VkPhysicalDeviceProperties properties;
        VkPhysicalDeviceMemoryProperties memoryProperties;
        vkGetPhysicalDeviceProperties(availableDevices[i], &properties);
        vkGetPhysicalDeviceMemoryProperties(availableDevices[i], &memoryProperties);

        uint32_t queueFamilyCount;
        vkGetPhysicalDeviceQueueFamilyProperties(availableDevices[i], &queueFamilyCount, nullptr);
        vector<VkQueueFamilyProperties> families = vector<VkQueueFamilyProperties>(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(availableDevices[i], &queueFamilyCount, families.data());
        VkDeviceSize totalVRAM = 0;

        for (uint32_t i = 0; i < memoryProperties.memoryHeapCount; i++) {
            VkMemoryHeap heap = memoryProperties.memoryHeaps[i];

            if (heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                totalVRAM += heap.size;
            }
        }

        int score = 0;

        QueueFamilyIndices queueFamilyIndices = QueueFamilyIndices(availableDevices[i], &surface);
        cout<<std::format("    {}.{}\n        Id: {}\n        Memory: {}\n        Queue families:\n", i, properties.deviceName, properties.deviceID, totalVRAM);
        for(int j = 0 ; j < families.size(); j++){
            cout<<std::format("            {}.\n                Queue count: {}\n                Flag Bits: {}\n", j, families[j].queueCount, families[j].queueFlags);
        }

        queueFamilyIndices.print(1);

        Result extensionSupport = checkDeviceExtensionSupport(requiredDeviceExtensions, availableDevices[i]);
        
        if(properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) score += 1;
        if((queueFamilyIndices[COMPUTE].size() != 0) && (queueFamilyIndices[GRAPHICS].size() != 0)) score += 1;

        cout<<extensionSupport.message<<endl;
        if(extensionSupport.success) score++;

        auto swapChainSupport = querySwapChainSupport(availableDevices[i]);

        cout<<"\tSupported present modes:"<<endl;
        for(auto presentMode: swapChainSupport.presentModes){
            cout<<"\t\t"<<presentMode<<endl;
        }

        cout<<"\tSupported formats:"<<endl;
        for(auto format: swapChainSupport.formats){
            cout<<"\t\tFormat: "<<format.format<<" Color Space: "<< format.colorSpace<<endl;
        }

        cout<<"\tOther swapchain details:"<<endl;
        cout<<"\t\tMin image extent: "<< swapChainSupport.capabilities.minImageExtent.width << " x " << 
                swapChainSupport.capabilities.minImageExtent.height<<endl;
        cout<<"\t\tCurrent image extent: "<< swapChainSupport.capabilities.currentExtent.width << " x " << 
                swapChainSupport.capabilities.currentExtent.height<<endl;
        cout<<"\t\tMax image extent: "<< swapChainSupport.capabilities.maxImageExtent.width << " x " << 
                swapChainSupport.capabilities.maxImageExtent.height<<endl;
        cout<<"\t\tImage count: "<< swapChainSupport.capabilities.minImageCount << " to " << 
                swapChainSupport.capabilities.maxImageCount<<endl;
        cout<<"\t\tMax image array layers: "<< swapChainSupport.capabilities.maxImageArrayLayers<<endl;

        if(!swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty()){
            score++;
        }
        
        if(score > maxScore){
            maxScore = score;
            this->physicalDevice = availableDevices[i];
        }
    }

    if(maxScore < 4){
        throw std::runtime_error("No Devices found with the minmum requirements");
    }
}

// ---------------------------------------------------------------------------------------
// -------------------------- Logical Device Creation ------------------------------------
// ---------------------------------------------------------------------------------------

void Engine::createLogicalDevice(){
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    
    QueueFamilyIndices queueFamilyIndices = QueueFamilyIndices(physicalDevice, &surface);
    vector<VkDeviceQueueCreateInfo> queueCreateInfos = {};
    std::set<uint32_t> uniqueQueueFamilies = {queueFamilyIndices[GRAPHICS][0], queueFamilyIndices[COMPUTE][0]
        , queueFamilyIndices[PRESENT][0]};

    graphicsQueueFamily = queueFamilyIndices[GRAPHICS][0];
    computeQueueFamily  = queueFamilyIndices[COMPUTE][0];
    presentQueueFamily  = queueFamilyIndices[PRESENT][0];
    
    //TODO this is shit

    const float queuePriority[] = {1.0, 1.0, 1.0};
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 3;
        queueCreateInfo.pQueuePriorities = queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};

    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtensions.size());
    createInfo.ppEnabledExtensionNames = requiredDeviceExtensions.data();

    if (DEBUG_ENABLED) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    if(vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice) != VK_SUCCESS){
        throw std::runtime_error("Failed to create a logical device");
    }

    vkGetDeviceQueue(logicalDevice, queueFamilyIndices[GRAPHICS][0], 0, &graphicsQueue);
    vkGetDeviceQueue(logicalDevice, queueFamilyIndices[PRESENT][0], 1, &presentQueue);
    vkGetDeviceQueue(logicalDevice, queueFamilyIndices[COMPUTE][0], 2, &computeQueue);

    cout<< graphicsQueue << " " << presentQueue << " " << computeQueue <<endl;
}

// ---------------------------------------------------------------------------------------
// -------------------------------- Swapchain Setup --------------------------------------
// ---------------------------------------------------------------------------------------

SwapChainSupportDetails Engine::querySwapChainSupport(VkPhysicalDevice device){
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}
VkSurfaceFormatKHR Engine::chooseSwapchainFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats){
    if(availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED){
        return {VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    }
    for (const auto& availableFormat : availableFormats) {
        cout << availableFormat.format << " " << availableFormat.colorSpace << endl;
        if ((availableFormat.format == VK_FORMAT_R8G8B8A8_UNORM || availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM) 
            && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}
VkPresentModeKHR Engine::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes){
    // for (const auto& availablePresentMode : availablePresentModes) {
    //     if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
    //         return availablePresentMode;
    //     }
    // }

    return VK_PRESENT_MODE_MAILBOX_KHR;
}
VkExtent2D Engine::chooseSwapExtent(VkSurfaceCapabilitiesKHR& surfaceCapabilities){
    if(surfaceCapabilities.currentExtent.width != UINT32_MAX){
        return surfaceCapabilities.currentExtent;
    }else{
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        VkExtent2D extent{};
        extent.width  = static_cast<uint32_t>(width);
        extent.height = static_cast<uint32_t>(height);

        extent.width  = max(surfaceCapabilities.minImageExtent.width, min(surfaceCapabilities.maxImageExtent.width, extent.width));
        extent.height = max(surfaceCapabilities.minImageExtent.height, min(surfaceCapabilities.maxImageExtent.height, extent.height));

        return extent;
    }
}
void Engine::createSwapchain(){
    auto swapchainSupportDetails = querySwapChainSupport(physicalDevice);
    auto chosenFormat = chooseSwapchainFormat(swapchainSupportDetails.formats);
    auto extent = chooseSwapExtent(swapchainSupportDetails.capabilities);
    
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.presentMode = chooseSwapPresentMode(swapchainSupportDetails.presentModes);
    createInfo.imageFormat = chosenFormat.format;
    createInfo.imageColorSpace = chosenFormat.colorSpace;
    createInfo.surface = surface;
    createInfo.imageExtent = extent;
    createInfo.preTransform = swapchainSupportDetails.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.imageArrayLayers = 1;
    createInfo.minImageCount = swapchainSupportDetails.capabilities.minImageCount + 1;

    if(swapchainSupportDetails.capabilities.maxImageCount){
        createInfo.minImageCount = min(swapchainSupportDetails.capabilities.maxImageCount, 
            swapchainSupportDetails.capabilities.minImageCount + 1);
    }
    createInfo.clipped = VK_TRUE;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    
    if(graphicsQueueFamily != presentQueueFamily){
        uint32_t queueFamilyIndices[] = {graphicsQueueFamily, presentQueueFamily};

        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;

        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }else{
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }
    createInfo.oldSwapchain = VK_NULL_HANDLE;
    
    if(vkCreateSwapchainKHR(logicalDevice , &createInfo, nullptr, &swapchain) != VK_SUCCESS){
        throw std::runtime_error("Failed to create the swapchain");
    }

    swapchainFormat = chosenFormat.format;
    swapchainExtent = extent;

    uint32_t swapchainImageCount;
    vkGetSwapchainImagesKHR(logicalDevice, swapchain, &swapchainImageCount, nullptr);
    vector<VkImage> images(swapchainImageCount);
    vkGetSwapchainImagesKHR(logicalDevice, swapchain, &swapchainImageCount, images.data());

    for(VkImage image : images){
        auto view = createImageView(image, swapchainFormat, VK_IMAGE_ASPECT_COLOR_BIT);
        swapchainImages.push_back({image, view});
    }
}

// ---------------------------------------------------------------------------------------
// --------------------------- framebuffers setup ----------------------------------------
// ---------------------------------------------------------------------------------------

void Engine::createFramebuffers(){
    framebuffers.resize(swapchainImages.size());

    for(int i = 0 ; i < framebuffers.size(); i++){
        VkFramebufferCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        createInfo.attachmentCount = 1;
        createInfo.pAttachments = &(swapchainImages[i].view);
        createInfo.height = swapchainExtent.height;
        createInfo.width = swapchainExtent.width;
        createInfo.renderPass = renderPass;
        createInfo.layers = 1;

        if (vkCreateFramebuffer(logicalDevice, &createInfo,nullptr, &(framebuffers[i])) != VK_SUCCESS){
            throw std::runtime_error("Failed to create the framebuffers");
        }
    }
}

// ---------------------------------------------------------------------------------------
// --------------------------- commandbuffers setup --------------------------------------
// ---------------------------------------------------------------------------------------

void Engine::createCommandBuffers(){
    commandBuffers.resize(swapchainImages.size());
    
    VkCommandPoolCreateInfo poolCreateInfo = {};
    poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolCreateInfo.queueFamilyIndex = graphicsQueueFamily;

    if ( vkCreateCommandPool(logicalDevice, &poolCreateInfo, nullptr, &commandPool) != VK_SUCCESS){
        throw std::runtime_error("Failed to create the command pool");
    }


    for(int i = 0 ; i < commandBuffers.size(); i++){
        VkCommandBufferAllocateInfo allocateInfo = {};
        allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocateInfo.commandPool = commandPool;
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocateInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(logicalDevice, &allocateInfo, &(commandBuffers[i])) != VK_SUCCESS){
            throw std::runtime_error("Failed to create the command buffers");
        }
    }
}

// ---------------------------------------------------------------------------------------
// -------------------------------- Surface Setup ----------------------------------------
// ---------------------------------------------------------------------------------------

void Engine::createSurface(){
    if (glfwCreateWindowSurface(vkInstance, window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}

VkImageView Engine::createImageView(VkImage image, VkFormat format, VkImageAspectFlags asspectFlags){
    VkImageViewCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;

    createInfo.image = image;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = format;
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    
    createInfo.subresourceRange.aspectMask = asspectFlags;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    
    if(vkCreateImageView(logicalDevice, &createInfo, nullptr, &imageView) != VK_SUCCESS){
        throw std::runtime_error("Failed to create the image view");
    }

    return imageView;
}

VkShaderModule Engine::createShaderModule(const char* source){
    FILE *file = std::fopen(source, "rb");
    long startPos, endPos;
    startPos = std::ftell(file);
    std::fseek(file, 0, SEEK_END);
    endPos = std::ftell(file);

    long size = (endPos - startPos);
    char buffer[size];
    fseek(file, 0, SEEK_SET);
    fread(buffer, sizeof(char), size, file);
    fclose(file);

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.pCode = reinterpret_cast<const uint32_t*>(buffer);
    createInfo.codeSize = size;

    VkShaderModule module;

    if(vkCreateShaderModule(logicalDevice, &createInfo, nullptr, &module) != VK_SUCCESS){
        throw std::runtime_error(std::format("Failed to create a shader module from {}", source));
    }

    return module;
}

void Engine::createGraphicsPipeLine(){
    auto vertShader = createShaderModule("./examples/shaders/spirv/vert.spv");
    auto fragShader = createShaderModule("./examples/shaders/spirv/frag.spv");

    VkPipelineShaderStageCreateInfo vertexShaderCreateInfo{};
    vertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexShaderCreateInfo.module = vertShader;
    vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexShaderCreateInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderCreateInfo{};
    fragShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderCreateInfo.module = fragShader;
    fragShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderCreateInfo.pName = "main";
    
    VkPipelineShaderStageCreateInfo shaderCreateInfos[] = {vertexShaderCreateInfo, fragShaderCreateInfo};

    // Vertex Input
    VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo{};
    vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputCreateInfo.vertexBindingDescriptionCount = 0;
    vertexInputCreateInfo.pVertexBindingDescriptions = nullptr;
    vertexInputCreateInfo.vertexAttributeDescriptionCount = 0;
    vertexInputCreateInfo.pVertexAttributeDescriptions = nullptr;

    // Input Assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo{};
    inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

    // Viewport and scissor
    // Viewport
    VkViewport viewport{};
    viewport.x = 0.0;
    viewport.y = 0.0;
    viewport.height = (float) swapchainExtent.height;
    viewport.width = (float) swapchainExtent.width;
    viewport.maxDepth = 1.0;
    viewport.minDepth = 0.0;

    // Scissor
    VkRect2D scissor{};
    scissor.extent = swapchainExtent;
    scissor.offset = {0, 0};

    VkPipelineViewportStateCreateInfo viewportStateCreateInfo{};
    viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCreateInfo.viewportCount = 1;
    viewportStateCreateInfo.pViewports = &viewport;
    viewportStateCreateInfo.scissorCount = 1;
    viewportStateCreateInfo.pScissors = &scissor;

    // Dynamic States
    vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
    dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCreateInfo.dynamicStateCount = dynamicStates.size();
    dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo{};
    rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizerCreateInfo.depthBiasEnable = VK_FALSE;
    rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL; // This needs Gpu Features for other modes 
    rasterizerCreateInfo.lineWidth = 1.0;                    // Other values need a gpu feature
    rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizerCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizerCreateInfo.depthBiasEnable = VK_FALSE;         // what is this? needs a feature. its for resolving "shadow acne" 

    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo{};
    multisamplingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisamplingCreateInfo.sampleShadingEnable = VK_FALSE;
    multisamplingCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Blending
    VkPipelineColorBlendAttachmentState colorState{};
    colorState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorState.blendEnable = VK_TRUE;
    colorState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorState.colorBlendOp        = VK_BLEND_OP_ADD;
    colorState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorState.alphaBlendOp        = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{};
    colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
    colorBlendStateCreateInfo.attachmentCount = 1;
    colorBlendStateCreateInfo.pAttachments    = &colorState;

    // Pipeline Layout
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 0;
    pipelineLayoutCreateInfo.pSetLayouts = nullptr;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

    if(vkCreatePipelineLayout(logicalDevice, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout) != VK_SUCCESS){
        throw std::runtime_error("Failed to create the graphics pipeline layout");
    }

    VkGraphicsPipelineCreateInfo graphicsPipeLineCreateInfo{};
    graphicsPipeLineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphicsPipeLineCreateInfo.stageCount = 2;
    graphicsPipeLineCreateInfo.pStages = shaderCreateInfos;
    graphicsPipeLineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
    graphicsPipeLineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
    graphicsPipeLineCreateInfo.pViewportState = &viewportStateCreateInfo;
    graphicsPipeLineCreateInfo.pDynamicState = nullptr;
    graphicsPipeLineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
    graphicsPipeLineCreateInfo.pMultisampleState = &multisamplingCreateInfo;
    graphicsPipeLineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
    graphicsPipeLineCreateInfo.pDepthStencilState = nullptr;
    graphicsPipeLineCreateInfo.layout = pipelineLayout;
    graphicsPipeLineCreateInfo.renderPass = renderPass;
    graphicsPipeLineCreateInfo.subpass = 0; // why? what is the point of dependencies then????!!!?!?!?!?!?!?

    graphicsPipeLineCreateInfo.basePipelineHandle = nullptr;
    graphicsPipeLineCreateInfo.basePipelineIndex = -1; 

    if (vkCreateGraphicsPipelines(logicalDevice, nullptr, 1, &graphicsPipeLineCreateInfo, nullptr, &graphicsPipeline) != VK_SUCCESS){
        throw std::runtime_error("Failed to create the graphics pipeline");
    }

    vkDestroyShaderModule(logicalDevice, vertShader, nullptr);
    vkDestroyShaderModule(logicalDevice, fragShader, nullptr);
}

void Engine::createRenderPass(){
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapchainFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRefrence = {};
    colorAttachmentRefrence.attachment = 0;
    colorAttachmentRefrence.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRefrence;

    // Dependencies
    vector<VkSubpassDependency> dependencies = {{}, {}};

    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    
    dependencies[0].dstSubpass = 0;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = 0;

    dependencies[1].srcSubpass = 0;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[1].dependencyFlags = 0;

    VkRenderPassCreateInfo renderPassCreateInfo{};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = 1;
    renderPassCreateInfo.pAttachments = &colorAttachment;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpass;
    renderPassCreateInfo.dependencyCount = 2;
    renderPassCreateInfo.pDependencies = dependencies.data();

    if (vkCreateRenderPass(logicalDevice, &renderPassCreateInfo, nullptr, &renderPass) != VK_SUCCESS){
        throw std::runtime_error("Failed to create the renderpass");
    }
}

// ---------------------------------------------------------------------------------------
// ------------------------------------- Draw --------------------------------------------
// ---------------------------------------------------------------------------------------

void Engine::recordCommandBuffer(){
    // cout << vkimage(logicalDevice, swapchain) << endl;
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderArea.extent = swapchainExtent;
    renderPassBeginInfo.renderArea.offset = {0, 0};
    renderPassBeginInfo.clearValueCount = 1;
    VkClearValue clearValues[] = {
        {0.0, 0.0, 0.0, 1.0}
    };
    renderPassBeginInfo.pClearValues = clearValues;
    renderPassBeginInfo.renderPass = renderPass;

    for (int i = 0 ; i < commandBuffers.size(); i++){
        VkResult result = vkBeginCommandBuffer(commandBuffers[i], &beginInfo);
        renderPassBeginInfo.framebuffer = framebuffers[i];

        if (result != VK_SUCCESS){
            throw std::runtime_error("Failed to begin the command buffer");
        }

        {
            vkCmdBeginRenderPass(commandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

            {
                vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
                vkCmdDraw(commandBuffers[i], 3, 1, 0, 0);
            }

            vkCmdEndRenderPass(commandBuffers[i]);
        }

        result = vkEndCommandBuffer(commandBuffers[i]);
        if (result != VK_SUCCESS){
            throw std::runtime_error("Failed to begin the command buffer");
        }
    }
}

void Engine::createSynchronization(){
    imageAcquireSemaphores.resize(swapchainImages.size());
    renderSemaphores.resize(swapchainImages.size());
    renderFences.resize(swapchainImages.size());
    
    VkSemaphoreCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (int i = 0 ; i < swapchainImages.size(); i++){
        if (vkCreateSemaphore(logicalDevice, &createInfo, nullptr, &(imageAcquireSemaphores[i])) != VK_SUCCESS 
            || vkCreateSemaphore(logicalDevice, &createInfo, nullptr, &(renderSemaphores[i])) != VK_SUCCESS
            || vkCreateFence(logicalDevice, &fenceCreateInfo, nullptr, &(renderFences[i])) != VK_SUCCESS){
            throw std::runtime_error("Failed to create the semaphores");
        }
    }

}

void Engine::draw(){
    // vkgetSwapchaintimin
    uint32_t imageIndex;
    
    prevTime = std::chrono::system_clock::now();
    vkWaitForFences(logicalDevice, 1, &(renderFences[currentFrame]), VK_TRUE, UINT64_MAX);
    vkResetFences(logicalDevice, 1, &(renderFences[currentFrame]));
    curTime = std::chrono::system_clock::now();
    auto wait = (std::chrono::duration_cast<std::chrono::microseconds>(curTime - prevTime).count());

    prevTime = std::chrono::system_clock::now();
    vkAcquireNextImageKHR(logicalDevice, swapchain, UINT64_MAX, imageAcquireSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
    curTime = std::chrono::system_clock::now();
    auto acquire = (std::chrono::duration_cast<std::chrono::microseconds>(curTime - prevTime).count());

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &(commandBuffers[imageIndex]);
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &(imageAcquireSemaphores[currentFrame]);
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderSemaphores[currentFrame];
    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };
    submitInfo.pWaitDstStageMask = waitStages;
    
    prevTime = std::chrono::system_clock::now();
    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, renderFences[currentFrame]) != VK_SUCCESS){
        throw std::runtime_error("Failed to submit to graphics queue");
    }
    curTime = std::chrono::system_clock::now();
    auto submit = (std::chrono::duration_cast<std::chrono::microseconds>(curTime - prevTime).count());
    
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pWaitSemaphores = &renderSemaphores[currentFrame];
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &imageIndex;
    
    prevTime = std::chrono::system_clock::now();
    if (vkQueuePresentKHR(presentQueue, &presentInfo) != VK_SUCCESS){
        throw std::runtime_error("Failed to submit to present queue");
    }
    curTime = std::chrono::system_clock::now();
    auto present = (std::chrono::duration_cast<std::chrono::microseconds>(curTime - prevTime).count());
    
    cout << "wait: " << wait << " acquire: " << acquire << " submit: " << submit << " present: " << present << " frame: " << currentFrame << endl;
    currentFrame = (currentFrame + 1) % swapchainImages.size();
}
// acquire: 19 submit: 9 present: 1534
// acquire: 17 submit: 10 present: 2080
// acquire: 25 submit: 13 present: 356
// acquire: 19 submit: 11 present: 1918
// acquire: 21 submit: 8 present: 1381
// acquire: 13 submit: 27 present: 388
// acquire: 18 submit: 15 present: 3744
// acquire: 14 submit: 13 present: 1172
// acquire: 83 submit: 10 present: 804
// acquire: 24 submit: 13 present: 3692


// acquire: 6 submit: 15 present: 5801
// acquire: 9 submit: 19 present: 6280
// acquire: 10 submit: 19 present: 5513
// acquire: 8 submit: 16 present: 5889
// acquire: 7 submit: 14 present: 6957
// acquire: 12 submit: 27 present: 5533
// acquire: 9 submit: 17 present: 5458
// acquire: 5 submit: 10 present: 5974
// acquire: 5 submit: 9 present: 6586
void Engine::cleanUp(){
    vkDeviceWaitIdle(logicalDevice);

    for(int i = 0 ; i < framebuffers.size(); i++){
        vkDestroyFramebuffer(logicalDevice, framebuffers[i], nullptr);

        vkDestroySemaphore(logicalDevice, imageAcquireSemaphores[i], nullptr);
        vkDestroySemaphore(logicalDevice, renderSemaphores[i], nullptr);
    }
    vkDestroyCommandPool(logicalDevice, commandPool, nullptr);

    vkDestroyPipelineLayout(logicalDevice, pipelineLayout, nullptr);
    vkDestroyPipeline(logicalDevice, graphicsPipeline, nullptr);
    vkDestroyRenderPass(logicalDevice, renderPass, nullptr);
    for(auto imageViewPair : swapchainImages){
        vkDestroyImageView(logicalDevice, imageViewPair.view, nullptr);
    }
    
    vkDestroySwapchainKHR(logicalDevice, swapchain, nullptr);
    vkDestroySurfaceKHR(vkInstance, surface, nullptr);
    vkDestroyDevice(logicalDevice, nullptr);
    
    
    if(window != nullptr){
        glfwDestroyWindow(window);
    }
    
    if(DEBUG_ENABLED){
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(vkInstance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(vkInstance, debugMessenger, nullptr);
        }
    }

    vkDestroyInstance(vkInstance, nullptr);
    glfwTerminate();
}

void Engine::initVulkan(){
    createInstance();
    cout << "Created Vulkan instance"<<endl;
    
    if(DEBUG_ENABLED){
        setupDebugMessenger();
        cout << "The debug messenger was setup"<<endl;
    }

    createSurface();
    cout << "Created window surface"<<endl;

    createPhysicalDevice();
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
    cout << "Chosen physical device: "<<physicalDeviceProperties.deviceName<<endl;

    createLogicalDevice();
    cout << "Created the logical device"<<endl;

    createSwapchain();
    cout << "Created the swapchain" << endl;

    createRenderPass();
    cout << "Created the render pass" << endl;

    createGraphicsPipeLine();
    cout << "Created the graphics pipeline" << endl;

    createFramebuffers();
    cout << "Created the framebuffers" << endl;
    
    createCommandBuffers();
    cout << "Created the command buffers" << endl;

    recordCommandBuffer();
    cout << "Recorded the command buffer" << endl;

    createSynchronization();
    cout << "Created semaphores and fences" << endl;
}
