#include "VulkanUtilities.hpp"
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <map>
#include <string>
#include <vulkan/vulkan_core.h>


using namespace VulkanUtilities;

static std::vector<VkExtendedQueueFlagBits> flagBits = {
    GRAPHICS,
    COMPUTE,
    TRANSFER,
    SPARSE_BINDING,
    PROTECTED,
    VIDEO_DECODE,
    VIDEO_ENCODE,
    OPTICAL_FLOW,
    PRESENT
};

static const std::map<VkExtendedQueueFlagBits, std::string> flagNames = {
    {GRAPHICS, "Graphics"},
    {COMPUTE, "Compute"},
    {TRANSFER, "Transfer"},
    {SPARSE_BINDING, "Sparse Binding"},
    {PROTECTED, "Protected"},
    {VIDEO_DECODE, "Video Decode"},
    {VIDEO_ENCODE, "Video Encode"},
    {OPTICAL_FLOW, "Optical Flow"},
    {PRESENT, "Present"}
};

QueueFamilyIndices::QueueFamilyIndices(VkPhysicalDevice device, VkSurfaceKHR* surface){
    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> families = std::vector<VkQueueFamilyProperties>(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, families.data());

    for(uint32_t i = 0; i < families.size(); i++){
        for(auto flagBit: flagBits){
            if(families[i].queueFlags & flagBit){
                map[flagBit].push_back(i);
            }
        }
        if(surface != NULL){
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, *surface, &presentSupport);
            if(presentSupport) map[PRESENT].push_back(i);
        }
    }
}

void QueueFamilyIndices::print(int indent){
    for(int i = 0; i < flagBits.size(); i++){
        for(int j = 0 ; j < indent; j++){
            std::cout<<"\t";
        }
        auto flagBit = flagBits[i]; 
        std::cout<<flagNames.at(flagBit) << ":";
        for(int index: map[flagBit]){
            std::cout<<" "<<index;
        }
        std::cout<<std::endl;
    }
}