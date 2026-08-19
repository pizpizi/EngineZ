#include "enginez/graphics/vulkan_backend.hpp"
#include "GLFW/glfw3.h"
#include "enginez/graphics/command_buffers.hpp"
#include "enginez/graphics/pipelines.hpp"
#include "enginez/graphics/vulkan_utilities.hpp"
#include "enginez/graphics/vulkan_window.hpp"
#include "logz/logger.hpp"
#include "utilities.hpp"
#include <X11/X.h>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <format>
#include <iostream>
#include <map>
#include <optional>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>

using namespace enginez::graphics;
using namespace std;

void VulkanBackend::setupLogger() {
    logger.addConsoleSink(true, logz::ERROR);
    logger.addFileSink("log.txt", logz::ERROR);

    validationLayerLogger.addConsoleSink(true, logz::ERROR);
    validationLayerLogger.addFileSink("log.txt", logz::ERROR);
}

void VulkanBackend::init(std::vector<Queue>& deviceQueues) {
    setupLogger();
    if (glfwInit() != GLFW_TRUE) {
        logger.error("failed to initialize glfw");
    }
    try {
        setupInstance();
        setupDebugMessenger();
        auto physicalDevice = choosePhysicalDevice(deviceQueues);
        logicalDevice = setupLogicalDevice(deviceQueues, physicalDevice);
    } catch (runtime_error e) {
        logger.error(e.what());
    }
}

void VulkanBackend::setupInstance() {
    uint32_t version;
    vkEnumerateInstanceVersion(&version);
    logger.debug(format("Api version: {}.{}.{}", VK_API_VERSION_MAJOR(version), VK_API_VERSION_MINOR(version), VK_API_VERSION_PATCH(version)));

    VkInstanceCreateInfo instanceCreateInfo{};
    VkApplicationInfo appCreateInfo{};

    // ------------------------- data ------------------------ //
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
    // ------------------------------------------------------- //

    // ------------------------ logs ------------------------- //
    {
        stringstream log;
        log << "available instance extensions :\n";
        for (const auto& ext : availableExtensions) {
            log << '\n' << ext.extensionName << ": " << ext.specVersion;
        }
        logger.debug(log.str());

        log.str("");
        log << "available instance layers :\n";
        for (const auto& lay : availableLayers) {
            log << '\n' << lay.layerName << ": " << lay.description;
        }
        logger.debug(log.str());
        log.str("");
        log << "required glfw extensions :\n";
        for (const auto& ext : glfwRequiredExtensions) {
            log << '\n' << ext;
        }
        logger.debug(log.str());
    }
    // ------------------------------------------------------- //

    // ---------------------- validation --------------------- //
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
    // ------------------------------------------------------- //

    // ---------------------- app info ----------------------- //
    appCreateInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appCreateInfo.apiVersion = version;
    appCreateInfo.engineVersion = 1;
    appCreateInfo.applicationVersion = 1;
    appCreateInfo.pApplicationName = "App";
    appCreateInfo.pEngineName = "EngineZ";
    // ------------------------------- //

    // ------------------- debug messenger ------------------- //
    VkDebugUtilsMessengerCreateInfoEXT messengerCreateInfo{};
    messengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    messengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    messengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    messengerCreateInfo.pUserData = this;
    messengerCreateInfo.pfnUserCallback = VulkanBackend::baseDebugCallback;
    // ------------------------------------------------------- //

    // -------------------- instance info -------------------- //
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appCreateInfo;
    instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(requiredInstanceExtensions.size());
    instanceCreateInfo.ppEnabledExtensionNames = requiredInstanceExtensions.data();
    instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(requiredInstanceLayers.size());
    instanceCreateInfo.ppEnabledLayerNames = requiredInstanceLayers.data();
    instanceCreateInfo.pNext = &messengerCreateInfo;
    // ------------------------------------------------------- //

    if (vkCreateInstance(&instanceCreateInfo, nullptr, &instance) != VK_SUCCESS) {
        throw runtime_error("failed to craete a vulkan instance");
    }
}

