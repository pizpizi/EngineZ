#pragma once
#include <cstdint>
#include <map>
#include <vector>
#include <vulkan/vulkan_core.h>

struct QueueFamilyIndices{
    QueueFamilyIndices(VkPhysicalDevice device);

    void print(int indent = 0);
    
    std::vector<uint32_t>& operator[](VkQueueFlagBits flagBit) {
        return map[flagBit]; 
    }
private:
    std::map<VkQueueFlagBits, std::vector<uint32_t>> map = std::map<VkQueueFlagBits, std::vector<uint32_t>>();
};