#include "vulkan_backend.hpp"
#include "GLFW/glfw3.h"
#include "enginez/graphics/buffer.hpp"
#include "enginez/graphics/enginez_window.hpp"
#include "logz/logger.hpp"
#include "vulkan/vulkan_buffer.hpp"
#include "vulkan/vulkan_window.hpp"
#include "vulkan_utilities.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <format>
#include <iostream>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>

using namespace enginez::graphics;
using namespace std;

void VulkanBackend::setupLogger() {
    logger.addConsoleSink(true, logz::DEBUG);
    logger.addFileSink("log.txt", logz::DEBUG);

    validationLayerLogger.addConsoleSink(true, logz::DEBUG);
    validationLayerLogger.addFileSink("log.txt", logz::DEBUG);
}

void VulkanBackend::init() {
    setupLogger();
    logger.info("vulkan logger setup.");

    if (glfwInit() != GLFW_TRUE) {
        logger.error("failed to initialize glfw");
    }
    logger.info("initialized glfw");

    try {
        setupInstance();
        setupDebugMessenger();
        setupPhysicalDevice();
        setupLogicalDevice();
    } catch (runtime_error e) {
        logger.error(e.what());
    }
}

void VulkanBackend::setupInstance() {
    uint32_t version;
    vkEnumerateInstanceVersion(&version);
    logger.info(format("Api version: {}.{}.{}", VK_API_VERSION_MAJOR(version), VK_API_VERSION_MINOR(version), VK_API_VERSION_PATCH(version)));

    VkInstanceCreateInfo instanceCreateInfo{};
    VkApplicationInfo appCreateInfo{};

    // ------------- data ------------ //
    vector<const char*> requiredInstanceExtensions = {VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
    vector<const char*> requiredInstanceLayers = {"VK_LAYER_KHRONOS_validation"};

    uint32_t availableExtentsionCount;
    vector<VkExtensionProperties> availableExtensions;
    vkEnumerateInstanceExtensionProperties(nullptr, &availableExtentsionCount, nullptr);
    availableExtensions.resize(availableExtentsionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &availableExtentsionCount, availableExtensions.data());

    vector<const char*> glfwRequiredExtensions = getGlfwRequiredExtensions();
    for (const char* ext : glfwRequiredExtensions) {
        requiredInstanceExtensions.push_back(ext);
    }

    uint32_t availableLayersCount;
    vector<VkLayerProperties> availableLayers;
    vkEnumerateInstanceLayerProperties(&availableLayersCount, nullptr);
    availableLayers.resize(availableLayersCount);
    vkEnumerateInstanceLayerProperties(&availableLayersCount, availableLayers.data());
    // ------------------------------- //

    // ------------------------ logs ------------------------ //
    {
        stringstream log;
        log << "available instance extensions :\n";
        for (const auto& ext : availableExtensions) {
            log << endl << ext.extensionName << ": " << ext.specVersion;
        }
        logger.debug(log.str());

        log.str("");
        log << "available instance layers :\n";
        for (const auto& lay : availableLayers) {
            log << endl << lay.layerName << ": " << lay.description;
        }
        logger.debug(log.str());
        log.str("");
        log << "required glfw extensions :\n";
        for (const auto& ext : glfwRequiredExtensions) {
            log << endl << ext;
        }
        logger.debug(log.str());
    }
    // ------------------------------------------------------ //

    // ---------- validation --------- //
    for (auto const& reqExt : requiredInstanceExtensions) {
        bool found = false;
        for (auto const& ext : availableExtensions) {
            if (!strcmp(ext.extensionName, reqExt)) {
                found = true;
                break;
            }
        }
        if (!found) {
            throw runtime_error(format("extension {} is not available", reqExt));
        }
    }
    // ------------------------------- //

    // ---------- app info ----------- //
    appCreateInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appCreateInfo.apiVersion = version;
    appCreateInfo.engineVersion = 1;
    appCreateInfo.applicationVersion = 1;
    appCreateInfo.pApplicationName = "App";
    appCreateInfo.pEngineName = "EngineZ";
    // ------------------------------- //

    // ------- debug messenger ------- //
    VkDebugUtilsMessengerCreateInfoEXT messengerCreateInfo{};
    messengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    messengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    messengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    messengerCreateInfo.pUserData = this;
    messengerCreateInfo.pfnUserCallback = VulkanBackend::baseDebugCallback;
    // ------------------------------- //

    // -------- instance info -------- //
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appCreateInfo;
    instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(requiredInstanceExtensions.size());
    instanceCreateInfo.ppEnabledExtensionNames = requiredInstanceExtensions.data();
    instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(requiredInstanceLayers.size());
    instanceCreateInfo.ppEnabledLayerNames = requiredInstanceLayers.data();
    instanceCreateInfo.pNext = &messengerCreateInfo;
    // ------------------------------- //

    if (vkCreateInstance(&instanceCreateInfo, nullptr, &instance) != VK_SUCCESS) {
        throw runtime_error("failed to craete a vulkan instance");
    }
}

void VulkanBackend::setupPhysicalDevice() {
    uint32_t deviceCount;
    vector<VkPhysicalDevice> devices;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    devices.resize(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    vector<uint8_t> scores;
    scores.resize(deviceCount);

    if (deviceCount == 0) {
        throw runtime_error("no devices available");
    }

    stringstream buffer;
    for (int i = 0; i < deviceCount; i++) {
        auto device = devices[i];

        // ----------------------------- data ----------------------------- //
        VkPhysicalDeviceDriverProperties driverProperties{};
        driverProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES;
        driverProperties.pNext = nullptr;
        VkPhysicalDeviceIDProperties idProperties{};
        idProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES;
        idProperties.pNext = &driverProperties;
        VkPhysicalDeviceProperties2 properties{};
        properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        properties.pNext = &idProperties;
        vkGetPhysicalDeviceProperties2(device, &properties);

        VkPhysicalDeviceMemoryProperties memoryProperties{};
        vkGetPhysicalDeviceMemoryProperties(device, &memoryProperties);

        uint32_t availableDeviceExtensionsCount;
        vector<VkExtensionProperties> availableDeviceExtensions;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &availableDeviceExtensionsCount, nullptr);
        availableDeviceExtensions.resize(availableDeviceExtensionsCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &availableDeviceExtensionsCount, availableDeviceExtensions.data());

        uint32_t queueFamilyCount;
        vector<VkQueueFamilyProperties> queueFamilyProperties;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        queueFamilyProperties.resize(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyProperties.data());
        // ---------------------------------------------------------------- //

        // ---------------------------- scoring --------------------------- //
        bool hasExtensions = true;
        bool isDescrete;
        bool supportsQueueFamilies = false; // TODO: currently only a single compute queue is created

        if (properties.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) isDescrete = true;
        for (auto& ext : requiredDeviceExtensions) {
            bool found = false;
            for (auto& ext2 : availableDeviceExtensions) {
                if (!strcmp(ext2.extensionName, ext)) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                hasExtensions = false;
            }
            break;
        }

        for (auto& fam : queueFamilyProperties) { // TODO: currently only a single compute queue is created
            if (fam.queueFlags | VK_QUEUE_COMPUTE_BIT) {
                supportsQueueFamilies = true;
                break;
            }
        }

        scores[i] = isDescrete;
        if (!(hasExtensions && supportsQueueFamilies)) scores[i] = 0;
        // ---------------------------------------------------------------- //

        // ----------------------------- logs ----------------------------- //
        buffer << format("{}. {} :", i, properties.properties.deviceName) << endl;
        buffer << "\ttype: ";
        switch (properties.properties.deviceType) {
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            buffer << "discrete gpu";
            break;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            buffer << "integrated gpu";
            break;
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            buffer << "cpu";
            break;
        default:
            buffer << "unknown";
        }
        buffer << endl;
        buffer << format("\tscore: {}", scores[i]) << endl;
        buffer << format("\tdriver name: {}", driverProperties.driverName) << endl;
        buffer << format("\tnode mask: {}", idProperties.deviceNodeMask) << endl;

        // buffer << "\tavailable extensions:" << endl;
        // for (int j = 0; j < availableDeviceExtensionsCount; j++) {
        //     auto ext = availableDeviceExtensions[j];
        //     buffer << format("\t\t{}: {}", ext.extensionName, ext.specVersion) << endl;
        // }

        buffer << "\tmemory:" << endl;
        buffer << "\t\theaps:" << endl;
        for (int j = 0; j < memoryProperties.memoryHeapCount; j++) {
            auto& heap = memoryProperties.memoryHeaps[j];
            buffer << "\t\t\t" << j << "." << endl;
            buffer << "\t\t\t\tsize: " << heap.size << endl;
        }
        buffer << "\t\ttypes:" << endl;
        for (int j = 0; j < memoryProperties.memoryTypeCount; j++) {
            auto& type = memoryProperties.memoryTypes[j];
            buffer << "\t\t\t" << j << "." << endl;
            buffer << "\t\t\t\theap: " << type.heapIndex << endl;
            buffer << "\t\t\t\tproperties:" << endl;
            if (type.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
                buffer << "\t\t\t\t\tDEVICE LOCAL BIT\n";
            }
            if (type.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
                buffer << "\t\t\t\t\tHOST VISIBLE BIT\n";
            }
            if (type.propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) {
                buffer << "\t\t\t\t\tHOST COHERENT BIT\n";
            }
            if (type.propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) {
                buffer << "\t\t\t\t\tHOST CACHED BIT\n";
            }
            if (type.propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) {
                buffer << "\t\t\t\t\tLAZILY ALLOCATED BIT\n";
            }
            if (type.propertyFlags & VK_MEMORY_PROPERTY_PROTECTED_BIT) {
                buffer << "\t\t\t\t\tPROTECTED BIT\n";
            }
            if (type.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD) {
                buffer << "\t\t\t\t\tDEVICE COHERENT BIT AMD\n";
            }
            if (type.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD) {
                buffer << "\t\t\t\t\tDEVICE UNCACHED BIT AMD\n";
            }
            if (type.propertyFlags & VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV) {
                buffer << "\t\t\t\t\tRDMA CAPABLE BIT NV\n";
            }
        }

        buffer << "\tqueue families:" << endl;
        for (int j = 0; j < queueFamilyCount; j++) {
            auto family = queueFamilyProperties[j];

            buffer << format("\t\t{}.", j) << endl;
            buffer << format("\t\t\tqueue count: {}", family.queueCount) << endl;
            buffer << "\t\t\tcapabilities:" << endl;
            if (family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                buffer << "\t\t\t\tgraphics" << endl;
            }
            if (family.queueFlags & VK_QUEUE_COMPUTE_BIT) {
                buffer << "\t\t\t\tcompute" << endl;
            }
            if (family.queueFlags & VK_QUEUE_TRANSFER_BIT) {
                buffer << "\t\t\t\ttransfer" << endl;
            }
            if (family.queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR) {
                buffer << "\t\t\t\tvideo decode" << endl;
            }
            if (family.queueFlags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR) {
                buffer << "\t\t\t\tvideo encode" << endl;
            }
            if (family.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) {
                buffer << "\t\t\t\tsparse binding" << endl;
            }
            if (family.queueFlags & VK_QUEUE_OPTICAL_FLOW_BIT_NV) {
                buffer << "\t\t\t\toptical flow" << endl;
            }
        }
        // ---------------------------------------------------------------- //
    }
    logger.info(format("devices: \n{}", buffer.str()));

    int chosenIndex = 0;
    for (int j = 0; j < deviceCount; j++) {
        if (scores[j] > scores[chosenIndex]) {
            chosenIndex = j;
        }
    }

    if (scores[chosenIndex] == 0) {
        throw runtime_error("no valid devices found");
    }

    logicalDevice.physicalDevice = devices[chosenIndex];
    vkGetPhysicalDeviceProperties(logicalDevice.physicalDevice, &logicalDevice.properties);
    vkGetPhysicalDeviceMemoryProperties(logicalDevice.physicalDevice, &logicalDevice.memoryProperties);

    logger.info(format("chose {} as the device", logicalDevice.properties.deviceName));
}

void VulkanBackend::setupLogicalDevice() {
    VkDeviceCreateInfo createInfo{};
    VkPhysicalDeviceFeatures features{};

    VkDeviceQueueCreateInfo queueCreateInfo; // TODO: currently only a single compute queue is created
    float priority = 1;
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.queueFamilyIndex = 0;
    queueCreateInfo.pQueuePriorities = &priority;

    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.queueCreateInfoCount = 1;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtensions.size());
    createInfo.ppEnabledExtensionNames = requiredDeviceExtensions.data();
    createInfo.pEnabledFeatures = &features;

    auto result = vkCreateDevice(logicalDevice.physicalDevice, &createInfo, nullptr, &logicalDevice.device);
    if (result != VK_SUCCESS) {
        throw runtime_error("failed to create a device");
    }

    vkGetDeviceQueue(logicalDevice.device, 0, 0, &computeQueue.handle);
    computeQueue.family = 0;
    computeQueue.index = 0;

    stringstream buffer;
    buffer << "compute queue handle: " << computeQueue.handle;
    logger.debug(buffer.str());
}