PhysicalDevice VulkanBackend::choosePhysicalDevice(std::vector<Queue> deviceQueues) {
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
    for (uint32_t i = 0; i < deviceCount; i++) {
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
        bool isDescrete = false;
        bool supportsQueueFamilies = assignQueues(queueFamilyProperties, deviceQueues);

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
        }

        scores[i] = isDescrete;
        if (!(hasExtensions && supportsQueueFamilies)) scores[i] = 0;
        // ---------------------------------------------------------------- //

        // ----------------------------- logs ----------------------------- //
        buffer << format("{}. {} :", i, properties.properties.deviceName) << '\n';
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
        buffer << '\n';
        buffer << format("\tscore: {}", scores[i]) << '\n';
        buffer << format("\tdriver name: {}", driverProperties.driverName) << '\n';
        buffer << format("\tnode mask: {}", idProperties.deviceNodeMask) << '\n';

        // buffer << "\tavailable extensions:" << '\n';
        // for (int j = 0; j < availableDeviceExtensionsCount; j++) {
        //     auto ext = availableDeviceExtensions[j];
        //     buffer << format("\t\t{}: {}", ext.extensionName, ext.specVersion) << '\n';
        // }

        buffer << "\tmemory:" << '\n';
        buffer << "\t\theaps:" << '\n';
        for (int j = 0; j < memoryProperties.memoryHeapCount; j++) {
            auto& heap = memoryProperties.memoryHeaps[j];
            buffer << "\t\t\t" << j << "." << '\n';
            buffer << "\t\t\t\tsize: " << heap.size << '\n';
        }
        buffer << "\t\ttypes:" << '\n';
        for (int j = 0; j < memoryProperties.memoryTypeCount; j++) {
            auto& type = memoryProperties.memoryTypes[j];
            buffer << "\t\t\t" << j << "." << '\n';
            buffer << "\t\t\t\theap: " << type.heapIndex << '\n';
            buffer << "\t\t\t\tproperties:" << '\n';
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

        buffer << "\tqueue families:" << '\n';
        for (int j = 0; j < queueFamilyCount; j++) {
            auto family = queueFamilyProperties[j];

            buffer << format("\t\t{}.", j) << '\n';
            buffer << format("\t\t\tqueue count: {}", family.queueCount) << '\n';
            buffer << "\t\t\tcapabilities:" << '\n';
            if (family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                buffer << "\t\t\t\tgraphics" << '\n';
            }
            if (family.queueFlags & VK_QUEUE_COMPUTE_BIT) {
                buffer << "\t\t\t\tcompute" << '\n';
            }
            if (family.queueFlags & VK_QUEUE_TRANSFER_BIT) {
                buffer << "\t\t\t\ttransfer" << '\n';
            }
            if (family.queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR) {
                buffer << "\t\t\t\tvideo decode" << '\n';
            }
            if (family.queueFlags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR) {
                buffer << "\t\t\t\tvideo encode" << '\n';
            }
            if (family.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) {
                buffer << "\t\t\t\tsparse binding" << '\n';
            }
            if (family.queueFlags & VK_QUEUE_OPTICAL_FLOW_BIT_NV) {
                buffer << "\t\t\t\toptical flow" << '\n';
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

    PhysicalDevice chosenDevice;
    chosenDevice.handle = devices[chosenIndex];
    vkGetPhysicalDeviceProperties(chosenDevice.handle, &chosenDevice.properties);
    vkGetPhysicalDeviceMemoryProperties(chosenDevice.handle, &chosenDevice.memoryProperties);

    uint32_t familyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(chosenDevice.handle, &familyCount, nullptr);
    chosenDevice.queueFamilyProperties.resize(familyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(chosenDevice.handle, &familyCount, chosenDevice.queueFamilyProperties.data());

    logger.info(format("chose {} as the device", chosenDevice.properties.deviceName));

    return chosenDevice;
}
bool VulkanBackend::assignQueues(vector<VkQueueFamilyProperties>& queueFamilyProperties, std::vector<Queue>& queues) {
    uint32_t familyNum = queueFamilyProperties.size();
    uint32_t queueNum = queues.size();

    vector<uint32_t> filled;
    filled.resize(familyNum);

    for (int i = 0; i < familyNum; i++) {
        filled[i] = 0;
    }

    for (auto& q : queues) {
        q.family = 0;
        q.index = 0;
    }

    int assigned = 0;
    while (assigned != queueNum) {
        bool found = false;
        for (int i = queues[assigned].family; i < familyNum; i++) {
            if (filled[i] == queueFamilyProperties[i].queueCount) continue;
            if (!(queueFamilyProperties[i].queueFlags & queues[assigned].type)) continue;

            queues[assigned].family = i;
            queues[assigned].index = filled[i];
            filled[i]++;
            found = true;
            break;
        }
        if (!found && !assigned) {
            break;
        }
        if (!found && assigned) {
            queues[assigned].family = 0;
            assigned--;
            filled[queues[assigned].family]--;
            queues[assigned].family++;
            continue;
        }

        assigned++;
    }

    return assigned == queueNum;
}
Device VulkanBackend::setupLogicalDevice(std::vector<Queue>& deviceQueues, PhysicalDevice phyisicalDevice) {
    VkDeviceCreateInfo createInfo{};
    VkPhysicalDeviceFeatures features{};

    if (!assignQueues(phyisicalDevice.queueFamilyProperties, deviceQueues)) {
        throw runtime_error(format("failed to assign the required queues to {} queue families", phyisicalDevice.properties.deviceName));
    }

    std::map<uint32_t, uint32_t> familyCount;
    for (auto& q : deviceQueues) {
        familyCount[q.family]++;
    }
    std::vector<float> queuePriorities(deviceQueues.size(), 1.0f);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    queueCreateInfos.reserve(familyCount.size());

    for (const auto& [familyIndex, count] : familyCount) {
        VkDeviceQueueCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        info.queueFamilyIndex = familyIndex;
        info.queueCount = count;
        info.pQueuePriorities = queuePriorities.data();

        queueCreateInfos.push_back(info);
    }

    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtensions.size());
    createInfo.ppEnabledExtensionNames = requiredDeviceExtensions.data();
    createInfo.pEnabledFeatures = &features;

    VkDevice handle;
    auto result = vkCreateDevice(phyisicalDevice.handle, &createInfo, nullptr, &handle);
    if (result != VK_SUCCESS) {
        throw runtime_error("failed to create a device");
    }

    for (auto& q : deviceQueues) {
        vkGetDeviceQueue(handle, q.family, q.index, &q.handle);
    }

    return Device(handle, phyisicalDevice);
}

VulkanWindow* VulkanBackend::createWindow(std::string title, int width, int height) {
    auto window = new VulkanWindow(instance, std::move(title), width, height);
    windows.push_back(window);
    return window;
}

void VulkanBackend::cleanUp() {
    // for (const auto& window : windows) {
    //     window->cleanUp();
    // }
    // for (const auto& buffer : memoryBlocks) {
    //     vkFreeMemory(logicalDevice.device, buffer.handle, nullptr);
    // }
    // for (const auto& buffer : buffers) {
    //     vkDestroyBuffer(logicalDevice.device, buffer.handle, nullptr);
    // }
    // for (const auto& shader : shaders) {
    //     vkDestroyShaderModule(logicalDevice.device, shader.handle, nullptr);
    // }
    vkDestroyDevice(logicalDevice.handle, nullptr);

    PFN_vkDestroyDebugUtilsMessengerEXT messengerDestroyFunc =
        (PFN_vkDestroyDebugUtilsMessengerEXT)(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
    messengerDestroyFunc(instance, debugMessenger, nullptr);

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
        validationLayerLogger.error(pCallbackData->pMessage);
        break;
    default:
        validationLayerLogger.info(pCallbackData->pMessage);
        break;
    }

    return VK_FALSE;
}

int32_t VulkanBackend::getSuitableMemoryType(Device& logicalDevice, VkMemoryPropertyFlags requiredFlags) {
    if (logicalDevice.handle == VK_NULL_HANDLE || logicalDevice.phyisicalDevice.handle == VK_NULL_HANDLE) return -1;
    for (int i = 0; i < logicalDevice.phyisicalDevice.memoryProperties.memoryTypeCount; i++) {
        auto& type = logicalDevice.phyisicalDevice.memoryProperties.memoryTypes[i];

        if ((requiredFlags & type.propertyFlags) == requiredFlags) {
            return i;
        }
    }
    return -1;
}

//    +----------------------------------------------------+
//    |                  memory allocation                 |
//    +----------------------------------------------------+

std::optional<MemoryBlock> VulkanBackend::allocateMemory(size_t size) {
    VkMemoryPropertyFlags requiredProperties =
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    auto typeIndex = getSuitableMemoryType(logicalDevice, requiredProperties);
    if (typeIndex == -1) {
        logger.error("failed to find a suitable memory type for allocation");
        return std::nullopt;
    }

    VkMemoryAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize = size;
    allocateInfo.memoryTypeIndex = typeIndex;

    VkDeviceMemory memoryHandle;
    if (vkAllocateMemory(logicalDevice.handle, &allocateInfo, nullptr, &memoryHandle) != VK_SUCCESS) {
        logger.error(format("failed to allocate memory of size {}", size));
        return std::nullopt;
    }

    return MemoryBlock(memoryHandle, requiredProperties, typeIndex);
}
void VulkanBackend::downloadFromMemory(MemoryBlock& block, void* dst, size_t size, size_t offset) {
    void* mappedMemory;
    vkMapMemory(logicalDevice.handle, block.handle, offset, size, 0, &mappedMemory);
    memcpy(dst, mappedMemory, size);
    vkUnmapMemory(logicalDevice.handle, block.handle);
}
void VulkanBackend::uploadToMemory(MemoryBlock& block, void* src, size_t size, size_t offset) {
    void* mappedMemory;
    vkMapMemory(logicalDevice.handle, block.handle, offset, size, 0, &mappedMemory);
    memcpy(mappedMemory, src, size);
    vkUnmapMemory(logicalDevice.handle, block.handle);
}
void VulkanBackend::cleanUpMemoryBlock(MemoryBlock& block) {
    vkFreeMemory(logicalDevice.handle, block.handle, nullptr);
}

//    +----------------------------------------------------+
//    |                       shaders                      |
//    +----------------------------------------------------+

std::optional<Shader> VulkanBackend::createShader(const char* filePath) {
    auto code = ReadBinaryFile(filePath);

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = static_cast<uint32_t>(code.size());
    createInfo.pCode = reinterpret_cast<uint32_t*>(code.data());

    VkShaderModule handle;
    if (vkCreateShaderModule(logicalDevice.handle, &createInfo, nullptr, &handle) != VK_SUCCESS) {
        logger.error(format("failed to make a shader module from \"{}\"", filePath));
        return std::nullopt;
    }

    return Shader(handle);
}
void VulkanBackend::cleanUpShader(Shader& shader) {
    vkDestroyShaderModule(logicalDevice.handle, shader.handle, nullptr);
}

//    +----------------------------------------------------+
//    |                       buffers                      |
//    +----------------------------------------------------+

std::optional<Buffer> VulkanBackend::createBuffer(size_t size, BufferType type, MemoryBlock& block, size_t offset) {
    VkBufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = size;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferCreateInfo.usage = type;

    VkBuffer bufferHandle;
    if (vkCreateBuffer(logicalDevice.handle, &bufferCreateInfo, nullptr, &bufferHandle) != VK_SUCCESS) {
        logger.errorf("failed to create buffer of size {}", size);
        return std::nullopt;
    }

    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(logicalDevice.handle, bufferHandle, &memoryRequirements);

    logger.debugf("buffer memory requirements:\n\tsize: {}\n\talignment:{}\n\ttypes:{}", memoryRequirements.size, memoryRequirements.alignment,
                  memoryRequirements.memoryTypeBits);

    if (!((1 << block.typeIndex) & memoryRequirements.memoryTypeBits)) {
        logger.error("memory block doesn't fit buffer requirements");
        return std::nullopt;
    }

    if (vkBindBufferMemory(logicalDevice.handle, bufferHandle, block.handle, offset) != VK_SUCCESS) {
        logger.error("failed to bind buffer memory");
        return std::nullopt;
    }

    return Buffer(bufferHandle, memoryRequirements.size);
    ;
}
void VulkanBackend::cleanUpBuffer(Buffer& buffer) {
    vkDestroyBuffer(logicalDevice.handle, buffer.handle, nullptr);
}

//    +----------------------------------------------------+
//    |                     descriptors                    |
//    +----------------------------------------------------+

std::optional<DescriptorSetLayout> VulkanBackend::createDescriptorSetLayout(VkDescriptorSetLayoutBinding* bindings, uint32_t bindingCount) {
    VkDescriptorSetLayoutCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    createInfo.bindingCount = bindingCount;
    createInfo.pBindings = bindings;

    VkDescriptorSetLayout handle;
    if (vkCreateDescriptorSetLayout(logicalDevice.handle, &createInfo, nullptr, &handle) != VK_SUCCESS) {
        logger.error("failed to create the descriptor set layout");
        return std::nullopt;
    }

    return DescriptorSetLayout(handle);
}
void VulkanBackend::cleanUpDescriptorSetLayout(DescriptorSetLayout layout) {
    vkDestroyDescriptorSetLayout(logicalDevice.handle, layout, nullptr);
}
std::optional<DescriptorPool> VulkanBackend::createDescriptorSetPool(std::map<VkDescriptorType, uint32_t> resourceCount, uint32_t maxSets) {
    std::vector<VkDescriptorPoolSize> poolSizes;
    poolSizes.reserve(resourceCount.size());
    for (auto& rsc : resourceCount) {
        poolSizes.push_back({.type = rsc.first, .descriptorCount = rsc.second});
    }

    VkDescriptorPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    createInfo.maxSets = maxSets;
    createInfo.poolSizeCount = static_cast<uint32_t>(resourceCount.size());
    createInfo.pPoolSizes = poolSizes.data();

    VkDescriptorPool handle;

    if (vkCreateDescriptorPool(logicalDevice.handle, &createInfo, nullptr, &handle) != VK_SUCCESS) {
        logger.error("failed to create descriptor pool");
        return std::nullopt;
    }

    return DescriptorPool(handle);
}
void VulkanBackend::cleanUpcreateDescriptorSetPool(DescriptorPool layout) {
    vkDestroyDescriptorPool(logicalDevice.handle, layout.handle, nullptr);
}
bool VulkanBackend::allocateDescriptorSets(DescriptorPool pool, uint32_t count, DescriptorSetLayout* pLayouts, DescriptorSet* pDescriptorSets) {
    VkDescriptorSetAllocateInfo allocationInfo{};
    allocationInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocationInfo.descriptorSetCount = count;
    allocationInfo.descriptorPool = pool.handle;
    allocationInfo.pSetLayouts = pLayouts;

    if (vkAllocateDescriptorSets(logicalDevice.handle, &allocationInfo, pDescriptorSets) != VK_SUCCESS) {
        logger.error("failed to allocate descriptor sets");
        return false;
    }

    return true;
}
void VulkanBackend::updateDescriptorSets(std::vector<VkWriteDescriptorSet> writes, std::vector<VkCopyDescriptorSet> copies) {
    vkUpdateDescriptorSets(logicalDevice.handle, static_cast<uint32_t>(writes.size()), writes.data(), static_cast<uint32_t>(copies.size()),
                           copies.data());
}

//    +----------------------------------------------------+
//    |                      pipelines                     |
//    +----------------------------------------------------+

std::optional<PipelineLayout> VulkanBackend::createPipelineLayout(uint32_t descriptorSetCount, DescriptorSetLayout* pDescriptorSetLayouts,
                                                                  uint32_t pushConstantRangesCount, VkPushConstantRange* pPushConstantRanges) {
    VkPipelineLayoutCreateInfo layoutCreateInfo{};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutCreateInfo.setLayoutCount = descriptorSetCount;
    layoutCreateInfo.pSetLayouts = pDescriptorSetLayouts;
    layoutCreateInfo.pushConstantRangeCount = pushConstantRangesCount;
    layoutCreateInfo.pPushConstantRanges = pPushConstantRanges;

    VkPipelineLayout handle;
    if (vkCreatePipelineLayout(logicalDevice.handle, &layoutCreateInfo, nullptr, &handle) != VK_SUCCESS) {
        logger.error("failed to create pipeline layout");
        return std::nullopt;
    }

    return PipelineLayout(handle);
}
std::optional<PipeLine> VulkanBackend::createComputePipeline(Shader& computeShader, PipelineLayout& layout) {
    VkPipelineShaderStageCreateInfo shaderStageCreateInfo{};
    shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shaderStageCreateInfo.module = computeShader.handle;
    shaderStageCreateInfo.pName = "main";

    VkComputePipelineCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    createInfo.stage = shaderStageCreateInfo;
    createInfo.layout = layout.handle;

    VkPipeline handle;
    if (vkCreateComputePipelines(logicalDevice.handle, nullptr, 1, &createInfo, nullptr, &handle) != VK_SUCCESS) {
        logger.error("failed to create compute pipeline");
        return std::nullopt;
    }

    return PipeLine(handle);
}

//    +----------------------------------------------------+
//    |                   command buffers                  |
//    +----------------------------------------------------+

optional<CommandPool> VulkanBackend::createCommandPool(Queue& queue) {
    VkCommandPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.queueFamilyIndex = queue.family;

    VkCommandPool handle;
    if (vkCreateCommandPool(logicalDevice.handle, &createInfo, nullptr, &handle) != VK_SUCCESS) {
        logger.error("failed to create command pool");
        return nullopt;
    }

    return CommandPool(handle);
}
optional<CommandBuffer> VulkanBackend::allocateCommandBuffer(CommandPool& pool) {
    VkCommandBuffer handle;

    VkCommandBufferAllocateInfo allocationInfo{};
    allocationInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocationInfo.commandPool = pool.handle;
    allocationInfo.commandBufferCount = 1; // TODO
    allocationInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    if (vkAllocateCommandBuffers(logicalDevice.handle, &allocationInfo, &handle) != VK_SUCCESS) {
        logger.error("failed to allocate command buffers");
        return nullopt;
    }

    return CommandBuffer(handle);
}
bool VulkanBackend::submitAndSynchronize(CommandBuffer commandBuffer, Queue& queue) {
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer.handle;

    if (vkQueueSubmit(queue.handle, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
        logger.error("Failed to submit compute command buffer!");
        return false;
    }

    // Wait for the queue to finish executing the submitted commands
    if (vkQueueWaitIdle(queue.handle) != VK_SUCCESS) {
        logger.error("Failed to wait for queue to become idle!");
        return false;
    }

    return true;
}

void VulkanBackend::update() {
    glfwPollEvents();

    for (int i = windows.size() - 1; i >= 0; i--) {
        auto& win = windows[i];
        win->update();

        if (win->isClosed()) {
            win->cleanUp();
            windows.erase(windows.begin() + i);
        }
    }
}