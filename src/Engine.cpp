#include <cstddef>
#include <cstdint>
#include <iostream>
#include <cstring>
#include <format>
#include <ostream>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "Engine.hpp"
#include "QueueFamilyIndecies.hpp"
#include "Result.hpp"
#include "VkExtendedQueueFlagBits.hpp"

using std::cout, std::endl, std::vector;

void Engine::run(){
    this->initWindow();
    this->initVulkan();

    while (!glfwWindowShouldClose(window)){
        glfwPollEvents();
    }

    cleanUp();
}

void Engine::initWindow(){
    glfwInit();
    
    cout<<"Glfw initialized"<<endl;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Engine", nullptr, nullptr);

    if(window == nullptr){
        glfwTerminate();
        throw std::runtime_error("Failed to create the window");
    }

    cout<<"Glfw Window created"<<endl;
}

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
void Engine::pickPhysicalDevice(){
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

    if(maxScore != 3){
        throw std::runtime_error("No Devices found with the minmum requirements");
    }
}
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
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}
VkPresentModeKHR Engine::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes){
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}
VkExtent2D Engine::chooseSwapExtent(const std::vector<VkPresentModeKHR>& availablePresentModes){
    
}
void Engine::createSwapchain(){
    
}
void Engine::createLogicalDevice(){
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    
    QueueFamilyIndices queueFamilyIndices = QueueFamilyIndices(physicalDevice, &surface);
    vector<VkDeviceQueueCreateInfo> queueCreateInfos = {};
    std::set<uint32_t> uniqueQueueFamilies = {queueFamilyIndices[GRAPHICS][0], queueFamilyIndices[COMPUTE][0]
        , queueFamilyIndices[PRESENT][0]};
    
    //TODO this is shit

    float queuePriority = 1.0;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 3;
        queueCreateInfo.pQueuePriorities = &queuePriority;
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
void Engine::createSurface(){
    if (glfwCreateWindowSurface(vkInstance, window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}
void Engine::initVulkan(){
    createInstance();
    cout<<"Created Vulkan instance"<<endl;

    setupDebugMessenger();
    cout<<"The debug messenger was setup"<<endl;

    createSurface();
    cout<<"Created window surface"<<endl;

    pickPhysicalDevice();
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
    cout<<"Chosen physical device: "<<physicalDeviceProperties.deviceName<<endl;

    createLogicalDevice();
    cout<<"Created the logical device"<<endl;
}
void Engine::cleanUp(){
    if(DEBUG_ENABLED){
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(vkInstance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(vkInstance, debugMessenger, nullptr);
        }
    }
    
    vkDestroyDevice(logicalDevice, nullptr);

    vkDestroySurfaceKHR(vkInstance, surface, nullptr);
    vkDestroyInstance(vkInstance, nullptr);

    if(window != nullptr){
        glfwDestroyWindow(window);
    }

    glfwTerminate();
}

VKAPI_ATTR VkBool32 VKAPI_CALL Engine::debugCallback( 
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {

        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
    }