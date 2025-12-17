#pragma once
#include "VkExtendedQueueFlagBits.hpp"
#include <cstddef>
#include <cstdint>
#include <map>
#include <vector>
#include <vulkan/vulkan_core.h>

struct QueueFamilyIndices{
    QueueFamilyIndices(VkPhysicalDevice device, VkSurfaceKHR* surface = NULL);

    void print(int indent = 0);
    
    std::vector<uint32_t>& operator[](VkExtendedQueueFlagBits flagBit) {
        return map[flagBit]; 
    }
private:
    std::map<VkExtendedQueueFlagBits, std::vector<uint32_t>> map = std::map<VkExtendedQueueFlagBits, std::vector<uint32_t>>();
};