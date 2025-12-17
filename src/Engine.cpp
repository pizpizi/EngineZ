#include <cstdint>
#include <iostream>
#include <cstring>
#include <format>
#include <ostream>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "Engine.hpp"
#include "QueueFamilyIndecies.hpp"
#include "Result.hpp"

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

vector<const char*> Engine::getRequiredExtentions(){
    uint32_t glfwExtentionCount = 0;
    const char** glfwExtentions;

    glfwExtentions = glfwGetRequiredInstanceExtensions(&glfwExtentionCount);

    vector<const char*> requiredExtentions = vector<const char*>(); 

    for(int i = 0 ; i < glfwExtentionCount; i++){
        requiredExtentions.push_back(glfwExtentions[i]);
    }

    requiredExtentions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME); //for macos moltenVk

    if(DEBUG_ENABLED){
        requiredExtentions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return requiredExtentions;
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
Result Engine::checkExtentionSupport(vector<const char*> requiredExtensions){
    uint32_t availableExtentionsCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &availableExtentionsCount, nullptr);
    vector<VkExtensionProperties> availableExtentions(availableExtentionsCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &availableExtentionsCount, availableExtentions.data());
    
    for(int i = 0 ; i < requiredExtensions.size(); i++){
        bool found = false;
        for(int j = 0 ; j < availableExtentions.size(); j++){
            if(strcmp(availableExtentions[j].extensionName, requiredExtensions[i]) == 0){
                found = true;
                break;
            }
        }
        if(!found){
            return {false, std::format("Extention \"{}\" is not available", requiredExtensions[i])};
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
    
    vector<const char*> requiredExtensions = getRequiredExtentions();
    Result result = checkExtentionSupport(requiredExtensions);

    if(!result.success){
        throw std::runtime_error(result.message);
    }
    cout<<"All required extentions are available:"<<endl;
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
                                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
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

        cout<<std::format("    {}.{}\n        Id:{}\n        Memory: {}\n        Queue families:\n", i, properties.deviceName, properties.deviceID, totalVRAM);
        for(int j = 0 ; j < families.size(); j++){
            cout<<std::format("            {}.\n                Queue count: {}\n                Flag Bits: {}\n", j, families[j].queueCount, families[j].queueFlags);
        }

        QueueFamilyIndices queueFamilyIndices = QueueFamilyIndices(availableDevices[i]);
        queueFamilyIndices.print(1);
        if(properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) score += 1;
        if((queueFamilyIndices[VK_QUEUE_COMPUTE_BIT].size() != 0) && (queueFamilyIndices[VK_QUEUE_GRAPHICS_BIT].size() != 0)) score += 1;

        if(score > maxScore){
            maxScore = score;
            this->physicalDevice = availableDevices[i];
        }
    }

    if(maxScore != 2){
        throw std::runtime_error("No Devices found with the minmum requirements");
    }
}
void Engine::createLogicalDevice(){
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    QueueFamilyIndices queueFamilyIndices = QueueFamilyIndices(physicalDevice);
    queueCreateInfo.queueFamilyIndex = queueFamilyIndices[VK_QUEUE_COMPUTE_BIT][0];
    queueCreateInfo.queueCount = 1;

    float queuePriority = 1.0;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkPhysicalDeviceFeatures deviceFeatures{};


    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = 0;

    if (DEBUG_ENABLED) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    if(vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice) != VK_SUCCESS){
        throw std::runtime_error("Failed to create a logical device");
    }

    vkGetDeviceQueue(logicalDevice, queueFamilyIndices[VK_QUEUE_GRAPHICS_BIT][0], 0, &graphicsQueue);
}
void Engine::createSurface(){
    
}
void Engine::initVulkan(){
    createInstance();
    cout<<"Created Vulkan instance"<<endl;

    setupDebugMessenger();
    cout<<"The debug messenger was setup"<<endl;

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