EnginezWindow* VulkanBackend::createWindow(std::string title, int width, int height) {
    auto window = new VulkanWindow(instance, title, width, height);
    windows.push_back(window);
    return window;
}

void VulkanBackend::cleanUp() {
    for (const auto& window : windows) {
        window->cleanUp();
    }
    for (const auto& buffer : buffers) {
        buffer->cleanUp();
    }

    PFN_vkDestroyDebugUtilsMessengerEXT messengerDestroyFunc =
        (PFN_vkDestroyDebugUtilsMessengerEXT)(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
    messengerDestroyFunc(instance, debugMessenger, nullptr);

    vkDestroyDevice(logicalDevice.device, nullptr);
    vkDestroyInstance(instance, nullptr);

    glfwTerminate();

    logger.info("cleaned up vulkan graphics backend");
}

void VulkanBackend::setupDebugMessenger() {
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pUserData = this;
    createInfo.pfnUserCallback = VulkanBackend::baseDebugCallback;

    PFN_vkCreateDebugUtilsMessengerEXT createFunction =
        (PFN_vkCreateDebugUtilsMessengerEXT)(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
    if (createFunction(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
        throw runtime_error("failed to setup the debug messenger");
    }
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanBackend::baseDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                                VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                                                                const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
    auto backend = static_cast<VulkanBackend*>(pUserData);
    return backend->debugCallback(messageSeverity, messageTypes, pCallbackData);
}

VkBool32 VulkanBackend::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                                      const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData) {
    switch (messageSeverity) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        validationLayerLogger.error(pCallbackData->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        validationLayerLogger.info(pCallbackData->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        validationLayerLogger.warning(pCallbackData->pMessage);
        break;
    default:
        validationLayerLogger.info(pCallbackData->pMessage);
    }

    return VK_FALSE;
}

int32_t VulkanBackend::getSuitableMemoryType(LogicalDevice& logicalDevice, VkMemoryPropertyFlags requiredFlags) {
    if (logicalDevice.device == NULL || logicalDevice.physicalDevice == NULL) return -1;
    for (int i = 0; i < logicalDevice.memoryProperties.memoryTypeCount; i++) {
        auto& type = logicalDevice.memoryProperties.memoryTypes[i];

        if ((requiredFlags & type.propertyFlags) == requiredFlags) {
            return i;
        }
    }
    return -1;
}

Buffer* VulkanBackend::createBuffer(size_t size) {
    auto typeIndex = getSuitableMemoryType(logicalDevice, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    if (typeIndex == -1) {
        return nullptr;
    }

    VulkanBuffer* buffer = new VulkanBuffer(logicalDevice.device, static_cast<uint32_t>(typeIndex), size);
    this->buffers.push_back(buffer);

    return buffer;
}