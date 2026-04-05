#include "vulkan_backend.hpp"
#include "GLFW/glfw3.h"
#include "enginez/graphics/enginez_window.hpp"
#include "logz/logger.hpp"
#include "vulkan/vulkan_window.hpp"
#include "vulkan_utilities.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <format>
#include <iostream>
#include <memory>
#include <ostream>
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
        logger.info("created the vulkan instance");

        setupDebugMessenger();
        logger.info("setup the debug messenger");
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
    for (auto const& reqExt : glfwRequiredExtensions) {
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

EnginezWindow* VulkanBackend::createWindow(std::string title, int width, int height) {
    auto window = std::make_unique<VulkanWindow>(instance, title, width, height);
    windows.push_back(std::move(window));

    logger.info("created window");

    return windows.back().get();
}

void VulkanBackend::cleanUp() {
    for (const auto& window : windows) {
        window->cleanUp();
    }

    PFN_vkDestroyDebugUtilsMessengerEXT messengerDestroyFunc = (PFN_vkDestroyDebugUtilsMessengerEXT)(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
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

    PFN_vkCreateDebugUtilsMessengerEXT createFunction = (PFN_vkCreateDebugUtilsMessengerEXT)(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
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