#include "QueueFamilyIndecies.hpp"
#include <iostream>
#include <map>
#include <string>
#include <vulkan/vulkan_core.h>

static std::vector<VkQueueFlagBits> flagBits = {
    VK_QUEUE_GRAPHICS_BIT,
    VK_QUEUE_COMPUTE_BIT,
    VK_QUEUE_TRANSFER_BIT,
    VK_QUEUE_SPARSE_BINDING_BIT,
    VK_QUEUE_PROTECTED_BIT,
    VK_QUEUE_VIDEO_DECODE_BIT_KHR,
    VK_QUEUE_VIDEO_ENCODE_BIT_KHR,
    VK_QUEUE_OPTICAL_FLOW_BIT_NV
};

static const std::map<VkQueueFlagBits, std::string> flagNames = {
    {VK_QUEUE_GRAPHICS_BIT, "Graphics"},
    {VK_QUEUE_COMPUTE_BIT, "Compute"},
    {VK_QUEUE_TRANSFER_BIT, "Transfer"},
    {VK_QUEUE_SPARSE_BINDING_BIT, "Sparse Binding"},
    {VK_QUEUE_PROTECTED_BIT, "Protected"},
    {VK_QUEUE_VIDEO_DECODE_BIT_KHR, "Video Decode"},
    {VK_QUEUE_VIDEO_ENCODE_BIT_KHR, "Video Encode"},
    {VK_QUEUE_OPTICAL_FLOW_BIT_NV, "Optical Flow"}
};

QueueFamilyIndices::QueueFamilyIndices(VkPhysicalDevice device){
    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> families = std::vector<VkQueueFamilyProperties>(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, families.data());

    for(int i = 0; i < families.size(); i++){
        for(auto flagBit: flagBits){
            if(families[i].queueFlags & flagBit){
                map[flagBit].push_back(i);
            }
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